// SPDX-License-Identifier: MIT
#include "telemetry/providers.h"

#include <windows.h>

#include <optional>
#include <utility>
#include <wbemidl.h>

namespace phone_monitor {
namespace {

template <typename T> class ComPointer {
  public:
    ~ComPointer() {
        if (value_)
            value_->Release();
    }
    ComPointer() = default;
    ComPointer(const ComPointer &) = delete;
    ComPointer &operator=(const ComPointer &) = delete;
    T **out() {
        return &value_;
    }
    T *operator->() const {
        return value_;
    }
    T *get() const {
        return value_;
    }

  private:
    T *value_ = nullptr;
};

class ComApartment {
  public:
    ComApartment() : result_(CoInitializeEx(nullptr, COINIT_MULTITHREADED)) {}
    ~ComApartment() {
        // RPC_E_CHANGED_MODE borrows the host's existing apartment; only our successful
        // CoInitializeEx call (including S_FALSE) is balanced with CoUninitialize.
        if (SUCCEEDED(result_))
            CoUninitialize();
    }
    bool usable() const {
        return SUCCEEDED(result_) || result_ == RPC_E_CHANGED_MODE;
    }

  private:
    HRESULT result_;
};

class AutomationString {
  public:
    explicit AutomationString(const wchar_t *value) : value_(SysAllocString(value)) {}
    ~AutomationString() {
        SysFreeString(value_);
    }
    AutomationString(const AutomationString &) = delete;
    AutomationString &operator=(const AutomationString &) = delete;
    BSTR get() const {
        return value_;
    }

  private:
    BSTR value_;
};

class AutomationValue {
  public:
    AutomationValue() {
        VariantInit(&value);
    }
    ~AutomationValue() {
        VariantClear(&value);
    }
    AutomationValue(const AutomationValue &) = delete;
    AutomationValue &operator=(const AutomationValue &) = delete;
    VARIANT value;
};

std::optional<std::wstring> query_processor_name() {
    ComApartment apartment;
    if (!apartment.usable())
        return std::nullopt;
    ComPointer<IWbemLocator> locator;
    if (FAILED(CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
                                reinterpret_cast<void **>(locator.out()))))
        return std::nullopt;
    const AutomationString resource(L"ROOT\\CIMV2"), language(L"WQL");
    const AutomationString query(L"SELECT Name FROM Win32_Processor");
    if (!resource.get() || !language.get() || !query.get())
        return std::nullopt;
    ComPointer<IWbemServices> service;
    if (FAILED(locator->ConnectServer(resource.get(), nullptr, nullptr, nullptr, 0, nullptr,
                                      nullptr, service.out())) ||
        FAILED(CoSetProxyBlanket(service.get(), RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                                 RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr,
                                 EOAC_NONE)))
        return std::nullopt;
    ComPointer<IEnumWbemClassObject> results;
    if (FAILED(service->ExecQuery(language.get(), query.get(),
                                  WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
                                  results.out())))
        return std::nullopt;
    ComPointer<IWbemClassObject> item;
    ULONG returned = 0;
    if (results->Next(1000, 1, item.out(), &returned) != WBEM_S_NO_ERROR || returned != 1)
        return std::nullopt;
    // WMI owns the returned BSTR through its VARIANT, even if copying the name throws.
    AutomationValue property;
    const auto status = item->Get(L"Name", 0, &property.value, nullptr, nullptr);
    const auto &value = property.value;
    std::optional<std::wstring> result;
    if (SUCCEEDED(status) && value.vt == VT_BSTR && value.bstrVal) {
        std::wstring name(value.bstrVal, SysStringLen(value.bstrVal));
        const auto first = name.find_first_not_of(L" \t\r\n");
        if (first != std::wstring::npos)
            result = name.substr(first, name.find_last_not_of(L" \t\r\n") - first + 1);
    }
    return result;
}

class WindowsCpuProvider final : public CpuProvider {
  public:
    WindowsCpuProvider() {
        if (auto name = query_processor_name())
            identity_ = {SensorStatus::available, std::move(*name)};
    }
    CpuIdentity identity() override {
        return identity_;
    }

  private:
    CpuIdentity identity_;
};

} // namespace

std::unique_ptr<CpuProvider> make_cpu_provider() {
    return std::make_unique<WindowsCpuProvider>();
}

} // namespace phone_monitor
