// SPDX-License-Identifier: MIT
#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <sstream>
#include <string>

namespace {
using Device = void *;
struct Utilization {
    unsigned gpu, memory;
};
struct Memory {
    unsigned long long total, free, used;
};
using ReadString = LPCWSTR(__cdecl *)(void *, LPCWSTR, LPCWSTR, BOOL);

template <typename T> T symbol(HMODULE module, const char *name) {
    const auto proc = module ? GetProcAddress(module, name) : nullptr;
    T result = nullptr;
    static_assert(sizeof(result) == sizeof(proc));
    std::memcpy(&result, &proc, sizeof(result));
    return result;
}

class Nvidia {
  public:
    Nvidia() {
        module_ = LoadLibraryExW(L"nvml.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!module_)
            return;
        const auto init = symbol<int (*)()>(module_, "nvmlInit_v2");
        shutdown_ = symbol<int (*)()>(module_, "nvmlShutdown");
        const auto device =
            symbol<int (*)(unsigned, Device *)>(module_, "nvmlDeviceGetHandleByIndex_v2");
        if (init && device && init() == 0) {
            initialized_ = true;
            device(0, &device_);
        }
        utilization =
            symbol<int (*)(Device, Utilization *)>(module_, "nvmlDeviceGetUtilizationRates");
        memory = symbol<int (*)(Device, Memory *)>(module_, "nvmlDeviceGetMemoryInfo");
        power = symbol<int (*)(Device, unsigned *)>(module_, "nvmlDeviceGetPowerUsage");
        temperature =
            symbol<int (*)(Device, unsigned, unsigned *)>(module_, "nvmlDeviceGetTemperature");
        name = symbol<int (*)(Device, char *, unsigned)>(module_, "nvmlDeviceGetName");
    }
    ~Nvidia() {
        if (initialized_ && shutdown_)
            shutdown_();
        if (module_)
            FreeLibrary(module_);
    }
    Nvidia(const Nvidia &) = delete;
    Nvidia &operator=(const Nvidia &) = delete;
    Device device() const {
        return device_;
    }
    int (*utilization)(Device, Utilization *) = nullptr;
    int (*memory)(Device, Memory *) = nullptr;
    int (*power)(Device, unsigned *) = nullptr;
    int (*temperature)(Device, unsigned, unsigned *) = nullptr;
    int (*name)(Device, char *, unsigned) = nullptr;

  private:
    HMODULE module_ = nullptr;
    Device device_ = nullptr;
    bool initialized_ = false;
    int (*shutdown_)() = nullptr;
};

std::wstring wide(const std::string &value) {
    if (value.empty())
        return {};
    const auto n = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (n <= 1)
        return {};
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, out.data(), n);
    out.pop_back();
    return out;
}

std::wstring cpuName() {
    wchar_t name[256] = L"CPU";
    DWORD bytes = sizeof(name);
    RegGetValueW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                 L"ProcessorNameString", RRF_RT_REG_SZ, nullptr, name, &bytes);
    std::wstring result(name);
    const auto first = result.find_first_not_of(L' ');
    if (first != std::wstring::npos)
        result.erase(0, first);
    const auto last = result.find_last_not_of(L' ');
    if (last != std::wstring::npos)
        result.erase(last + 1);
    std::replace(result.begin(), result.end(), L'|', L' ');
    return result;
}

struct State {
    Nvidia nvidia;
    std::wstring file;
    std::wstring cpu = cpuName();
    std::wstring text;
};
} // namespace

extern "C" __declspec(dllexport) void Initialize(void **data, void *) {
    *data = new State();
}

extern "C" __declspec(dllexport) void Reload(void *raw, void *rm, double *maximum) {
    *maximum = 100;
    // Resolve the Rainmeter host API without an SDK or import library.
    const auto read = symbol<ReadString>(GetModuleHandleW(L"Rainmeter.dll"), "RmReadString");
    if (read)
        static_cast<State *>(raw)->file = read(rm, L"FPSStateFile", L"", TRUE);
}

extern "C" __declspec(dllexport) double Update(void *raw) {
    auto &s = *static_cast<State *>(raw);
    auto &nv = s.nvidia;
    const auto device = nv.device();
    Utilization utilization{};
    Memory memory{};
    unsigned power = 0, temperature = 0;
    char name[128] = "GPU unavailable";
    if (device) {
        if (nv.utilization)
            nv.utilization(device, &utilization);
        if (nv.memory)
            nv.memory(device, &memory);
        if (nv.power)
            nv.power(device, &power);
        if (nv.temperature)
            nv.temperature(device, 0, &temperature);
        if (nv.name)
            nv.name(device, name, sizeof(name));
    }

    double fps = 0, low = 0, frameTime = 0;
    unsigned long long stamp = 0;
    std::string game, line;
    std::ifstream input(s.file.c_str());
    std::getline(input, line);
    std::replace(line.begin(), line.end(), '|', '\t');
    std::istringstream fields(line);
    if (fields >> stamp >> fps >> low >> frameTime) {
        std::getline(fields, game);
        if (!game.empty() && game.front() == '\t')
            game.erase(0, 1);
    }
    const auto now = GetTickCount64();
    const bool live = stamp && now >= stamp && now - stamp < 4000;
    if (!live) {
        fps = low = frameTime = 0;
        game.clear();
    }
    std::replace(game.begin(), game.end(), '|', '_');
    const auto gameName = game.empty() ? L"\u7b49\u5f85\u6e38\u620f" : wide(game);
    const auto gpuName = wide(name);
    wchar_t out[2048];
    std::swprintf(out, 2048, L"%ls|%u|%.1f|%.2f|%.1f|%u|%.1f|%.1f|%.2f|%ls|%u|%u|%ls",
                  gpuName.c_str(), utilization.gpu, power / 1000.0, memory.used / 1073741824.0,
                  memory.total / 1073741824.0, temperature, fps, low, frameTime, gameName.c_str(),
                  live ? 1u : 0u, device ? 1u : 0u, s.cpu.c_str());
    s.text = out;
    return utilization.gpu;
}

extern "C" __declspec(dllexport) LPCWSTR GetString(void *raw) {
    return static_cast<State *>(raw)->text.c_str();
}

extern "C" __declspec(dllexport) void Finalize(void *raw) {
    delete static_cast<State *>(raw);
}
