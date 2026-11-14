# Architecture

| Boundary | Responsibility | Does not own |
| --- | --- | --- |
| Public C ABI | Rainmeter's five lifecycle functions; catches exceptions at the host boundary | Drivers, frame calculations, process rules |
| Rainmeter adapter | Reads options, composes a service, maps typed data to the existing 13-field presentation contract | NVML ABI or WMI implementation |
| Telemetry service | Combines injected GPU, CPU and FPS providers; caches CPU identity | Host APIs, stdout, network |
| Windows providers | WMI/NVML queries, per-call status checking, cached identity and resource lifetime | UI formatting or process policy |
| FPS core | Validated configuration, CSV decoding, wildcard rules, bounded frame history and state protocol | Windows handles, ETW or global state |
| Windows process adapter | Owns child job, handles, pipe reader and atomic local state replacement | Application name lists or frame statistics |
| Skin generator | Combines palette configuration, templates and composable layout regions | Host inspection or screenshots |

Dependencies point toward the typed core. `GpuProvider`, `CpuProvider` and `FpsSource` can be replaced
by test implementations without loading a driver. CPU identity is read through Windows WMI once;
the NVML provider keeps its ABI and pointers private and only samples changing metrics thereafter.
Unavailable metrics use optionals; numeric compatibility placeholders are introduced by the adapter.

FPS configuration is loaded before capture starts, with optional local overrides. Unknown keys and
invalid values fail explicitly. Frame history is bounded by time, number of streams and sample count.
The reader thread and application thread share only a guarded history instance. Child teardown closes
the kill-on-close job before joining the reader, so a blocked pipe can drain and terminate.
The sampler uses system uptime and ignores frames appended after its captured timestamp. FPS uses
the recent mean frame duration; 1% Low uses the mean of the slowest `ceil(1%)` frames in the history.

The Windows process adapter distinguishes successful termination from child or reader failure.
It drains and joins the reader before declaring success, including when the child exits before the
first poll. Callbacks borrow each line only during the call; no raw child output is persisted.
The state publisher owns invalidation on startup and scope exit, with bounded retries for sharing
conflicts during atomic file replacement. Exception cleanup preserves the original error and is
best-effort if the filesystem remains unavailable; the plugin's freshness check is the fallback.

Tests cover provider injection/caching, unavailable readings, reordered and malformed CSV, wildcard
precedence, local overrides, stale state, frame expiry, foreground selection, FPS and 1% Low.
Headless Windows fixtures cover child exit failures, callback errors, output limits, final partial
lines, kill-on-close containment and state replacement/cleanup. They require no ETW or elevation.
Public ABI checks and synthetic tests do not access real hardware. Runtime game compatibility remains
unverified and is documented separately from these contract checks.
