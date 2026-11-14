// SPDX-License-Identifier: MIT
#include "fps/application.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    try {
        return phone_monitor::fps::run_application();
    } catch (...) {
        // Generic failure only: never persist raw CSV, process names, or machine details.
        MessageBoxW(nullptr,
                    L"FPS capture could not start or stopped. Check configuration, PresentMon, and "
                    L"ETW permissions.",
                    L"Phone Monitor", MB_OK | MB_ICONERROR);
        return 1;
    }
}
