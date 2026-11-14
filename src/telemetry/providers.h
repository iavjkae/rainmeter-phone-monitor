// SPDX-License-Identifier: MIT
#pragma once

#include "telemetry/model.h"

#include <cstdint>
#include <filesystem>
#include <memory>

namespace phone_monitor {

class GpuProvider {
  public:
    virtual ~GpuProvider() = default;
    virtual GpuSample sample() = 0;
};

class CpuProvider {
  public:
    virtual ~CpuProvider() = default;
    virtual CpuIdentity identity() = 0;
};

class FpsSource {
  public:
    virtual ~FpsSource() = default;
    virtual FpsSample sample(std::uint64_t now_ms) = 0;
};

struct GpuSelection {
    unsigned index = 0;
};

std::unique_ptr<GpuProvider> make_gpu_provider(GpuSelection selection);
std::unique_ptr<CpuProvider> make_cpu_provider();
std::unique_ptr<FpsSource> make_file_fps_source(std::filesystem::path path,
                                                std::uint64_t freshness_ms);

} // namespace phone_monitor
