// SPDX-License-Identifier: MIT
#include "phone_monitor/rainmeter_plugin.h"
#include "rainmeter/plugin.h"

using phone_monitor::rainmeter::Host;
using phone_monitor::rainmeter::Plugin;

// Never let C++ exceptions cross Rainmeter's C ABI. Log only fixed messages so that
// filesystem exceptions, driver details and local readings do not escape to host logs.
void Initialize(void **data, void *host) {
    if (!data)
        return;
    *data = nullptr;
    try {
        *data = new Plugin();
    } catch (...) {
        Host(host).report_error(L"PhoneMonitor: initialization failed.");
    }
}

void Reload(void *data, void *host, double *maximum) {
    if (maximum)
        *maximum = 100;
    if (!data)
        return;
    try {
        static_cast<Plugin *>(data)->reload(Host(host));
    } catch (...) {
        Host(host).report_error(L"PhoneMonitor: configuration failed.");
    }
}

double Update(void *data) {
    if (!data)
        return 0;
    try {
        return static_cast<Plugin *>(data)->update();
    } catch (...) {
        return 0;
    }
}

const wchar_t *GetString(void *data) {
    return data ? static_cast<Plugin *>(data)->text() : L"";
}

void Finalize(void *data) {
    delete static_cast<Plugin *>(data);
}
