// SPDX-License-Identifier: MIT
#pragma once

#include "telemetry/providers.h"

namespace phone_monitor {

class TelemetryService {
  public:
    TelemetryService(std::unique_ptr<GpuProvider> gpu, std::unique_ptr<CpuProvider> cpu,
                     std::unique_ptr<FpsSource> fps);
    TelemetrySnapshot sample(std::uint64_t now_ms);
    void set_gpu_provider(std::unique_ptr<GpuProvider> provider);
    void set_fps_source(std::unique_ptr<FpsSource> source);

  private:
    std::unique_ptr<GpuProvider> gpu_;
    std::unique_ptr<FpsSource> fps_;
    CpuIdentity cpu_;
};

} // namespace phone_monitor
