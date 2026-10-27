# Windows patch: Alchemy, Levitate and Mind Resistance

Status: Implemented / Automatically tested on Windows / ZIP packaged. Manual gameplay pending.

Source: `c991da37a2de3b6c74a5ed838e38537fb6a5cc2b` on `feature/minotaur-rebalance`.
[Actions run](https://github.com/Plane-PPP/openyamm/actions/runs/40541424092).
Base: `minotaur-1.14-0b4fc6589a50-19-1`.

## Execution and parallelism

The mod workflow builds natively on GitHub `windows-latest` with MSYS2 UCRT64.
It does not cross-compile locally on the Ubuntu VM. The upstream release workflow is different
(its Windows cross build also runs on GitHub, on Ubuntu); it is not used for this mod package.
Neither upstream main nor fork main is changed.

Runner CPU count and physical/available RAM are measured before compilation. Jobs are the minimum
of 8, detected CPU count and floor((available RAM - 2 GiB) / 2 GiB).
GitHub documents standard public Windows runners as 4 vCPU / 16 GB; Ubuntu's 8 vCPU and
16/18 GiB cgroup limits do not determine the remote build's settings.

Per-phase reports measure the whole Windows runner every second: peak used physical RAM,
minimum available RAM, Windows commit charge and average/peak system CPU utilization.
This is not compiler RSS and not directly comparable with the previous Linux task-cgroup peak.
Windows commit is not disk swap usage. Linux PSI and cgroup/OOM counters are unavailable.
A low physical-memory or commit reserve sustained for 10 seconds stops the workload process tree;
there is no automatic retry or limit increase. The report artifact uploads even on failure.

## Intended content and verification

- Alchemy: trained level + effective skill bonuses, then ingredient power; MAW bonus/cap unchanged.
- Levitate: protect indoor/outdoor environmental lava/water damage; preserve selective trap filtering.
- Mind Resistance: restore duration and consequently the buff/Stats bonus.
- No changes to Lethal Strike, Axe, Damage Resistance or save schema.
- Existing Linux coverage: 81 selected cases / 32,164 assertions passed.
- Windows installer tests and native monitor smoke test passed.
- Windows gameplay tests: 81 cases / 32,164 assertions passed; package validator passed.

Back up saves before manual testing and save into a new slot. The ZIP installer preserves saves/settings.
This patch must not be marked manually tested until the user checks the Windows game.

## Manual checks after installing the ZIP

1. Brew a new catalyst with trained Alchemy 4, displayed equipment bonus +25 and a reagent
   of intrinsic power 10: effective Alchemy 29, resulting potion power 39. Existing potions
   are not recalculated. Check both inventory mixing and the alchemy bench.
2. Cast Levitate before standing on lava/water, indoors and outdoors where supported.
   Environmental damage should stop while the buff is active and resume after expiry.
   Ground pressure traps retain their existing selective filtering; chest traps are separate.
3. Cast Mind Resistance and inspect its party effect and every member's Mind resistance.
   For example Mind Magic 4 Expert gives +8 resistance for four game hours. Character level
   20 does not replace the numeric Mind Magic skill in this calculation.

## Confirmed package

[Download ZIP](https://github.com/Plane-PPP/openyamm/actions/runs/40541424092/artifacts/12753968279):
`Minotaur-Update-minotaur-1.14-c991da37a2de-20-1.zip`, **22,234,588 bytes**.
Base `minotaur-1.14-0b4fc6589a50-19-1` -> target `minotaur-1.14-c991da37a2de-20-1`.
SHA256: `de2984f9ea537bdb719bd9524dd2aeaa624db58fad172be70960ca294110f14b`.
Changed resource packs: **0**; reused: **132 / 5,777,255,868 bytes**. No new full installation.
The next-update baseline is now the confirmed target; older baseline manifests remain available.

## Measured Windows resources

Actual runner: **4 vCPU, 17,174,360,064 bytes physical RAM (~15.995 GiB)**.
Parallelism selected: **4**, not 8. Ubuntu's CPU allocation and cgroup limits were not changed.

| Phase | Seconds | Peak whole-runner RAM, GiB | Mean CPU, % of runner |
|---|---:|---:|---:|
| Game, launcher, shaders, cooker | 1179.90 | 7.220 | 91.44 |
| Unit-test compilation/linking | 946.67 | 7.404 | 98.75 |
| Selected gameplay tests | 3.57 | 2.632 | 27.40 |
| Content indexing | 145.98 | 3.832 | 27.49 |
| Program packaging | 6.94 | 3.530 | 17.91 |
| Incremental ZIP creation | 3.00 | 3.056 | 28.65 |

The Windows job took **50 min 34 s**, including checkout, toolchain installation, configuration,
verification and uploads. The entire workflow took **51 min 16 s** from creation to final update.
The two compilation phases total **35 min 26.57 s**. Maximum sampled CPU utilization: **100%**
(all four vCPU); minimum sampled available RAM across monitored phases: **8.591 GiB**.
All monitored commands exited 0 with reason `completed`; the low-memory guard did not trigger.
No retry or automatic limit increase was performed. This was not a jobs=8 trial.
The previous Windows job took 48 min 55 s, but runner hardware/network and fresh dependencies
are not controlled; no speed regression or benefit can be attributed to monitoring from one run.

[Resource artifact](https://github.com/Plane-PPP/openyamm/actions/runs/40541424092/artifacts/12754552629)
contains every-second samples and phase summaries. A [durable summary](data/windows-build-c991da37-resources.json)
is kept in the repository. Local downloaded evidence is in
`/home/plane/.local/state/openyamm-windows/40541424092/`.
This whole-Windows-runner RAM measurement is not the earlier Linux task-cgroup peak of 4.09 GiB.
No claim of zero disk swap or Linux PSI/OOM counters is made for Windows; those metrics are unavailable.

The packager's ZIP SHA256 and byte count match the GitHub artifact's independent `digest`
and `size_in_bytes`. The optional local full download was interrupted for slow transfer;
its partial file is retained as `.zip.partial` and is not a validated/local distributable.
The download link above points to the complete verified GitHub artifact.
