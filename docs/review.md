# Local review

Scope: the uncommitted refactor, including the Rainmeter ABI, providers, FPS pipeline, build tools,
package boundaries and privacy checks. This record contains no host inventory or captured readings.

| Priority | Confirmed finding | Resolution and evidence |
| --- | --- | --- |
| P1 | The pipe reader could append a frame newer than the sampler's timestamp, causing a valid stream to disappear or a future duration to affect 1% Low. | Sample only the eligible prefix; regression covers the exact timestamp boundary, future frames and stale history. |
| P1 | Nonzero child exits and errors arriving after the last poll could be reported as success. Exception exits skipped state invalidation. | Inspect exit status after joining the reader; scoped state ownership clears readings on both exit paths. Headless fixtures cover failed exits, callback exceptions and cleanup. |
| P1 | CMake diagnostics and the current state temporary filename were not ignored; force-tracked runtime data could bypass path checks. | Ignore diagnostics and temporary output; reject them in source review independently of Git ignore rules. Synthetic tests verify rejection without echoing payloads. Existing diagnostic files are preserved locally. |
| P2 | A concurrent state-file read could make atomic replacement fail and stop collection. | Retry Windows sharing/access conflicts for a bounded 200 ms. Tests hold an old reader open, then release it; a permanent failure must propagate without retaining the temporary file. |
| P2 | A newline could bypass the complete-line size limit; the final unterminated line was dropped. | Validate before delivering every line and deliver the final bounded tail. Fixtures cover the exact limit and one byte over. |
| P2 | Exceptions during WMI string copying or NVML initialization could bypass cleanup; window enumeration callbacks could unwind through Windows. | Scoped VARIANT/NVML session ownership and a caught callback boundary; resource lifetime and ownership comments document the contracts. |

Comments also document the borrowed Rainmeter string lifetime, delayed sensor initialization,
optional metrics, cached CPU identity, COM apartment ownership, history locking, uptime timestamps,
aggregation definitions, config override precedence and package allowlisting.

Validation uses `mise run check` and `mise run smoke`. It covers lint, configuration syntax,
deterministic generation, package/privacy boundaries, synthetic core/Windows contracts and a
sensor-free DLL ABI smoke check. Real games, anti-cheat behavior, ETW permissions, driver errors and
the alternate compiler are outside this local verification. No release or remote CI run is implied.

State cleanup cannot be guaranteed after abrupt process termination or persistent filesystem errors;
the consumer therefore rejects expired records. Privacy checks are heuristic and do not inspect or
rewrite remote Git history. No files have been staged, committed or pushed as part of this review.
