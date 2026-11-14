// SPDX-License-Identifier: MIT
#include "platform/windows/presentmon_process.h"
#include "platform/windows/state_publisher.h"

#include <atomic>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <stdexcept>

namespace {
namespace fps = phone_monitor::fps;
namespace windows = phone_monitor::windows;

class ContractFailure final : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

void require(bool condition, const char *message) {
    if (!condition)
        throw ContractFailure(message);
}

class TemporaryDirectory {
  public:
    TemporaryDirectory() {
        const auto base = std::filesystem::temp_directory_path();
        for (unsigned i = 0; i < 1000; ++i) {
            path = base / ("phone-monitor-windows-test-" + std::to_string(i));
            if (std::filesystem::create_directory(path))
                return;
        }
        throw std::runtime_error("Cannot create isolated Windows test directory");
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
    std::filesystem::path path;
};

void drain(windows::PresentMonProcess &capture) {
    const auto deadline = GetTickCount64() + 5000;
    while (capture.running(100))
        require(GetTickCount64() < deadline, "Synthetic child did not finish");
}

void test_process_output(fps::BridgeConfig config) {
    for (const auto *mode : {L"normal", L"tail", L"limit"}) {
        config.session_name = mode;
        std::string output;
        windows::PresentMonProcess capture(config, [&](auto line) { output.assign(line); });
        drain(capture);
        // running() returning false guarantees that the reader joined before this access.
        if (config.session_name == L"normal")
            require(output == "synthetic row", "Complete child row was lost");
        else if (config.session_name == L"tail")
            require(output == "synthetic tail", "Unterminated final child row was lost");
        else
            require(output.size() == 65536, "Maximum allowed child row was rejected");
    }
    for (const auto *mode : {L"failure", L"oversized", L"callback"}) {
        config.session_name = mode;
        bool rejected = false;
        bool delivered = false;
        try {
            windows::PresentMonProcess capture(config, [&](auto) {
                delivered = true;
                if (config.session_name == L"callback")
                    throw std::runtime_error("Synthetic consumer failure");
            });
            drain(capture);
        } catch (const ContractFailure &) {
            throw;
        } catch (const std::runtime_error &) {
            rejected = true;
        }
        require(rejected, "Child exit, line limit or reader failure was silently accepted");
        if (config.session_name == L"oversized")
            require(!delivered, "Oversized row reached the consumer before validation");
    }
}

void test_kill_on_close(fps::BridgeConfig config) {
    config.session_name = L"lifetime";
    windows::Handle child;
    std::atomic<DWORD> pid{0};
    {
        windows::PresentMonProcess capture(config, [&](std::string_view line) {
            DWORD value = 0;
            const auto parsed = std::from_chars(line.data(), line.data() + line.size(), value);
            if (parsed.ec == std::errc{} && parsed.ptr == line.data() + line.size())
                pid = value;
        });
        const auto deadline = GetTickCount64() + 5000;
        while (pid == 0) {
            require(capture.running(20), "Synthetic lifetime child exited too early");
            require(GetTickCount64() < deadline, "Synthetic lifetime child did not start");
        }
        child.reset(OpenProcess(SYNCHRONIZE, FALSE, pid));
        require(static_cast<bool>(child), "Cannot observe synthetic child lifetime");
    }
    require(WaitForSingleObject(child.get(), 1000) == WAIT_OBJECT_0,
            "Child survived its capture owner");
}

fps::StateRecord read_state(const std::filesystem::path &file) {
    std::ifstream input(file);
    std::string text;
    std::getline(input, text);
    const auto state = fps::parse_state(text);
    require(state.has_value(), "Published state was not a complete record");
    return *state;
}

void test_state_lifetime() {
    TemporaryDirectory directory;
    const auto file = directory.path / "fps-state.txt";
    const fps::StateRecord sample{100, {60, 45, 16.67, "synthetic.exe", true}};
    std::ofstream(file) << fps::serialize_state(sample);
    try {
        windows::StatePublisher state(file);
        require(read_state(file).timestamp_ms == 0, "Previous capture state was retained");
        state.publish(sample);
        require(read_state(file).sample.process_name == "synthetic.exe",
                "Synthetic state was not published");
        throw std::runtime_error("Synthetic capture failure");
    } catch (const std::runtime_error &error) {
        require(std::string_view(error.what()) == "Synthetic capture failure",
                "State cleanup replaced the original error");
    }
    const auto cleared = read_state(file);
    require(cleared.timestamp_ms == 0 && cleared.sample.process_name.empty(),
            "Exception left stale capture readings");

    {
        windows::StatePublisher state(file);
        // Model std::ifstream denying delete-sharing while Rainmeter reads the last record.
        windows::Handle reader(CreateFileW(file.c_str(), GENERIC_READ,
                                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                           OPEN_EXISTING, 0, nullptr));
        require(static_cast<bool>(reader), "Cannot hold synthetic state reader");
        auto release = std::async(std::launch::async, [handle = std::move(reader)]() mutable {
            Sleep(30);
            handle.reset();
        });
        state.publish(sample);
        release.get();
        require(read_state(file).timestamp_ms == 100, "Transient sharing blocked state publish");
    }
    require(read_state(file).timestamp_ms == 0, "Normal exit retained capture state");
    auto temporary = file;
    temporary += L".tmp";
    require(!std::filesystem::exists(temporary), "Temporary runtime state was retained");

    // A permanent rename failure must propagate and clean the temporary copy.
    const auto blocked = directory.path / "blocked-state";
    std::filesystem::create_directory(blocked);
    std::ofstream(blocked / "keep.txt") << "synthetic fixture";
    bool rejected = false;
    try {
        windows::StatePublisher state(blocked);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    require(rejected, "Permanent state replacement failure was silently accepted");
    temporary = blocked;
    temporary += L".tmp";
    require(!std::filesystem::exists(temporary), "Failed replacement retained temporary data");
    require(std::filesystem::is_regular_file(blocked / "keep.txt"),
            "Failed replacement changed the existing destination");
}
} // namespace

int main(int argc, char **argv) {
    const char *stage = "fixture setup";
    try {
        require(argc == 2, "Synthetic child fixture argument is required");
        fps::BridgeConfig config;
        config.presentmon = std::filesystem::absolute(argv[1]);
        stage = "child output and exit status";
        test_process_output(config);
        stage = "kill-on-close job";
        test_kill_on_close(config);
        stage = "local state lifecycle";
        test_state_lifetime();
        std::cout << "Windows contracts passed (headless synthetic child and local state only).\n";
        return 0;
    } catch (const ContractFailure &error) {
        // Assertion messages are fixed source literals, not runtime payloads.
        std::cerr << "Windows contract failed: " << error.what() << '\n';
        return 1;
    } catch (const std::exception &) {
        // Keep even failed tests free of arbitrary child output or local filesystem paths.
        std::cerr << "Windows contract failed during " << stage << ".\n";
        return 1;
    }
}
