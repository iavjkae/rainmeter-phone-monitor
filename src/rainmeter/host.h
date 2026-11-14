// SPDX-License-Identifier: MIT
#pragma once

#include <string>

namespace phone_monitor::rainmeter {

class Host {
  public:
    explicit Host(void *context);
    std::wstring read_string(const wchar_t *key, const wchar_t *fallback) const;
    double read_number(const wchar_t *key, double fallback) const;
    void report_error(const wchar_t *message) const noexcept;

  private:
    void *context_;
};

} // namespace phone_monitor::rainmeter
