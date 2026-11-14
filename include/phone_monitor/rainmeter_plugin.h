// SPDX-License-Identifier: MIT
#pragma once

// The complete Rainmeter C ABI. Domain types and platform APIs stay private.
// Initialize transfers one instance to Rainmeter; Finalize releases that instance.
// Calls for an instance are serialized by the host. GetString returns borrowed storage,
// valid until the next Update or Finalize; the host must neither modify nor free it.
#if defined(_WIN32)
#define PHONE_MONITOR_EXPORT extern "C" __declspec(dllexport)
#else
#define PHONE_MONITOR_EXPORT extern "C"
#endif

PHONE_MONITOR_EXPORT void Initialize(void **data, void *host);
PHONE_MONITOR_EXPORT void Reload(void *data, void *host, double *maximum);
PHONE_MONITOR_EXPORT double Update(void *data);
PHONE_MONITOR_EXPORT const wchar_t *GetString(void *data);
PHONE_MONITOR_EXPORT void Finalize(void *data);
