// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <string_view>

namespace phone_monitor::windows {
std::wstring from_utf8(std::string_view value);
std::string to_utf8(std::wstring_view value);
} // namespace phone_monitor::windows
