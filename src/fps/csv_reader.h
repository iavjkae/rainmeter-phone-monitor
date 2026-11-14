// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace phone_monitor::fps {

struct PresentEvent {
    std::uint32_t process_id = 0;
    std::string process_name;
    std::string swap_chain;
    double frame_time_ms = 0;
};

class CsvReader {
  public:
    std::optional<PresentEvent> consume(std::string_view line);
    bool has_header() const noexcept {
        return columns_.has_value();
    }

  private:
    std::optional<std::array<std::size_t, 4>> columns_;
};

} // namespace phone_monitor::fps
