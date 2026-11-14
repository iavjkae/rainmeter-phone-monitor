// SPDX-License-Identifier: MIT
#pragma once

#include "rainmeter/host.h"
#include "telemetry/service.h"

namespace phone_monitor::rainmeter {

class Plugin {
  public:
    Plugin();
    void reload(const Host &host);
    double update();
    const wchar_t *text() const noexcept;

  private:
    unsigned gpu_index_ = 0;
    std::unique_ptr<TelemetryService> service_;
    std::wstring text_;
};

} // namespace phone_monitor::rainmeter
