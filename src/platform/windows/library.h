// SPDX-License-Identifier: MIT
#pragma once

#include <windows.h>

#include <cstring>

namespace phone_monitor::windows {

class SystemLibrary {
  public:
    explicit SystemLibrary(const wchar_t *name)
        : handle_(LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32)) {}
    ~SystemLibrary() {
        if (handle_)
            FreeLibrary(handle_);
    }
    SystemLibrary(const SystemLibrary &) = delete;
    SystemLibrary &operator=(const SystemLibrary &) = delete;
    explicit operator bool() const {
        return handle_ != nullptr;
    }

    template <typename Function> Function symbol(const char *name) const {
        return resolve<Function>(handle_, name);
    }

    template <typename Function> static Function resolve(HMODULE handle, const char *name) {
        const auto address = handle ? GetProcAddress(handle, name) : nullptr;
        Function function = nullptr;
        static_assert(sizeof(function) == sizeof(address));
        std::memcpy(&function, &address, sizeof(function));
        return function;
    }

  private:
    HMODULE handle_;
};

} // namespace phone_monitor::windows
