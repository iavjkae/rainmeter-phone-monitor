// SPDX-License-Identifier: MIT
// Headless stand-in for PresentMon: no ETW, drivers, elevation or user applications.
#include <windows.h>

#include <string>
#include <string_view>

namespace {
bool emit(const std::string &text) {
    DWORD written = 0;
    return WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), text.data(), static_cast<DWORD>(text.size()),
                     &written, nullptr) &&
           written == text.size();
}
} // namespace

int main(int argc, char **argv) {
    std::string_view mode;
    for (int i = 1; i + 1 < argc; ++i)
        if (std::string_view(argv[i]) == "--session_name")
            mode = argv[i + 1];
    if (mode == "failure")
        return 7;
    if (mode == "oversized")
        return emit(std::string(65537, 'x') + '\n') ? 0 : 8;
    if (mode == "limit")
        return emit(std::string(65536, 'x') + '\n') ? 0 : 8;
    if (mode == "tail")
        return emit("synthetic tail") ? 0 : 8;
    if (mode == "lifetime") {
        if (!emit(std::to_string(GetCurrentProcessId()) + '\n'))
            return 8;
        Sleep(INFINITE);
    }
    return emit("synthetic row\n") ? 0 : 8;
}
