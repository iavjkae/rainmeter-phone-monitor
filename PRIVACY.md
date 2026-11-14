# Privacy and publication boundaries

- Hardware values are read only for the local monitoring UI; there is no upload endpoint or telemetry.
- Default tests use synthetic providers and frame events. ABI smoke checks do not initialize sensors.
- An explicit local `--hardware` check reports pass/fail only. Failure messages must not include payloads.
- PresentMon output is parsed in memory. Raw CSV, process lists and hardware diagnostics are not logged.
- The local FPS state file contains the selected application's temporary display data. It is excluded
  from Git and packages. A scoped publisher resets it before capture and on normal or exceptional
  exit. Cleanup is best-effort if the filesystem refuses writes; readers also expire stale records.
- No screenshots, media, personal system descriptions or local environment files are published.
- Source review checks flag media, personal paths, recognizable machine specifications and secrets;
  they print file locations, never matching values. These checks are an aid, not a publication approval.
- CMake diagnostic directories, runtime state, temporary files, crash dumps and raw logs are ignored
  and rejected by source checks even if force-tracked. Keep those diagnostics local.
- Packaging copies only known source/config resources and built binaries. Local override files and
  runtime output are never included. Existing output directories must not be reused.

Changing a working tree does not remove earlier disclosures from remote repositories, Git history,
previous CI logs, artifacts, caches or clones. Any purge/history rewrite is a separate operation that
requires the repository owner's approval; this project performs no automatic remote cleanup.
