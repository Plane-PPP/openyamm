# OpenYAMM 1.21 integration — 2026-11-06

## Source and isolation

Start: `0c172ed04667abbb504df54dd37a8f9056cdd206` on feature/minotaur-rebalance.
Backup ref: backup/minotaur-before-v1.21-2026-11-06.
Latest confirmed ZIP baseline: minotaur-1.20-c80ebaa9634d-25-1.
Common ancestor / already integrated official1.20: `1add1d374848648887c08707199921335b4ba440`.
Official annotated tag v1.21 peels to `c24a0aaa6bb6f7956c84beed1bd44c88e62453c4`;
release published2026-11-05. Five upstream commits,1189 changed paths,52 overlap our changes.
Official Windows, Linux, Android, macOS and release scenarios succeeded; latest stable publication succeeded
(an earlier publication attempt failed, then was rerun successfully).

Medusa is a separate active worktree. No files, refs, processes, build outputs or settings there are modified by this task.
Its initial HEAD was0c172ed; any concurrent changes belong to Claude. Shared Git store fetches use maintenance disabled,
no worktree prune/reset/clean or branch switch. Four preexisting untracked research documents are preserved.

## Semantic conflict matrix

| Area / file | Official1.21 | Mod / resolution | Verification |
|---|---|---|---|
| GameplayCombatController | shared primaryStatBonus, monster condition immunity/resistance logic | keep upstream combat refactor and mod DR trace; remove obsolete duplicated bonus table | combat/LS/DR regressions |
| OutdoorWorldRuntime | yelled flee state reset; MM9 hit bookkeeping | retain effective-level migration and LS terminal HP override; invoke new bookkeeping once | indoor/outdoor LS runtime |
| InventoryItemMixingRuntime | brew into bottle instance; retain potion geometry | bottle template plus atomic two-input stack transaction, decrement exactly one each; native potion dimensions | both held orientations, full inventory |
| InventoryBags | tab-aware sorting and explicit OverflowBag255 | preserve native-geometry repair; use official named overflow and takeOverflow API | migration, sort, full inventories |
| SaveGame | official142→154, completions, journals, ledgers, MM9 placements, MAW preferences | mod155→154, explicit historical143→142 row; both summary/full readers use mapping; migration uses decoded sourceVersion | genuine old-writer fixtures and roundtrips |
| SkillRules / MawSkillRules | resistance/save chances in stat inspection | single5-arg API, compact mod Current/Next plus upstream effects, no threshold tables; MAW override retained | inspect and MAW stat tests |
| NewGameScreen / PartyCreationSheet | new party sheet/mastery UI | preserve innate skill grant; LS listed as fixed class skill and Racial group, no optional cost; Master→GM limits | creation tests |
| GameplayItemService / quick lists | carried potion/scroll actions | preserve definition-aware item replacement; reagent/bottle stacks excluded by official use classification | quick-item regressions |
| Installer / AppUpdater | healthy-start cleanup | offline Minotaur retains rollback app/packs; copy user data before switch; numeric official cleanup unchanged | updater + real Windows PowerShell tests |

Eight textual conflict files: GameplayCombatController.cpp, InventoryBags.h, InventoryItemMixingRuntime.cpp,
SaveGame.cpp, OutdoorWorldRuntime.cpp, SkillRules.cpp, MawSkillRules.h, NewGameScreenTests.cpp.
Other overlaps were reviewed semantically. No new balance formula; current v0.4+Luck and Axe/DR behavior preserved.
Alchemy item/hireling and black-potion limits, MindResistance duration and Levitate protections already came from
previous official integration; no duplicate bonus or repeated patch introduced here. Official1.21 adds derived Lich
Mind/Body immunity and removes obsolete saved Light/Dark immunity; keep the official migration and condition checks.

New official UI: party creation, paused potion/scroll lists, teacher ledgers/journal. Monster locomotion, large-actor
passages, indoor rendering and MAW updates are retained from upstream. No MAW rule is copied into Standard.
For future Medusa integration Claude should review NewGameScreen, RulesetDefinition and GameApplication changes;
Medusa is deliberately not merged or rebased here.

## Saves and installation

Frozen unchanged previous production writer fixture: tests/fixtures/minotaur-1.20-v143-bottles.oysav,
SHA256 fa4ab7f14c4cbbb961e5b95d60f1ac088c5090bb81c3334668c5635881c71c60; see sidecar provenance.
Historical mod89/94/95/130/132/140/143 remain distinct from official schemas; unsupported future headers rejected.
New saves use155 and are not promised readable by1.20. Skill49 stays LethalStrike; named skills/stacks retain data.
Native item geometry and saved overflow are unchanged by this update. New upstream fields default via explicit
version gates; migrations run with decoded official schema, never the mod header number.
User private playthrough saves are unavailable: frozen fixtures cannot certify every campaign state.

The patch supports the exact production base minotaur-1.20-c80ebaa9634d-25-1, validated by app/current and index
content hashes. It carries only new program and changed packs. It never replaces saves/settings.ini. Backup failure
prevents switching; original data are untouched. Previous app and packs stay for rollback. A manual external copy of
saves/settings is still useful. Load a copied old save, inspect party/skills/items/quests/location, save to a NEW slot,
restart and reload. Keep old saves separate for rollback. Numeric official updater manifests cannot replace the
minotaur version namespace; its regression remains required. Do not install an official executable over this patch.

## Verification status

Independent read-only review completed, corrected its singleton transaction finding. Local Release game, unit and
runtime targets compile. Selected unit258/258,49,850assertions; native runtime7/7,163assertions passed.
Exact commands: protected `cmake --build build --target openyamm openyamm_unit_tests openyamm_runtime_tests --parallel4`
(with a space between --parallel and4), then the exact `--test-case` filters in minotaur-windows.yml on
build/tests/openyamm_unit_tests and openyamm_runtime_tests. The same selected set runs on Windows.
MM9*,*MM9 RUDE*,*Bloodaxe* and the graphics-only lava shader case are excluded: MM9 data and GPU rendering are
outside this MM6–MM8 ZIP/test scope. These are not counted as passed. Full suite not run.
`python3 tools/check_doc_i18n.py`:3pairs,0errors,4preexisting translated-reference warnings.
`python3 tools/lighting/stale_bakes.py --check`:0stale; sparse fixture scope, not a full art-production audit.
`git diff --check` passes after restoring a zero-length file left by interrupted asset checkout; no engine crash.
[Protected operation metrics](openyamm-1.21-local-resources.csv). Main compilation565.679seconds,4.682GiBpeak,
no swap/PSI-high/OOM; all compilation uses jobs4 and16/18GiB limits. No Claude process stopped.
Windows results and artifact links will be recorded only after actual completion. No new manual gameplay or GPU/Android visual compatibility claimed.
First expanded test run:345cases,301passed/44failed (not a passing result). Sparse-hidden source assets caused
most failures. Two genuine integration mismatches corrected: singleton held bottle result now follows upstream;
its transaction must skip reinserting a removed reagent (caught by independent review), and LS trainer topics
share their already existing teacher location/discovery autonotes176/108/100. No other teacher services change.
Local source asset restoration initially fetched per file; stopped own runner normally (exit130, no pressure) and
replaced with a batch of canonical upstream blobs. This is fixture preparation, not a gameplay fallback.
New upstream saving-throw/resistance supplemental text remains English; compact mod stat inspection is RU/EN.
