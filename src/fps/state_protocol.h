// SPDX-License-Identifier: MIT
#pragma once

#include "telemetry/model.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace phone_monitor::fps {

struct StateRecord {
    // System uptime (GetTickCount64), shared by collector and plugin. Zero invalidates
    // the record; it is deliberately neither a Unix timestamp nor a persisted session id.
    std::uint64_t timestamp_ms = 0;
    FpsSample sample;
};

std::optional<StateRecord> parse_state(std::string_view line);
std::string serialize_state(const StateRecord &record);
bool is_fresh(std::uint64_t timestamp_ms, std::uint64_t now_ms, std::uint64_t freshness_ms);

} // namespace phone_monitor::fps
