// SPDX-License-Identifier: MIT
#pragma once

#include <windows.h>

#include <utility>

namespace phone_monitor::windows {

class Handle {
  public:
    explicit Handle(HANDLE value = nullptr) noexcept : value_(value) {}
    ~Handle() {
        reset();
    }
    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
    Handle(Handle &&other) noexcept : value_(std::exchange(other.value_, nullptr)) {}
    Handle &operator=(Handle &&other) noexcept {
        if (this != &other) {
            reset();
            value_ = std::exchange(other.value_, nullptr);
        }
        return *this;
    }
    HANDLE get() const noexcept {
        return value_;
    }
    explicit operator bool() const noexcept {
        return value_ && value_ != INVALID_HANDLE_VALUE;
    }
    void reset(HANDLE value = nullptr) noexcept {
        if (*this)
            CloseHandle(value_);
        value_ = value;
    }

  private:
    HANDLE value_;
};

} // namespace phone_monitor::windows
