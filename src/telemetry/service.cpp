// SPDX-License-Identifier: MIT
#include "telemetry/service.h"

#include <stdexcept>
#include <utility>

namespace phone_monitor {

TelemetryService::TelemetryService(std::unique_ptr<GpuProvider> gpu,
                                   std::unique_ptr<CpuProvider> cpu, std::unique_ptr<FpsSource> fps)
    : gpu_(std::move(gpu)), fps_(std::move(fps)) {
    if (!gpu_ || !cpu || !fps_)
        throw std::invalid_argument("Telemetry providers must not be null");
    // Identity is stable for this service lifetime; polling never repeats WMI discovery.
    cpu_ = cpu->identity();
}

TelemetrySnapshot TelemetryService::sample(std::uint64_t now_ms) {
    return {gpu_->sample(), cpu_, fps_->sample(now_ms)};
}

void TelemetryService::set_gpu_provider(std::unique_ptr<GpuProvider> provider) {
    if (!provider)
        throw std::invalid_argument("GPU provider must not be null");
    gpu_ = std::move(provider);
}

void TelemetryService::set_fps_source(std::unique_ptr<FpsSource> source) {
    if (!source)
        throw std::invalid_argument("FPS source must not be null");
    fps_ = std::move(source);
}

} // namespace phone_monitor
