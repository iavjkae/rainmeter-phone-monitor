// SPDX-License-Identifier: MIT
#include "platform/windows/text.h"

#include <windows.h>

#include <limits>

namespace phone_monitor::windows {

std::wstring from_utf8(std::string_view value) {
    if (value.empty() || value.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return {};
    const auto size = static_cast<int>(value.size());
    const int length =
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), size, nullptr, 0);
    if (length <= 0)
        return {};
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), size, result.data(), length);
    return result;
}

std::string to_utf8(std::wstring_view value) {
    if (value.empty() || value.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
        return {};
    const auto size = static_cast<int>(value.size());
    const int length =
        WideCharToMultiByte(CP_UTF8, 0, value.data(), size, nullptr, 0, nullptr, nullptr);
    if (length <= 0)
        return {};
    std::string result(static_cast<size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), size, result.data(), length, nullptr, nullptr);
    return result;
}

} // namespace phone_monitor::windows
