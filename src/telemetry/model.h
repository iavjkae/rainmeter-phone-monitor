// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace phone_monitor {

enum class SensorStatus { available, unsupported, unavailable };

struct GpuSample {
    SensorStatus status = SensorStatus::unavailable;
    std::wstring name;
    // Absence means unavailable/unsupported, distinct from a valid zero reading.
    // Only the Rainmeter formatter may substitute compatibility placeholders.
    std::optional<double> utilization_percent;
    std::optional<double> power_watts;
    std::optional<double> temperature_celsius;
    std::optional<std::uint64_t> memory_used_bytes;
    std::optional<std::uint64_t> memory_total_bytes;
};

struct CpuIdentity {
    SensorStatus status = SensorStatus::unavailable;
    std::wstring name;
};

struct FpsSample {
    double frames_per_second = 0;
    double one_percent_low = 0;
    double frame_time_ms = 0;
    std::string process_name;
    bool collector_active = false;
};

struct TelemetrySnapshot {
    GpuSample gpu;
    CpuIdentity cpu;
    FpsSample fps;
};

} // namespace phone_monitor
