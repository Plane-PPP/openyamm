[Русский](ROADMAP.md) | **English**

# Minotaur Rebalance — Roadmap

## OpenYAMM 1.21 — Windows ZIP ready

Official `c24a0aaa6bb6f7956c84beed1bd44c88e62453c4`; balance preserved. Save mod155/official154,
historical mod143 maps to official142. The new creation sheet shows innate Lethal Strike.
[Audit, conflicts and verification scope](research/openyamm-1.21-integration.md).
Local unit258/258,49,851assertions; runtime7/7,163assertions passed. Windows CI passed; private campaign saves have not been manually tested.
Package `minotaur-1.21-c9949bbc7301-28-1`, source `c9949bbc7301240adda39e1503ff2985729d470b`.
[ZIP, measured resources and installation](research/windows-1.21-build.md).
[Permanent Release](https://github.com/Plane-PPP/openyamm/releases/tag/minotaur-1.21-c9949bbc7301-28-1) — unchanged ZIP, no Actions retention expiry.

Previous summary as of 2026-11-02. Base: official OpenYAMM v1.20, last confirmed package
`minotaur-1.20-c80ebaa9634d-25-1`, mod save format 143. The v0.4 + Luck balance has not changed.
The detailed chronology is kept below under "History". Document index: [docs/README](README.en.md).

Statuses: automatic tests and manual verification are tracked separately. "Implemented" does not mean "verified in game".

**Mod scope.** The mod targets the original MM8/OpenYAMM experience with focused balance and mechanics changes.
The MAW ruleset is out of its scope, and compatibility with it is not required.

**Latest automatic check (2026-11-02, Linux, uncommitted tree on `21f8b78`).** CI set: unit 192 of 192,
48,820 assertions; runtime 7 of 7, 159 assertions. The full unit suite was not run. Details:
[audit](research/independent-audit-2026-11-02.en.md).

## Implemented

| Feature | Automatic tests | Manual verification |
|---|---|---|
| Minotaur Damage Resistance | Yes | Build #3: confirmed from logs against dragonflies |
| Master Axe: one-handed axe in the off hand, including Mace in the main hand + Axe in the off hand (T-1, T-2) | Yes | Build #3: two axes; mixed pairs and save/load not verified |
| GM Axe: Recovery reduction for a one-handed axe | Yes | Build #3: from logs; full matrix not verified |
| GM Axe: ×2 on the physical part of a two-handed blow | Yes | Build #3: confirmed; all targets and enchantments not verified |
| Diagnostics `minotaur_damage_trace`, `axe_damage_trace`, `lethal_strike_trace` | Yes | Logs partly used |
| Lethal Strike v0.4 + Luck, including Luck from worn items (T-3), teachers, Stats row (LS-001, LS-002) | Yes | Layout and topics confirmed; combat run not done |
| Reagent and bottle stacks to 50, Shift and Ctrl, one-unit consumption | Yes | Pending |
| Native icon geometry and safe migration of old layouts | Yes | Pending; no screenshots claimed |
| Tooltips with real stat thresholds and Recovery | Yes | Pending |
| Levitate protection from water and lava | Yes | Pending |
| Alchemy and Mind Resistance fixes from upstream | Yes | Pending |
| Saves: mod 155 reads mod 143 and mods 140, 132, 95, 94, 89 and official formats | Yes | The user's real save on 1.20 is not claimed |
| ZIP updates and Windows CI | Yes | The user ran the 1.20 package (window glow report); the documents record no separate installation check |

## In Progress

These items need no new code; they are waiting for your playthrough.

- Campaign and visual check of stacks, geometry and tooltips on 1.20 at native scale.
- Lethal Strike combat: frequency, message, GIANT 60%, dynamic P0, teachers, old saves.
- Mixed weapon pairs and save/load, including Mace in the main hand + Axe in the off hand.

## Planned

| Task | What blocks it | Depends on |
|---|---|---|
| LS-003: themed teacher texts and next-teacher hints | Needs your texts | — |
| UND-001…003: Lich and Vampire immunity schools, save migration | Needs a fidelity decision | MM8 verification |
| Surface Materials maps for MM8 towns | Separate content task | — |
| Weapon tooltip cache (A-1 from the [audit](research/independent-audit-2026-11-02.en.md)) | Measure the performance impact first | Measurement |

## Research / Ideas

Not approved for implementation; the ideas are kept.

- Identify Monster, knowledge, AC/damage/aura/GM magic: Deferred, [design](design/identify-monster.md), [monster knowledge](design/monster-knowledge.md).
- Bestiary and lore: Deferred, [research](research/monster-lore.md).
- Verifying special attacks against undead in original MM8: open question, [research](research/undead-condition-immunities.en.md).
- Windows CI optimization: postponed by the 2026-10-29 decision, [research](research/windows-ci-optimization.md).
- Adjusting the experimental Lethal Strike balance: only after the manual run.
- Lethal Strike and special protections: **Deferred / Low Priority**. A potential interaction with energy shields and invulnerability is known. Intended behaviour: instant death does not bypass them. The protection rules must be clarified against current code and published OpenYAMM content; the fix is postponed until there is a practical need. [Materials](design/lethal-strike-protection-task.en.md).

---

# History

The chronology is preserved as written, with later entries first. Test counts and statuses belong to their own stage.
The first entry predates this summary. The current state appears only above.

## MM8 night-window glow — research complete; content coverage pending

Investigated the Windows 1.20 report in Alvar/Ravenshore/Shadowspire. No Minotaur renderer regression or missing
surface-pack delta found: upstream 1.20 contains only 11 MM8 material pairs; Alvar matches only solid01.
[Coverage, renderer/settings/package checks and hardware limits](research/surface-materials-mm8-window-glow.en.md).
No code/resources/build/push change. Next, separately scoped: author/cook missing MM8 material maps and visually
validate day/night; use covered MM6 New Sorpigal to distinguish any independent driver/runtime issue.

## Undead condition immunities — research complete; fixes pending

The user reproduced Naga poison on a quest-promoted MM8 Lich and a Vampire. Vampire poison is compatible with its
expected Mind-only immunity. Standard Lich promotion incorrectly sets Light/Dark instead of Mind/Body; inherited
from upstream, not Minotaur Rebalance. Standard Vampire Mind initialization is also missing; MAW has separate race rules.
[Evidence, original-game distinctions and unresolved native-MM8 special-attack semantics](research/undead-condition-immunities.en.md).
No gameplay/save/CI change. Next: decide fidelity target and verify native MM8 before implementing condition policy;
then separately scoped school/migration fixes and regression tests. No blanket undead condition immunity approved.

## OpenYAMM 1.20 — implemented / Linux and Windows tested / ZIP packaged

Official stable v1.20: 1add1d374848648887c08707199921335b4ba440, published 2026-10-30.
Backup: backup/minotaur-before-v1.20-2026-10-31 at b776797. Normal merge preserves history.
Retain upstream search/bag fallback/death explosions and the mod's stack/Minotaur APIs.
Mod 143 wraps upstream 142; mod 140 explicitly continues to decode upstream 139, including the save-list summary path.
Native multi-cell geometry and safe layout migration implemented. Linux 189/48,755 and native 7/159 passed.
Additional save/inventory selection 165/21,008 passed (overlapping). Visual preparation blocked by incomplete
local terrain resources; no rendered screenshots claimed. Windows 189/48,755 and 7/159 passed.
[Successful 254.81 MiB ZIP and installation](research/windows-1.20-build.md), 47 changed/119 reused resource packs.
Confirmed next-update baseline advanced. Next: user campaign and native-scale visual retest; no balance change.
[Integration audit](research/openyamm-1.20-stack-geometry.md).

## Historical 1.18 UI & inventory hotfix — superseded geometry; other fixes retained

Known-by-rules item names and stack identification compatibility corrected; genuine unknowns preserved.
Shared bounded stack icon geometry fixes Wasp Wing/Gog Blood overflow. Empty bottles stack to 50;
brewing consumes 1 bottle and 1 reagent with atomic cursor placement and safe MAW closed-bag handling.
Seven stat tooltips keep original/current/next information without threshold tables; HP shares the SP/AC/DR edge.
169 gameplay cases / 39,730 assertions and 4 native world/layout cases / 123 assertions passed.
Additional inventory/chest/corpse selection: 89 cases / 6,820 assertions passed.
[Causes, compatibility and manual checklist](research/ui-inventory-hotfix.md).
[Successful Windows run and 31.27 MiB ZIP](research/windows-ui-inventory-hotfix-build.md) over the prior 1.18 package.
Next: user campaign/visual retest. Confirmed next-update baseline recorded; existing deferred design scope unchanged.

## Expanded inspection — Implemented / automatically tested / Windows ZIP packaged

Real Standard attribute bonuses/thresholds and armor/weapon Recovery now supplement original descriptions.
Read-only equip previews, ruleset Recovery overrides and bounded wheel/touch scrolling included.
156 selected cases / 36,020 assertions and three native Lethal Strike cases / 58 assertions passed.
Linux and Windows checks passed; [Windows package and resource report](research/windows-1.18-build.md).
[137 MiB update ZIP](https://github.com/Plane-PPP/openyamm/actions/runs/40863165794/artifacts/12894798568)
updates minotaur-1.14-c991da37a2de-20-1 to minotaur-1.18-c0ccf0a877bc-23-1.
A separate local inventory/loot selection passed 83 cases / 4,076 assertions; debug-Lich test scope corrected.
Next stage: user campaign-save, inventory and visual retest; then separately scoped corrections.
Identify Monster, bestiary and teacher flavor/next-teacher hints remain Deferred.
[Current 1.18 notes](minotaur-1.18.md).

## Reagent stacks — Implemented / automatically tested on Linux and Windows

Up to 50 ordinary same-property reagents per stack; atomic pickup, manual merge/remainder, split controls,
sorting, single-unit brewing, sale/buy pricing and quantity persistence added.
21 selected cases / 1187 assertions passed. [Current 1.18 notes](minotaur-1.18.md).
Expanded tooltips and final unit/runtime/Windows packaging checks passed; manual gameplay remains pending.

## OpenYAMM v1.18 integration — Implemented / automatically tested / packaged

Merged official v1.18 `4ab07ed519a66feab24599d69bc0ae19971250e2` into the feature branch,
from backup `backup/minotaur-before-v1.18-2026-10-29` (`9dcf1c7`).
Official Alchemy/item/follower, black-potion and Mind Resistance fixes retained; no duplicate bonus.
Levitate setter duplication removed; pre-movement sync and queued-effect guards retained.
Adapted Lethal Strike death handling to ShieldHitModifiers without changing balance.
Cybernetics keeps upstream ID 48; Lethal Strike moves to 49, named saved skills and teachers preserved.
Mod save 140 maps to upstream 139; production mod 132 maps explicitly to 131; older mappings retained.
Original 1.14 binary generated a frozen v132 fixture; 85 selected cases / 32,239 assertions passed.
Reagent stacks, expanded tooltips and Windows packaging completed in the stages above.
Historical intermediate counts below describe their own stage, not the current package.

## History: preparing the upstream update — superseded by the v1.18 integration above

Checked our Alchemy/Mind Resistance/Levitate against upstream `fc4e117` (2026-10-28).
[Merge and verification plan](research/upstream-fc4e117-fix-integration.md).
Found 4 conflicting hunks in 3 Levitate files and hidden setter duplicates, including
an automatically merged header. No full merge, implementation or build took place.
Keep Alchemy/Mind Resistance for now; merge Levitate without duplicates, keeping the new
upstream floorBurn/resistances/FireImmune and our first-update/queued-tick guards.
The real integration and all its checks remain Pending. The latest Windows build below did not change.

# Windows gameplay fixes — packaged / manual retest pending

Alchemy equipment bonuses, Levitate environmental protection and Mind Resistance duration:
Implemented / Automatically tested on Linux and Windows / Windows ZIP packaged.
81 selected Windows cases / 32,164 assertions passed; installer and package checks passed.
[Package, measurements and instructions](research/windows-gameplay-fixes-build.md).
Source `c991da37a2de3b6c74a5ed838e38537fb6a5cc2b`, successful run 40541424092.
22.2 MB update for installed `minotaur-1.14-0b4fc6589a50-19-1`; 132 content packs reused.
Next-update baseline: `minotaur-1.14-c991da37a2de-20-1`.
Remote Windows runner had 4 vCPU / ~16 GiB and used jobs=4; peak RAM 7.40 GiB.
Local Ubuntu limits unchanged; no jobs=8 experiment. Next: user's actual-save installation/gameplay retest.
Identify Monster, bestiary and teacher flavour/next-teacher hints remain deferred/pending as previously recorded.

# Initial OpenYAMM 1.14 port / Minotaur v0.4 + Luck — previous package

New official 1.14 appeared during 1.13 validation; exact 1.14 now supersedes that unpublished
intermediate port. The update still targets installed `minotaur-1.10-b7e80f0583df-14-1`.
Installer root detection fixed; Minotaur balance retained. New save OYMINO1/v132 maps to
schema 131; production mod 95 still explicitly maps to 94. [Compatibility](research/openyamm-1.14-compatibility.md).
Windows build/tests and ZIP publication succeeded; manual gameplay pending. Earlier 1.13 entries below are history.

## Windows 1.14 — Packaged / manual retest pending

[Successful run 40325494796](https://github.com/Plane-PPP/openyamm/actions/runs/40325494796), source `0b4fc6589a5003142c2a0a1d3dd82da6d5542d43`.
[Download ZIP patch](https://github.com/Plane-PPP/openyamm/actions/runs/40325494796/artifacts/12654334120): **910,392,473 bytes (910.4 MB)**.
Base `minotaur-1.10-b7e80f0583df-14-1` → `minotaur-1.14-0b4fc6589a50-19-1`.
Changed 23 resource packs (888,146,682 bytes); reused 109 packs (4,889,109,186 bytes).
The delta includes updated shared icons/models/world items, core/maps and the new ruleset pack.
84.17% less download than the previous 5,751,530,172-byte full 1.10 archive; no new full 1.14 archive was built.
SHA256: `8bc8d0182468ee147f4d50090653156b3e99e0f4b100cc62c4c0ee42c70c4407`.

Automatically tested on Windows: 29 selected gameplay cases / 30,791 assertions; 17 updater tests,
including 10 actual PowerShell 5.1 filesystem scenarios; full/thin validator regressions.
Native x64 game/launcher, shaders, all resource hashes and embedded teacher/Stats resources verified.
This is selected coverage; MM9 data is outside this package and its Bloodaxe quest is excluded.
The user's actual save and manual gameplay/installation of this 1.14 patch remain untested.
Back up saves, run the **new patch folder's** Apply-MinotaurUpdate.cmd and save into a new slot.
Installer supports both nested extraction and scripts placed directly in the game root.
The next build defaults to this newly confirmed baseline; old baselines remain available.

# OpenYAMM 1.13 / Minotaur v0.4 + Luck — integration, Windows checks pending

Port to latest official v1.13; previous balance retained. ZIP update base:
`minotaur-1.10-b7e80f0583df-14-1` (user confirmed installation on 2026-10-26).
Installer now detects extraction directly into the game root as well as a child folder.
Lethal Strike moves to skill ID 48 to preserve new upstream IDs; named saved skills are retained.
New save header OYMINO1/v130 maps to schema 129; old mod 95 remains explicitly schema 94.
[Compatibility review](research/openyamm-1.13-compatibility.md). Windows packaging and manual gameplay pending.

# Original v0.4 roadmap

Current specification: **Lethal Strike v0.4 + Luck**, 2026-10-15 with the 2026-10-22 addendum. It replaced Death Strike v0.2.
Identify Monster and the bestiary are Deferred. Research 69bed40 is preserved; it does not limit
the formulas, categories or teachers directly approved in v0.4.

## Confirmed baseline: Windows build #3

> Historical entry: this was the first manual verification of the Axe mechanics. The current update baseline is in the summary at the top.

SHA 142a54e01da50d9faf258e38919e88dba9e8de29,
[Actions run #3](https://github.com/Plane-PPP/openyamm/actions/runs/38148278640).
On Windows 11 the user verified two-handed GM Axe ×2, two one-handed axes, the off hand and the doll,
the Axe descriptions, Recovery from logs and passive physical protection against dragonflies on Emerald Island.
This is a limited manual check of the listed mechanics, not of all combinations, saves or situations.
Axe, Armsmaster and the passive protection formula are kept in v0.4.
Details: [CHANGELOG](../CHANGELOG.md), [mod description](MINOTAUR_REBALANCE.en.md).

## Development stages

Previous confirmed Windows package: 1.7+Luck, source a1b8deb; successful run 39898467800.
The 1.5 builds and original v0.4 below are kept as history.

| Stage | Status | Result |
| --- | --- | --- |
| Table/map research | Research completed | 658 shared templates, 198 original MM8, 184 with campaign evidence; [review](research/final-review.md) |
| v0.4 contract | Approved | P0 from S/V and the current Luck bonus (2026-10-22), 30% cap, GIANT strictly below 60%, Identify Monster not involved |
| Category registry | Implemented | 48 GIANT / 610 NORMAL; MM8 21/177; explicit IDs, unknown NORMAL + trace warning |
| Skill and training | LS-001 fixed / topics manually confirmed | Correct topic schema; Expert Perception / Master Mace / GM Axe; [details](research/lethal-strike-teacher-bug.md) |
| Combat and diagnostics | Implemented | One trial after melee, standard death, visible message, separate trace |
| Skills/Stats | LS-002 fixed / layout manually confirmed | Four resource rows inside the frame, original Condition/Quick Spell, separate Earth/Mind/Body; [details](research/damage-resistance-layout-bug.md) |
| Automatic checks | Automatically tested on 1.7+Luck | 26 focused / 30530 assertions; 3 runtime / 58; 223 selected / 7506. Suite limits in BUILD-NOTES |
| Windows packaging on 1.5 | Packaged / success | [Run 39297183309](https://github.com/Plane-PPP/openyamm/actions/runs/39297183309); SHA bad9c27; OpenYAMM-windows-Release.zip |
| v0.4 manual test | In progress / hotfix retest pending | Stats fix and topic appearance confirmed; LS-003 and the combat run remain |
| Identify Monster | Deferred | Caps/UI/behaviour unchanged; knowledge, AC/damage/aura/GM magic postponed |
| Bestiary/lore | Deferred | Research kept, parsing and extended interface postponed |

## Resource hotfix 1

LS-001 and LS-002 were fixed in separate commits. A [small patch](patches/Minotaur-v0.4-hotfix-1.zip) was prepared
for executable 385a1e4, [installation and test](patches/README.md). The automatic run of 22 tests / 30155 assertions
succeeded; the user confirmed the layout and topic appearance.
The 1.5 build included the fixes in engine.zip; 1.7 includes them in indexed content packs without a separate hotfix.

## Next stages

Resource fixes LS-001/LS-002 are confirmed by the user for Stats and topic appearance.
[LS-003](research/lethal-strike-teacher-dialogue-followup.md): the next build should receive
the user's themed texts and next-teacher hints. Not yet implemented.

1. Obtain the user's rank texts and implement LS-003; then prepare the next full build.
2. Manual Windows v0.4 run: frequency/message, GIANT 60%, dynamic P0, Damage Resistance, teachers and old saves.
3. Record the user's results separately from what was automatically tested.
4. Based on the results, adjust the experimental balance as a separate stage.

Historical original v0.4: automatic checks and Windows packaging completed. Production source is
385a1e4. The hotfix changes three resources on top of that build, not the executable;
the test/documentation commits preceding the hotfix did not change game code.
Details and limits: [BUILD-NOTES](../BUILD-NOTES.md).

Record each completed stage as a separate commit with a ROADMAP update.
Do not merge main. New Axe/DR balance and Identify Monster are out of scope.
Current contract: [Lethal Strike v0.4](design/death-strike.en.md).

## Port of v0.4 to OpenYAMM 1.5 — Packaged / user save retest pending, 2026-10-19

Official 1.5 tag: 1414884155362e613977a3444a3c7f81ed4ee047. Before merging, a fixture of the old mod-v85 was
preserved. A collision of number 85 with the upstream weather layout was found;
compatibility is ensured by separate decoding and verified by fixtures. Hotfix LS-001/LS-002 is preserved.

Integration and compatibility verified: 25 focused/30294, 3 runtime/58, 205 selected/7329.
[Report](research/openyamm-1.5-compatibility.md). Both hotfixes are embedded; do not carry the old separate overlay
onto the new build. Windows packaging completed successfully (run 39297183309). Next stage: user check of a save copy.
LS-003 stays Planned until the user's texts and a separate implementation; it does not block the 1.5 port.

Additionally verified 3 migration cycles of a fixture realistic to v0.4 progress structure:
inventory/XP/skill points/quest bits/NPC overrides/bank/reserve/visited maps are preserved.
The test commit did not change the executable of the first run cbaea4b; the successful later 1.5 package is bad9c27.

The package check was aligned with the 1.5 contents (engine+MM6/MM7/MM8); it verifies the x64 PE and the current
hotfix resources in engine.zip. The first run 39287322676 ended at an outdated Verify step,
not at compilation. Retry 39297183309 finished successfully; hotfix resources inside the package were compared with the checkout.

Windows artifact: [OpenYAMM-windows-Release.zip](https://github.com/Plane-PPP/openyamm/actions/runs/39297183309/artifacts/12211627532).
The new build has not yet been checked by the user; carry save copies into a separate installation.

## OpenYAMM 1.7 integration — 2026-10-22

Exact v1.7 merged; explicit production 1.5 save migration and embedded hotfixes retained.
25 focused tests / 30394 assertions passed. [Compatibility](research/openyamm-1.7-compatibility.md).
Luck implementation, final regressions and Windows packaging completed; next is user save retest.

Luck bonus implemented: +0.5 percentage points per full 10 units at all ranks up to KL/cap.
The dynamic tooltip and trace use the shared calculation. The CSV includes Luck 0/90.
Checks and 1.7 packaging completed; a manual run is expected.

Automatically verified on 1.7+Luck: 26 focused/30530, 3 runtime/58, 223 selected/7506.
Windows packaging completed; the user's save has not yet been provided.

CI adapted to upstream 1.7 release target/launcher/indexed content and Python YAML dependency.
The Windows job result is recorded below after actual completion.

Windows run 39830857374: native compile/link passed, packaging failed because CMake
selected hosted Python without PyYAML despite MSYS2 Python YAML being installed.
Retry pins Python3_EXECUTABLE to UCRT64 and checks import before configure.

Native MinGW launcher now links its C++ runtime statically: root launcher must not depend
on runtime DLLs located only beside the game in app/<version>. Retry 39894785306 stopped early
to include this packaging fix before compilation; gameplay code unchanged.

Final tooltip review removed duplicate English Luck prose (already supplied by skill_des.txt);
Russian custom description retains the same rule. Formula/save code unchanged.

## Windows 1.7 + Luck — Packaged / manual retest pending

[Run 39898467800](https://github.com/Plane-PPP/openyamm/actions/runs/39898467800) completed successfully: 2026-10-23T04:35:10Z.
Source `a1b8debf0198779d1ea418555c50bb50c21e8028`; previous confirmed 1.5 `bad9c2798b3320f16a108661c588d536b98770da`.
[Download OpenYAMM-windows-x64-Release.zip](https://github.com/Plane-PPP/openyamm/actions/runs/39898467800/artifacts/12478309627), 5,750,960,934 bytes.
Windows compile/link, versioned package, x64 launcher/game PE, all indexed content hashes,
MM6/MM7/MM8 presence and exact embedded teacher/Stats resources passed; artifact uploaded.
No old hotfix overlay is required. User manual playthrough and actual save remain untested.

## Port to the latest official version — 1.10

Before integration, a production fixture of the mod's 1.7/v94 format was saved.
Next: merge exact v1.10, verify 1.5/1.7 saves and existing mechanics, build for Windows.

Exact v1.10 integrated; the new schema 95 keeps support for mod 89/94.
[Report](research/openyamm-1.10-compatibility.md). Next: checks and Windows CI.

Preflight 1.10: 26 focused/30630, 3 runtime/58, 226 selected/7531 passed.
Temporary Linux dependency cache is being restored after the host restart.
Windows CI uses a clean checkout; the additional local save test subsequently passed.

All final local 1.10 checks passed: 27 focused/30639, 3 runtime/58, 227 selected/7540.
The test executable was incrementally rebuilt/relinked; a full Linux rebuild is not claimed.
Windows run 39941527807 source 13d2cf3 finished successfully; artifact verified. Next: manual run.

## Windows 1.10 — Packaged / manual retest pending

[Run 39941527807](https://github.com/Plane-PPP/openyamm/actions/runs/39941527807) completed successfully at 2026-10-23T11:18:35Z.
Source `13d2cf3bc1e1b0c005a15f4a253bc0dba875522b`; previous confirmed package was 1.7/a1b8deb.
[Download OpenYAMM-windows-x64-Release.zip](https://github.com/Plane-PPP/openyamm/actions/runs/39941527807/artifacts/12498430051), 5,751,530,172 bytes.
Compile/link, launcher/game x64 PE, new model-emissive shader, indexed content hashes,
MM6/MM7/MM8 presence and exact embedded teacher/Stats resource comparison passed.
Artifact uploaded. Manual Windows gameplay and the user's actual save remain untested.
Use a separate install, copy only saves, keep originals and save in a new slot.

## Incremental ZIP updates — researched 2026-10-23

The user chose ZIP patches. [Original architecture and plan](research/minotaur-incremental-updates.md).
The 1.10 base was saved from the published ZIP using HTTP ranges (221884 bytes instead of 5.75 GB).
Next stage: patch generator, Windows installer, CI and checks; game mechanics do not change.

Incremental release implemented: generator by content_id, SHA checks, Windows PowerShell 5.1
installer with atomic switching, a version per build, update/full/both in CI.
Locally 7 Python tests passed; 8 Windows scenarios await CI.
Next: Windows check of the installer, the first real ZIP patch and publishing the new base.

Windows tests 39960808276 found Get-FileHash unavailable in a child PowerShell 5.1.
The installer was moved to standard .NET SHA256 with no dependency on a PowerShell module. A CI retry is expected.

Retry 39961282682 exposed an incorrect backup argument to File.Replace under Windows 5.1.
A real temporary path is now passed for atomic replacement; the backup file is removed after success.
CI runs the child 5.1 with its standard module directory and no interactive prompts.

Removing the temporary backup after a successful atomic replacement is best effort: a cleanup error
must not report failure of an already activated version.

## First ZIP patch — Packaged / manual retest pending

[Run 40000571371](https://github.com/Plane-PPP/openyamm/actions/runs/40000571371) completed successfully on 2026-10-23. Source `b7e80f0583df4e594718cb3de63f87c4a0793fc7`.
[Download the first patch](https://github.com/Plane-PPP/openyamm/actions/runs/40000571371/artifacts/12522948534): 20,316,780 bytes (20.3 MB) instead of
the 5,751,530,172 bytes of the full 1.10, a 99.65% download reduction.
Base `minotaur-1.10` → `minotaur-1.10-b7e80f0583df-14-1`.
131 packs reused (5,766,643,023 bytes on disk),
0 changed resource packs. The program and installer are delivered.
Windows: 15 tests passed, including 8 Windows PowerShell 5.1 scenarios.
Validator full/thin checks passed separately.
Compile/PE/shaders/hashes/embedded teacher and Stats fixes verified.
API size/SHA256 of the artifact match the builder's report. The full ZIP was not independently downloaded.
Manual game launch and installation into a real user folder have not been verified.
The new base is saved in the repository; the next build uses it by default.

1.13 validation: first Windows game build passed; unit build exposed an old Axe test-world
method signature, now adapted to shield-tier API. 17 updater tests passed. Added explicit
EnergyGuns/LethalStrike coexistence save test. Retry remains required before publishing.

1.13 second validation: all game/unit sources compiled; the Windows unit link required
winhttp/bcrypt, now declared for the standalone unit target. Gameplay test results remain
pending until the next run actually executes them.

1.13 gameplay checks executed: 29 applicable cases passed in run 40318306385. The broad Axe
name filter also picked an unrelated MM9 Bloodaxe quest requiring unshipped MM9 assets;
exclude MM9* from the MM6/MM7/MM8 mod job and retry packaging.
