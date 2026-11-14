// SPDX-License-Identifier: MIT
#pragma once

#include "telemetry/model.h"

namespace phone_monitor::rainmeter {
// The stable 13-field Rainmeter presentation format is owned by the adapter.
std::wstring format_snapshot(const TelemetrySnapshot &snapshot);
} // namespace phone_monitor::rainmeter
