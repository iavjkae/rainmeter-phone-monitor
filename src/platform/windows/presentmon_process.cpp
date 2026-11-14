// SPDX-License-Identifier: MIT
#include "platform/windows/presentmon_process.h"

#include <array>
#include <stdexcept>
#include <string>
#include <utility>

namespace phone_monitor::windows {

PresentMonProcess::PresentMonProcess(const fps::BridgeConfig &config, LineHandler handler) {
    SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
    HANDLE read_pipe = nullptr, write_pipe = nullptr;
    if (!CreatePipe(&read_pipe, &write_pipe, &security, 0))
        throw std::runtime_error("Cannot create PresentMon output pipe");
    pipe_.reset(read_pipe);
    Handle output(write_pipe);
    if (!SetHandleInformation(pipe_.get(), HANDLE_FLAG_INHERIT, 0))
        throw std::runtime_error("Cannot protect PresentMon input handle");
    Handle input(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security,
                             OPEN_EXISTING, 0, nullptr));
    if (!input)
        throw std::runtime_error("Cannot initialize PresentMon input");
    job_.reset(CreateJobObjectW(nullptr, nullptr));
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job_ || !SetInformationJobObject(job_.get(), JobObjectExtendedLimitInformation, &limits,
                                          sizeof(limits)))
        throw std::runtime_error("Cannot initialize PresentMon process lifetime");
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    startup.hStdOutput = output.get();
    startup.hStdError = output.get();
    startup.hStdInput = input.get();
    std::wstring command =
        L"\"" + config.presentmon.wstring() +
        L"\" --output_stdout --no_console_stats --v1_metrics --no_track_display --no_track_input "
        L"--no_track_gpu --session_name " +
        config.session_name + L" --stop_existing_session";
    PROCESS_INFORMATION info{};
    // Suspend first so kill-on-close containment is attached before any child code runs.
    if (!CreateProcessW(config.presentmon.c_str(), command.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr,
                        config.presentmon.parent_path().c_str(), &startup, &info))
        throw std::runtime_error("Cannot start PresentMon");
    process_.reset(info.hProcess);
    Handle thread(info.hThread);
    if (!AssignProcessToJobObject(job_.get(), process_.get())) {
        TerminateProcess(process_.get(), 1);
        throw std::runtime_error("Cannot contain PresentMon process");
    }
    if (ResumeThread(thread.get()) == static_cast<DWORD>(-1))
        throw std::runtime_error("Cannot resume PresentMon");
    output.reset();
    reader_ = std::thread([this, handler = std::move(handler)] { read_output(handler); });
}

PresentMonProcess::~PresentMonProcess() {
    // Closing the job terminates its child, unblocking ReadFile before joining.
    job_.reset();
    if (process_)
        WaitForSingleObject(process_.get(), 3000);
    if (reader_.joinable())
        reader_.join();
}

bool PresentMonProcess::running(std::uint64_t wait_ms) {
    if (wait_ms >= INFINITE)
        throw std::invalid_argument("Invalid PresentMon wait interval");
    const auto status = WaitForSingleObject(process_.get(), static_cast<DWORD>(wait_ms));
    if (status != WAIT_TIMEOUT && status != WAIT_OBJECT_0)
        throw std::runtime_error("Cannot wait for PresentMon");
    if (status == WAIT_OBJECT_0 && reader_.joinable())
        reader_.join();
    // Join before checking: a short-lived child can fail while the main loop never ran.
    if (reader_failed_)
        throw std::runtime_error("PresentMon output processing failed");
    if (status == WAIT_TIMEOUT)
        return true;
    DWORD exit_code = 0;
    if (!GetExitCodeProcess(process_.get(), &exit_code) || exit_code != 0)
        throw std::runtime_error("PresentMon exited unsuccessfully; check capture permissions");
    return false;
}

void PresentMonProcess::read_output(const LineHandler &handler) noexcept {
    try {
        std::array<char, 8192> buffer{};
        DWORD count = 0;
        std::string pending;
        constexpr std::size_t maximum_line = 65536;
        for (;;) {
            if (!ReadFile(pipe_.get(), buffer.data(), static_cast<DWORD>(buffer.size()), &count,
                          nullptr)) {
                if (GetLastError() != ERROR_BROKEN_PIPE)
                    throw std::runtime_error("Cannot read PresentMon output");
                break;
            }
            if (count == 0)
                break;
            pending.append(buffer.data(), count);
            std::size_t end = 0;
            while ((end = pending.find('\n')) != std::string::npos) {
                // Check complete lines too: a newline must not bypass the memory bound.
                if (end > maximum_line)
                    throw std::runtime_error("PresentMon output line exceeded limit");
                handler(std::string_view(pending.data(), end));
                pending.erase(0, end + 1);
            }
            if (pending.size() > maximum_line)
                throw std::runtime_error("PresentMon output line exceeded limit");
        }
        if (!pending.empty())
            handler(pending);
    } catch (...) {
        // CSV and child stderr can contain local process data: retain only a failure flag.
        reader_failed_ = true;
    }
}

} // namespace phone_monitor::windows
