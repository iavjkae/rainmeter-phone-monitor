// SPDX-License-Identifier: MIT
#include "platform/windows/library.h"
#include "rainmeter/host.h"

namespace phone_monitor::rainmeter {

Host::Host(void *context) : context_(context) {}

std::wstring Host::read_string(const wchar_t *key, const wchar_t *fallback) const {
    using ReadString = const wchar_t *(__cdecl *)(void *, const wchar_t *, const wchar_t *, BOOL);
    const auto read = windows::SystemLibrary::resolve<ReadString>(
        GetModuleHandleW(L"Rainmeter.dll"), "RmReadString");
    const auto value = read ? read(context_, key, fallback, TRUE) : fallback;
    return value ? value : fallback;
}

double Host::read_number(const wchar_t *key, double fallback) const {
    using ReadFormula = double(__cdecl *)(void *, const wchar_t *, double);
    const auto read = windows::SystemLibrary::resolve<ReadFormula>(
        GetModuleHandleW(L"Rainmeter.dll"), "RmReadFormula");
    return read ? read(context_, key, fallback) : fallback;
}

void Host::report_error(const wchar_t *message) const noexcept {
    using Log = void(__cdecl *)(void *, int, const wchar_t *);
    const auto log =
        windows::SystemLibrary::resolve<Log>(GetModuleHandleW(L"Rainmeter.dll"), "RmLog");
    if (log)
        log(context_, 1, message);
}

} // namespace phone_monitor::rainmeter
