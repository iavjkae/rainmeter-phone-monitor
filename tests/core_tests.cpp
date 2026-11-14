// SPDX-License-Identifier: MIT
#include "fps/config.h"
#include "fps/csv_reader.h"
#include "fps/frame_history.h"
#include "fps/process_filter.h"
#include "fps/state_protocol.h"
#include "telemetry/service.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
namespace fps = phone_monitor::fps;
using namespace phone_monitor;

void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

void close_to(double actual, double expected) {
    require(std::abs(actual - expected) < 0.001, "Unexpected aggregate result");
}

class FakeGpu final : public GpuProvider {
  public:
    GpuSample sample() override {
        GpuSample result;
        result.status = SensorStatus::available;
        result.name = L"Synthetic GPU";
        result.utilization_percent = 25;
        return result;
    }
};
class FakeCpu final : public CpuProvider {
  public:
    explicit FakeCpu(unsigned &reads) : reads_(reads) {}
    CpuIdentity identity() override {
        ++reads_;
        return {SensorStatus::available, L"Synthetic CPU"};
    }

  private:
    unsigned &reads_;
};
class FakeFps final : public FpsSource {
  public:
    FpsSample sample(std::uint64_t) override {
        return {};
    }
};

class TemporaryDirectory {
  public:
    TemporaryDirectory() {
        const auto base = std::filesystem::temp_directory_path();
        for (unsigned i = 0; i < 1000; ++i) {
            path = base / ("phone-monitor-core-test-" + std::to_string(i));
            if (std::filesystem::create_directory(path))
                return;
        }
        throw std::runtime_error("Cannot create isolated test directory");
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
    std::filesystem::path path;
};

void test_service() {
    unsigned reads = 0;
    TelemetryService service(std::make_unique<FakeGpu>(), std::make_unique<FakeCpu>(reads),
                             std::make_unique<FakeFps>());
    for (unsigned i = 0; i < 3; ++i) {
        const auto snapshot = service.sample(i);
        require(snapshot.gpu.utilization_percent == 25, "GPU provider was not called");
        require(!snapshot.gpu.power_watts, "Missing metric must not become a real zero");
        require(snapshot.cpu.name == L"Synthetic CPU", "CPU identity not retained");
    }
    require(reads == 1, "CPU identity must be cached");
}

void test_protocol() {
    const fps::StateRecord state{100, {60, 45, 16.67, "synthetic.exe", true}};
    const auto parsed = fps::parse_state(fps::serialize_state(state));
    require(parsed.has_value(), "State roundtrip failed");
    close_to(parsed->sample.frames_per_second, 60);
    require(parsed->sample.process_name == "synthetic.exe", "Process name changed");
    for (const auto *input : {"", "1|2|3", "-1|60|40|15|sample.exe", "1|60|-4|15|sample.exe",
                              "1|nan|1|1|sample.exe", "1|60|1|1|bad|field"})
        require(!fps::parse_state(input), "Malformed state accepted");
    require(fps::is_fresh(100, 200, 101), "Recent state rejected");
    require(!fps::is_fresh(100, 200, 100), "Expired state accepted");
    require(!fps::is_fresh(200, 100, 1000), "Future timestamp accepted");

    TemporaryDirectory directory;
    const auto file = directory.path / "state.txt";
    auto source = make_file_fps_source(file, 100);
    require(!source->sample(110).collector_active, "Missing state must be inactive");
    std::ofstream(file) << fps::serialize_state(state);
    require(source->sample(110).collector_active, "Fresh state rejected by source");
    require(!source->sample(200).collector_active, "Stale FPS was retained");
}

void test_csv() {
    fps::CsvReader reader;
    require(!reader.consume("sample.exe,1,0x1,16"), "Headerless CSV accepted");
    reader.consume("MsBetweenPresents,SwapChainAddress,Application,ProcessID\r");
    const auto event = reader.consume("16.5,0x1,\"synthetic,render.exe\",42");
    require(event && event->process_name == "synthetic,render.exe",
            "Quoted or reordered CSV failed");
    require(event->process_id == 42, "Wrong process id");
    close_to(event->frame_time_ms, 16.5);
    for (const auto *line : {"1,0x1", "nan,0x1,sample.exe,42", "1,0x1,sample.exe,-1",
                             "1,0x1,sample.exe,42tail", "1,0x1,\"broken.exe,42"})
        require(!reader.consume(line), "Malformed CSV row accepted");
    reader.consume("Application,ProcessID");
    require(!reader.has_header(), "Incomplete replacement header retained stale columns");
    reader.consume("\xef\xbb\xbf"
                   "Application,ProcessID,SwapChainAddress,MsBetweenPresents");
    require(reader.has_header(), "UTF-8 BOM header rejected");
}

void test_filter_and_config() {
    require(fps::ProcessFilter({}).allows("anything.exe"), "Default filter must be empty");
    fps::ProcessFilter filter({{"sample*.exe"}, {"sample-helper.exe"}, true});
    require(filter.allows("SAMPLE-game.EXE"), "Case-insensitive pattern failed");
    require(!filter.allows("sample-helper.exe"), "Exclusion must take precedence");
    require(!filter.allows("different.exe"), "Include rule bypassed");
    TemporaryDirectory directory;
    const auto base = directory.path / "base.ini";
    const auto local = directory.path / "local.ini";
    std::ofstream(base)
        << "[Selection]\nInclude=sample*.exe\nExclude=\n[History]\nRecentWindowMs=1000\n";
    std::ofstream(local) << "[Selection]\nExclude=sample-helper.exe\nForegroundOnly=0\n";
    const auto config = fps::load_config(base, local);
    require(config.selection.exclude.size() == 1 && !config.selection.foreground_only,
            "Local override failed");
    require(config.presentmon == directory.path / "PresentMon.exe", "Config-relative path failed");
    std::ofstream(local) << "[Capture]\nUpdateIntervalMs=-1\n";
    bool rejected = false;
    try {
        fps::load_config(base, local);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    require(rejected, "Negative duration accepted");
    std::ofstream(local) << "[Selection]\nTypo=1\n";
    rejected = false;
    try {
        fps::load_config(base, local);
    } catch (const std::runtime_error &) {
        rejected = true;
    }
    require(rejected, "Unknown configuration key silently accepted");
}

void test_history() {
    fps::HistoryPolicy policy;
    policy.history_window_ms = 3000;
    fps::FrameHistory history(policy);
    for (std::uint64_t i = 0; i < 100; ++i)
        history.add({1, "synthetic.exe", "chain-a", i == 99 ? 100.0 : 10.0}, 100 + i * 10);
    const auto sample = history.sample(1100, 1, true);
    close_to(sample.frames_per_second, 100000.0 / 1090.0);
    close_to(sample.one_percent_low, 10);
    require(history.sample(1100, 2, true).frames_per_second == 0, "Foreground policy ignored");
    require(history.sample(1100, 2, false).frames_per_second > 0,
            "Explicit background fallback failed");
    require(history.sample(5000, 1, true).frames_per_second == 0,
            "Old history must expire without new frames");
    for (std::uint64_t i = 0; i < 3; ++i)
        history.add({2, "another.exe", "chain-b", 20}, 5010 + i * 20);
    close_to(history.sample(5100, 2, true).frames_per_second, 50);
}

void test_history_snapshot_boundary() {
    fps::FrameHistory history({});
    for (std::uint64_t time : {100, 110, 120})
        history.add({1, "synthetic.exe", "chain-a", 10}, time);
    // Simulate the reader winning the lock after the sampler captured its timestamp.
    history.add({1, "synthetic.exe", "chain-a", 1000}, 130);
    const auto earlier = history.sample(120, 1, true);
    close_to(earlier.frames_per_second, 100);
    close_to(earlier.one_percent_low, 100);
    close_to(earlier.frame_time_ms, 10);
    require(history.sample(99, 1, true).frames_per_second == 0,
            "Snapshot before the first frame must be empty");
    close_to(history.sample(130, 1, true).one_percent_low, 1);
    // A much later queued event must not keep an already stale snapshot alive.
    history.add({1, "synthetic.exe", "chain-a", 10}, 5000);
    require(history.sample(4000, 1, true).frames_per_second == 0,
            "Future event bypassed the snapshot stale check");
}

} // namespace

int main() {
    try {
        test_service();
        test_protocol();
        test_csv();
        test_filter_and_config();
        test_history();
        test_history_snapshot_boundary();
        std::cout << "Core contracts passed (synthetic fixtures only).\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "Core contract failed: " << error.what() << '\n';
        return 1;
    }
}
