# Minotaur Rebalance: official OpenYAMM 1.21 integration

## Revision and scope

- Initial branch HEAD: `0c172ed04667abbb504df54dd37a8f9056cdd206`.
- Already integrated official 1.20 / common ancestor: `1add1d374848648887c08707199921335b4ba440`.
- Official annotated `v1.21`: `c24a0aaa6bb6f7956c84beed1bd44c88e62453c4`, published 2026-11-05.
- Backup: `backup/minotaur-before-v1.21-2026-11-06` points to the original HEAD.
- Merge: `8ee68e726b190564e9f33aaeb948497a34165463`.
- Final executable source: `c9949bbc7301240adda39e1503ff2985729d470b`.
- Supported installed baseline: `minotaur-1.20-c80ebaa9634d-25-1`.

The update uses a normal merge, preserving published history. No force-push, main-branch change,
upstream write, worktree deletion or shared Git maintenance was performed.
Five upstream commits changed 1,189 paths; 52 overlapped earlier mod changes. The official release's
Windows/Linux/macOS/Android builds and release scenarios passed; its latest stable publication succeeded.

Official changes retained: C2 party creation and mastery sheets; paused potion/scroll lists; teacher ledgers
and journal; monster chase, gait, large-actor passages and spawn visibility; indoor rendering improvements;
completion certificates; MM6–MM8 fixes and separate MAW changes.

## Conflict matrix

Eight files had textual conflicts; other overlaps were checked for behavior as well.

| File / area | Upstream change | Final resolution | Evidence |
|---|---|---|---|
| `GameplayCombatController.cpp` | Shared primary-stat bonus and condition/immunity handling | Keep upstream implementation and mod physical DR trace; remove obsolete duplicated stat table | Combat, condition, resistance and Minotaur tests |
| `OutdoorWorldRuntime.cpp` | Restore yelled-flee state; MM9 hit bookkeeping | Keep both the effective-level migration and Lethal Strike terminal HP path; record the hit once | Indoor/outdoor Lethal Strike runtime cases |
| `InventoryBags.h/.cpp` | Tab-aware sorting, named `OverflowBag = 255`, new `takeOverflow` API | Preserve native-geometry repair and serialized overflow; adopt official API | Native footprints, repair, sorting, overflow and roundtrips |
| `InventoryItemMixingRuntime.cpp` | Brew into the bottle instance; put singleton held-bottle results in the hand | Preserve atomic two-stack transaction and one-unit consumption. Do not reinsert a removed reagent | Both orientations, quantities 1/2/20/50, full-inventory refusal |
| `SaveGame.cpp` | Official schema 142 → 154 | Mod 155 → official 154; retain historical mod 143 → official 142; use decoded version for migrations | Frozen old-writer fixtures, full/summary reads, save/reload |
| `SkillRules.cpp`, `MawSkillRules.h` | Resistance and condition-save inspection | One five-argument API: compact mod Current/Next plus official resistance facts; MAW override matches | Inspection and MAW stat tests |
| `NewGameScreenTests.cpp` and new creation UI | Reworked party sheet and skill groups | Keep both upstream and mod tests. Show LS as a fixed class skill and in the Racial group, beyond optional choices | Creation, class caps and Master → GM tests |
| `GameplayItemService.cpp` (semantic overlap) | Shared carried-item action for quick lists | Retain definition-aware item replacement; lists exclude reagents and empty bottles | Quick-list and inventory tests |

The established Axe, physical Damage Resistance and Lethal Strike v0.4 + Luck formulas are unchanged.
Dual Axe, Axe + Mace in either supported orientation, GM Recovery and two-handed physical damage remain covered.
Existing skill names, teachers, success messages, compact stat descriptions, HP alignment and Recovery inspection remain.

### Existing hotfixes and official behavior

Alchemy item/hireling bonuses, black-potion limits, Mind Resistance duration and Levitate protections were already
integrated in the 1.20 base. No second application of these fixes or bonuses was introduced. Stack transactions
remain an extension of the official bottle-template fix, not a replacement for its Alchemy calculations.

Official 1.21 also derives undead/Lich Mind and Body immunity and removes old erroneous saved Light/Dark immunity.
The official condition handling and save migration are retained. MAW rules remain ruleset-scoped.
Identification rules and native reagent/bottle geometry are not changed by this port.

### Independent review

A separate read-only reviewer checked the merge, save mapping, transactions and updater. Findings corrected:

1. The new hardcoded creation skill groups omitted LS; it is now visible and fixed for the Minotaur classes.
2. The singleton bottle result could reinsert an already consumed reagent; the transaction now respects
   `targetItemRemoved`. The public brewing matrix verifies quantity conservation.
3. Retaining old versions initially skipped root launcher refresh. The follow-up retains only obsolete app/pack/legacy
   cleanup and leaves launcher refresh on the shared path. The test checks both launcher bytes and retained rollback data.

The final reviewed source had no confirmed blocking findings. The reviewer made no file/ref changes and ran no builds.
New upstream resistance/save supplemental lines remain English even in the Russian inspection path; existing compact
mod Current/Next text remains localized. This presentation limitation does not change calculations.

## Save compatibility

The mod and official headers remain separate namespaces. Official schemas through 154 are accepted. Historical mod
mappings remain explicit: 89→88, 94→93, 95→94, 130→129, 132→131, 140→139, 143→142, 155→154.
Unknown future mod headers are rejected. The legacy exact-format reader is retained.

