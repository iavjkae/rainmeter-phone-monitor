// SPDX-License-Identifier: MIT
#include "platform/windows/bridge_environment.h"

#include <stdexcept>
#include <vector>

namespace phone_monitor::windows {

SingleInstance::SingleInstance()
    : mutex_(CreateMutexW(nullptr, FALSE, L"Local\\RainmeterPhoneFPSBridge")),
      acquired_(mutex_ && GetLastError() != ERROR_ALREADY_EXISTS) {
    if (!mutex_)
        throw std::runtime_error("Cannot initialize FPS single-instance guard");
}

std::filesystem::path executable_directory() {
    std::vector<wchar_t> buffer(32768);
    const DWORD length =
        GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size())
        throw std::runtime_error("Cannot locate FPS application directory");
    return std::filesystem::path(buffer.data()).parent_path();
}

std::uint32_t foreground_process_id() {
    DWORD pid = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &pid);
    return pid;
}

bool panel_is_loaded(const std::filesystem::path &skin_directory) {
    struct Search {
        std::filesystem::path directory;
        bool found = false;
        bool failed = false;
    } search{skin_directory};
    const auto enumerated = EnumWindows(
        [](HWND window, LPARAM data) -> BOOL {
            auto &state = *reinterpret_cast<Search *>(data);
            // C++ exceptions must not unwind through the Windows callback boundary.
            try {
                wchar_t class_name[64]{};
                if (!GetClassNameW(window, class_name, 64) ||
                    std::wstring_view(class_name) != L"RainmeterMeterWindow")
                    return TRUE;
                const int length = GetWindowTextLengthW(window);
                if (length <= 0)
                    return TRUE;
                std::wstring title(static_cast<std::size_t>(length) + 1, L'\0');
                GetWindowTextW(window, title.data(), length + 1);
                title.resize(static_cast<std::size_t>(length));
                std::error_code error;
                if (std::filesystem::equivalent(std::filesystem::path(title).parent_path(),
                                                state.directory, error) &&
                    !error)
                    state.found = true;
                return state.found ? FALSE : TRUE;
            } catch (...) {
                state.failed = true;
                return FALSE;
            }
        },
        reinterpret_cast<LPARAM>(&search));
    if (search.failed || (!enumerated && !search.found))
        throw std::runtime_error("Cannot inspect Rainmeter panel lifetime");
    return search.found;
}

} // namespace phone_monitor::windows
