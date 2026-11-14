// SPDX-License-Identifier: MIT
#include "rainmeter/format.h"
#include "rainmeter/plugin.h"

#include <windows.h>

#include <cmath>
#include <limits>

namespace phone_monitor::rainmeter {

// Construction and export smoke checks are sensor-free; Reload composes real providers.
Plugin::Plugin() : text_(format_snapshot({})) {}

void Plugin::reload(const Host &host) {
    const double index = host.read_number(L"GPUIndex", 0);
    if (!std::isfinite(index) || index < 0 || index > std::numeric_limits<unsigned>::max() ||
        index != std::floor(index)) {
        host.report_error(L"PhoneMonitor: GPUIndex must be a nonnegative integer.");
        return;
    }
    const auto selected = static_cast<unsigned>(index);
    const auto freshness = host.read_number(L"FPSFreshnessMs", 4000);
    const auto valid_freshness = std::isfinite(freshness) && freshness >= 1 && freshness <= 60000;
    auto source =
        make_file_fps_source(host.read_string(L"FPSStateFile", L""),
                             valid_freshness ? static_cast<std::uint64_t>(freshness) : 4000);
    if (!service_) {
        service_ = std::make_unique<TelemetryService>(make_gpu_provider({selected}),
                                                      make_cpu_provider(), std::move(source));
    } else {
        if (selected != gpu_index_)
            service_->set_gpu_provider(make_gpu_provider({selected}));
        service_->set_fps_source(std::move(source));
    }
    gpu_index_ = selected;
}

double Plugin::update() {
    if (!service_)
        return 0;
    const auto snapshot = service_->sample(GetTickCount64());
    text_ = format_snapshot(snapshot);
    return snapshot.gpu.utilization_percent.value_or(0);
}

const wchar_t *Plugin::text() const noexcept {
    return text_.c_str();
}

} // namespace phone_monitor::rainmeter
