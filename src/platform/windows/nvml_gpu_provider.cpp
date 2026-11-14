// SPDX-License-Identifier: MIT
#include "platform/windows/library.h"
#include "platform/windows/text.h"
#include "telemetry/providers.h"

#include <array>

namespace phone_monitor {
namespace {

// Only this translation unit knows the NVML ABI; no driver handles escape the provider.
using Device = void *;
struct NvmlUtilization {
    unsigned gpu, memory;
};
struct NvmlMemory {
    unsigned long long total, free, used;
};
constexpr int nvml_success = 0;

class NvmlSession {
  public:
    bool initialize(const windows::SystemLibrary &library) {
        const auto start = library.symbol<int (*)()>("nvmlInit_v2");
        shutdown_ = library.symbol<int (*)()>("nvmlShutdown");
        initialized_ = start && shutdown_ && start() == nvml_success;
        return initialized_;
    }
    ~NvmlSession() {
        if (initialized_)
            shutdown_();
    }
    NvmlSession() = default;
    NvmlSession(const NvmlSession &) = delete;
    NvmlSession &operator=(const NvmlSession &) = delete;

  private:
    int (*shutdown_)() = nullptr;
    bool initialized_ = false;
};

class NvmlGpuProvider final : public GpuProvider {
  public:
    explicit NvmlGpuProvider(GpuSelection selection) : library_(L"nvml.dll") {
        if (!library_) {
            status_ = SensorStatus::unsupported;
            return;
        }
        const auto select =
            library_.symbol<int (*)(unsigned, Device *)>("nvmlDeviceGetHandleByIndex_v2");
        if (!select || !session_.initialize(library_))
            return;
        if (select(selection.index, &device_) != nvml_success)
            return;
        utilization_ =
            library_.symbol<int (*)(Device, NvmlUtilization *)>("nvmlDeviceGetUtilizationRates");
        memory_ = library_.symbol<int (*)(Device, NvmlMemory *)>("nvmlDeviceGetMemoryInfo");
        power_ = library_.symbol<int (*)(Device, unsigned *)>("nvmlDeviceGetPowerUsage");
        temperature_ =
            library_.symbol<int (*)(Device, unsigned, unsigned *)>("nvmlDeviceGetTemperature");
        const auto read_name =
            library_.symbol<int (*)(Device, char *, unsigned)>("nvmlDeviceGetName");
        std::array<char, 256> buffer{};
        if (read_name && read_name(device_, buffer.data(), static_cast<unsigned>(buffer.size())) ==
                             nvml_success) {
            buffer.back() = '\0';
            name_ = windows::from_utf8(buffer.data());
        }
        status_ = SensorStatus::available;
    }

    GpuSample sample() override {
        GpuSample result;
        result.status = status_;
        result.name = name_;
        if (status_ != SensorStatus::available)
            return result;
        NvmlUtilization utilization{};
        NvmlMemory memory{};
        unsigned power = 0, temperature = 0;
        if (utilization_ && utilization_(device_, &utilization) == nvml_success)
            result.utilization_percent = utilization.gpu;
        if (memory_ && memory_(device_, &memory) == nvml_success) {
            result.memory_used_bytes = memory.used;
            result.memory_total_bytes = memory.total;
        }
        if (power_ && power_(device_, &power) == nvml_success)
            result.power_watts = power / 1000.0;
        if (temperature_ && temperature_(device_, 0, &temperature) == nvml_success)
            result.temperature_celsius = temperature;
        if (!result.utilization_percent && !result.memory_used_bytes && !result.power_watts &&
            !result.temperature_celsius)
            result.status = SensorStatus::unavailable;
        return result;
    }

  private:
    // Destruction order matters: session shutdown runs while the DLL and its function
    // pointers are still valid, even if name allocation throws in the constructor.
    windows::SystemLibrary library_;
    NvmlSession session_;
    Device device_ = nullptr;
    SensorStatus status_ = SensorStatus::unavailable;
    std::wstring name_;
    int (*utilization_)(Device, NvmlUtilization *) = nullptr;
    int (*memory_)(Device, NvmlMemory *) = nullptr;
    int (*power_)(Device, unsigned *) = nullptr;
    int (*temperature_)(Device, unsigned, unsigned *) = nullptr;
};

} // namespace

std::unique_ptr<GpuProvider> make_gpu_provider(GpuSelection selection) {
    return std::make_unique<NvmlGpuProvider>(selection);
}

} // namespace phone_monitor
