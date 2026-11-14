# Contributing

Use `mise install` and `mise run setup` for the project toolchain and locked development dependencies.
Windows runtime builds need an installed C++17 compiler; MSVC and MinGW are supported.
Linux can build and execute the platform-independent core using
`-DPHONE_MONITOR_BUILD_RUNTIME=OFF`.

- Keep the C ABI in `include/phone_monitor/rainmeter_plugin.h` only. Do not export internal classes.
- Keep OS and driver APIs inside `src/platform/windows`; inject providers into the telemetry service.
- Keep process rules in configuration, never in compiled application lists.
- Edit palettes, templates, or `tools/skin_builder`, then run `mise run generate`.
- Run `mise run format` and `mise run check`. Warnings are errors; use synthetic fixtures in tests.
- Run hardware or ETW checks only explicitly and locally, with all values suppressed.
- Review every file before staging. A successful privacy check does not authorize publication.

Do not add screenshots, personal hardware specifications, local configurations, credentials,
absolute developer paths, raw CSV, state files, or logs to source control or CI artifacts.
`mise.local.toml`, FPS local overrides, build products, and runtime state remain local.
Packaging uses an allowlist and rejects an existing destination to avoid carrying private files forward.

Contributions are MIT-licensed. See [PRIVACY.md](PRIVACY.md) for data-handling boundaries.
