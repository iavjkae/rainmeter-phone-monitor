// SPDX-License-Identifier: MIT
#pragma once

#include "platform/windows/handle.h"

#include <filesystem>

namespace phone_monitor::windows {

std::filesystem::path executable_directory();
std::uint32_t foreground_process_id();
bool panel_is_loaded(const std::filesystem::path &skin_directory);

class SingleInstance {
  public:
    SingleInstance();
    bool acquired() const noexcept {
        return acquired_;
    }

  private:
    Handle mutex_;
    bool acquired_;
};

} // namespace phone_monitor::windows
