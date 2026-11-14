// SPDX-License-Identifier: MIT
#include "platform/windows/text.h"
#include "rainmeter/format.h"

#include <algorithm>
#include <iomanip>
#include <locale>
#include <sstream>

namespace phone_monitor::rainmeter {
namespace {

std::wstring field(std::wstring value, const wchar_t *fallback) {
    for (auto &ch : value)
        if (ch == L'|' || ch == L'\r' || ch == L'\n')
            ch = L' ';
    return value.empty() ? fallback : value;
}

} // namespace

std::wstring format_snapshot(const TelemetrySnapshot &snapshot) {
    constexpr double gibibyte = 1073741824.0;
    const auto &gpu = snapshot.gpu;
    const auto &fps = snapshot.fps;
    // Zero numeric placeholders preserve the skin contract; availability is a separate flag.
    std::wostringstream out;
    out.imbue(std::locale::classic());
    out << field(gpu.name, L"GPU unavailable") << '|' << std::fixed << std::setprecision(0)
        << gpu.utilization_percent.value_or(0) << '|' << std::setprecision(1)
        << gpu.power_watts.value_or(0) << '|' << std::setprecision(2)
        << gpu.memory_used_bytes.value_or(0) / gibibyte << '|' << std::setprecision(1)
        << gpu.memory_total_bytes.value_or(0) / gibibyte << '|' << std::setprecision(0)
        << gpu.temperature_celsius.value_or(0) << '|' << std::setprecision(1)
        << fps.frames_per_second << '|' << fps.one_percent_low << '|' << std::setprecision(2)
        << fps.frame_time_ms << '|'
        << field(windows::from_utf8(fps.process_name), L"\u7b49\u5f85\u6e38\u620f") << '|'
        << (fps.collector_active ? 1 : 0) << '|' << (gpu.status == SensorStatus::available ? 1 : 0)
        << '|' << field(snapshot.cpu.name, L"CPU unavailable");
    return out.str();
}

} // namespace phone_monitor::rainmeter
