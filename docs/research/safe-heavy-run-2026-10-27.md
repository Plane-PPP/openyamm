# First protected OpenYAMM compilation — 2026-10-27

Status: successful normal-load validation; no stress/OOM experiment and no jobs=6 run.
Target: `openyamm_unit_tests` on `feature/minotaur-rebalance`, with the three gameplay fixes.
This rebuilt and linked the Linux test executable and its gameplay/engine dependencies;
it did not produce a new Windows game package or run a desktop/Vulkan build.

## Recovery and preflight

The interruption left modified AlchemyBenchRegressionTests, GameplayRuleRegressionTests and
SpellRegressionTests, before production fixes. Preserve those changes and the separately updated
AGENTS.md. No reset, cleaning or deletion of existing build data.

Read `/home/plane/AGENTS.md`, project AGENTS.md, safe-heavy-run README and implementation report.
Sandbox systemd access refused as intended (exit 125); host execution of the same protected runner
was approved. `check` confirmed 8 vCPU, ~23.40 GiB guest RAM, ~21.72 GiB MemAvailable,
unused 4 GiB system swap, memory PSI zero and no competing compiler/Make/Ninja/CTest.
Direct systemd inspection confirmed the empty parent slice and limits:

| Setting | Verified value |
|---|---:|
| MemoryHigh | 17,179,869,184 bytes (16 GiB) |
| MemoryMax | 19,327,352,832 bytes (18 GiB) |
| MemorySwapMax | 1,073,741,824 bytes (1 GiB) |
| TasksMax | 512 |
| Parallelism | 4 |
| OOMPolicy / KillMode | kill / control-group |
| NoNewPrivileges | yes |

CMake was still absent; `/tmp/minotaur-fix-toolchain` had disappeared. Download and unpack
Ubuntu CMake 3.28.3, cmake-data, libjsoncpp25 and librhash0 into
`/home/plane/.local/share/openyamm-toolchain/root`, without system installation.
Restore pinned FreeType VER-2-14-2, PhysFS release-3.2.0, yaml-cpp 0.8.0 and
ImGui v1.92.6-docking under its `sources/`. Reconfigure the existing build directory through
safe-heavy-run with these permanent paths. Asset cooking/editor/tools remain disabled.
System limits, services, kernel, swap and Proxmox settings were not changed.

## Real compilation measurements

Persistent evidence directory:
`/home/plane/.local/state/safe-heavy-run/runs/20261027T160723Z-ec91891a58/`.
Its summary.json, metrics.jsonl and output.log remain authoritative. `safe-heavy-run recent`
confirms completion with exit 0. Configure run: `20261027T160706Z-893f3cac4d` (exit 0).

| Measurement | Result |
|---|---:|
| Duration | 260.385 s (4 min 20 s) |
| Task cgroup memory peak | 4,390,703,104 bytes / 4.089 GiB |
| Task swap peak | 0 |
| Minimum sampled VM MemAvailable | 17.827 GiB |
| Maximum sampled total RAM minus MemAvailable | 5.576 GiB |
| CPU time | ~1,023.7 CPU seconds |
| Average task CPU | ~393% of one CPU (~49% of 8 vCPU capacity) |
| Maximum sampled load average 1 min | 4.081 |
| Host memory PSI some/full avg10 | 0 / 0 in every sample |
| Task memory PSI total some/full | 0 / 0 microseconds |
| Parent high/max/oom/oom_kill/oom_group_kill deltas | all 0 |
| Watchdog/guard stop | none |

The monitor checked every 5 seconds and retained ~30-second metrics and a final sample.
Compilation and final linking both completed under the same cgroup; no per-stage peak is claimed.
Short sub-sample host fluctuations cannot be excluded. Task peak uses cgroup/kernel accounting;
CPU average derives from cumulative task CPU and elapsed sampling time.

## Comparison with the incident

Historical evidence: `/home/plane/incident-2026-10-27/REPORT.md` and retained sar/journal files.

| Measurement | Interrupted October 27 build | Protected run |
|---|---|---|
| VM / jobs | ~11.6 GiB, 4 vCPU / 25 | ~23.4 GiB, 8 vCPU / 4 |
| RAM | 97.66% used by sar; available 0 | task peak 4.09 GiB; available >=17.83 GiB sampled |
| Swap | ~4 GiB / nearly 100% | 0 |
| CPU / load | up to 92.32% system CPU; load 71.10 | task mean ~393% of one CPU; load <=4.081 sampled |
| Duration/outcome | did not complete; ~33 min to end of old boot | 260.385 s; exit 0 |
| Memory pressure | repeated journald pressure warnings; PSI not retained | host/task PSI 0; no limit/OOM events |

These are different VM sizes and incremental build states. Historical sar system CPU/RAM and
new task cgroup metrics are not directly equivalent; this is not a controlled speed benchmark.
The mechanism of the VM reset remains unknown; do not attribute it to an OOM killer.

## Tests and next step

Focused tests: 7 cases / 404 assertions. Final related tests: 81 cases / 32,164 assertions.
All passed through the protected runner. See [gameplay investigation](2026-10-27-gameplay-fixes.md)
for filters, restored fixtures, earlier failed runs and excluded full-map coverage.
No real OOM or dangerous PSI was deliberately induced; successful normal execution verifies
placement, limits, monitoring and completion, not real-OOM cleanup under exhaustion.

The jobs=4 run exceeds 30 seconds, peaks below 12 GiB and has zero high/max/OOM events,
so it meets the runner's prerequisites to propose a separate jobs=6 experiment.
Do not launch it without user direction. Do not clean/rebuild solely to manufacture a benchmark;
a repeat on hot incremental outputs would not establish a meaningful speedup.