New official fields include completion records, journal note flags, teacher ledgers, MM9 placement state and MAW
preferences. Old saves take the official gated defaults and migrations. Named skills, LS ID 49 and item quantities
are preserved; this update changes neither the stack representation nor native item dimensions.

The frozen genuine previous writer fixture is `tests/fixtures/minotaur-1.20-v143-bottles.oysav`:
9,346 bytes, SHA-256 `fa4ab7f14c4cbbb961e5b95d60f1ac088c5090bb81c3334668c5635881c71c60`.
It was captured unchanged from the old binary before rebuilding, not generated with a relabeled new header.
The sidecar records provenance. It is a synthetic regression save, not the user's campaign.

Frozen mod 132/140/143 saves are loaded, summarized, restored, written as the new schema and loaded again.
Checks cover member identity/class/level/XP, skills/mastery, spells, equipment, inventory and stored quantities,
quest bits/autonotes, gold, map identity, camera and saved map counts. Other legacy skill fixtures remain covered.
Expected official migrations, including Lich immunity correction, are intentional.

The user's private MM8 saves are unavailable. These checks do not certify every campaign/map state or visual layout.
New schema 155 saves are not promised readable by the old 143 executable. Keep the old save and use a new slot.

## Installer and updater

`Apply-MinotaurUpdate.cmd` and its PowerShell implementation remain the installation entry point.
The patch supports the exact baseline named above, not arbitrary official or mod versions. It verifies the installed
index and resource hashes before switching, carries the new program and changed packs, and reuses unchanged packs.

Before switching `app/current`, it copies `saves` and `settings.ini` to
`backups/minotaur/<previous-version>-<unique-id>`. Copy failure prevents the switch; originals remain untouched.
Old app versions, packs and `app/previous` remain after a healthy Minotaur start. Root launcher refresh still runs.
Official numeric releases retain their normal cleanup. Official signed app updates cannot replace the offline
`minotaur-*` version namespace; this behavior is regression-tested.

For rollback, close the game, set `app/current` to the version recorded in `app/previous`, and restore pre-update
saves/settings. Do not try to open new-format saves in the older executable.

## Project Medusa isolation

Only the Minotaur worktree and branch were changed. No Medusa files, refs, processes, settings or build directories
were modified by this task. Fetches share the object store as intended, with automatic maintenance disabled.
Medusa independently advanced under Claude from the initial `0c172ed` to
`2c1ae605467a08af411e4f5a424a1fe37b34a11c`; that is not a Codex ref update.
Four preexisting untracked research documents remain unchanged and uncommitted.

A later Medusa sync should review overlapping `NewGameScreen`, `MenuDesignScreen`, `GameApplication`,
`RulesetDefinition` and `AssetFileSystem` work against the new party UI/content architecture. This task neither
merges nor rebases Medusa and does not claim those future conflicts have been resolved.

## Automatic verification

| Set | Cases passed | Assertions |
|---|---:|---:|
| Selected gameplay / Minotaur / save / inventory / UI units | 258 | 49,851 |
| Selected native Minotaur / inventory / stealing runtime cases | 7 | 163 |
| All `UpdaterTests.cpp` cases | 15 | 146 |
| AI / paths / bolster / character sheet units | 147 | 2,204 |
| Indoor detection / flying / faction runtime cases | 18 | 1,097 |

The sets overlap; they must not be summed as unique test coverage. Linux Python packaging tests passed 7 cases;
10 Windows-only cases were skipped there and run with the actual Windows PowerShell 5.1 in CI.
The full unit suite was not run. MM9/RUDE/Bloodaxe and the GPU-only lava-shader case are excluded from the selected
MM6–MM8 set, not counted as passing.

Exact filters are in `.github/workflows/minotaur-windows.yml`. Local executables use `build/tests/` instead of
`build-windows/tests/`. Every compilation and resource-intensive test used `safe-heavy-run`, jobs 4.
Build targets: `cmake --build build --target openyamm openyamm_unit_tests openyamm_runtime_tests --parallel 4`.
Final local test log: protected run `20261106T121158Z-6c07620b5c`.

`tools/check_doc_i18n.py`: 3 pairs, 0 errors, 4 existing translated-reference warnings.
`tools/lighting/stale_bakes.py --check`: 0 stale in the materialized sparse scope, not a full art-production audit.
Initial sparse-fixture failures were resolved with canonical source assets; no tests were weakened.
Interrupted/failed Git downloads were network failures, not OOM. The permanent metrics retain those attempts.

Main local incremental port build: 565.679 seconds, peak cgroup memory 4.682 GiB, mean 3.761 CPU cores,
zero observed swap, memory PSI and OOM events. The shared budget remained 16/18 GiB plus 1 GiB swap.
This is not a cold-build benchmark or a measurement of whole-VM usage.
[Local resource records](openyamm-1.21-local-resources.csv).

## Windows result

SUCCESS: final run [42215979271](https://github.com/Plane-PPP/openyamm/actions/runs/42215979271), source `c9949bbc`.
All listed sets passed on Windows too; its updater set has 145 platform-specific assertions versus 146 on Linux.
The ZIP is 1,932,865,419 bytes, 53 changed and 113 reused packs. GitHub digest matches the CI archive SHA-256;
HTTP Range inspection verified the real central directory/manifest/installer. A full duplicate local download did
not finish within the bounded network attempt; payload hashing completed in CI, not independently on this VM.
The previous run 42209078766 passed but is superseded by the launcher follow-up; do not use its artifact here.
[Final ZIP, Windows measurements and installation](windows-1.21-build.md).
Manual private-campaign, GPU, aspect-ratio and Android visual checks remain pending.
