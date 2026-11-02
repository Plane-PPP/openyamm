[Русский](independent-audit-2026-11-02.md) | **English**

# Independent audit of Minotaur Rebalance

Date: 2026-11-02. Author: Claude Code. Reviewed: `feature/minotaur-rebalance` at `21f8b78` against
`upstream/main` at `1add1d3` (v1.20). That is the merge base, so the diff below is exactly the fork's patch.
No game code was changed.

## Summary

**No Critical or High defects were found in the mod's code.** Combat mechanics, stacks and tooltips are correct on the
paths reviewed, and the solutions are mostly of good quality. The audit found one inefficiency, several behaviours the
contract leaves undefined, test gaps and noticeably stale documentation. The most serious gameplay defect (Lich
immunities) is inherited from upstream.

## Current state

Updated on 2026-11-02 after the fixes. This section shows what is current. The sections below are kept as first-pass results,
and their conclusions hold only with the corrections in this table.

| ID | Status | Now |
|---|---|---|
| A-1 | Open, postponed | Tooltip cache not added; a performance measurement comes first |
| A-2 | Fixed | Status summary in ROADMAP and the mod description, history preserved |
| A-3 | Fixed | The `/docs/*` rules were removed from `.gitignore`; new documents appear in `git status`, `git add -f` is not needed. Paths outside the sparse patterns still take `git add --sparse <path>` |
| A-4 | Open | Needs a fidelity decision (`UND-001…003`) |
| B-1 | Closed, out of scope | The MAW ruleset is not supported; the mod targets the original MM8/OpenYAMM experience |
| B-2 | Known limitation, Deferred / Low Priority | From reading the code, Lethal Strike bypasses a raised energy shield; the fix is postponed, [materials](../design/lethal-strike-protection-task.en.md) |
| B-2a | Known limitation, Deferred / Low Priority | Found after the first pass: Lethal Strike also bypasses `self:setInvulnerable`; the flag's semantics are not yet clarified |
| B-3 | Open | Low priority |
| T-1, T-2, T-3 | Implemented and verified | New tests passed: 3 of 3, 65 assertions |
| T-4 | Deferred | Together with B-2 |
| T-5 | Closed, out of scope | Follows from B-1 |

### Check on a current binary

The binaries were rebuilt through `safe-heavy-run`: exit code 0, peak memory 1.6 GiB. Only the two changed test files were rebuilt,
so by the build's own data the game object files already matched the sources. Tree: `21f8b78` plus uncommitted tests and documents.

| Set | Result |
|---|---|
| New tests T-1, T-2, T-3 | 3 of 3, 65 assertions |
| CI filter, unit | 192 of 192, 48,820 assertions |
| CI filter, runtime | 7 of 7, 159 assertions |

The full unit suite and running the game were not done. The new tests have not yet run in Windows CI.

### The three tests that failed in the first pass

| Test | Cause | Result |
|---|---|---|
| `lich item inspection speech audio resolves for identify and repair outcomes` | 5 voice files hidden by the sparse checkout | With files temporarily restored: 1 of 1, 7 assertions |
| `legacy lua exporter preserves mm8 reagent item ranges` | `Global.EVT` hidden by the sparse checkout | With the file temporarily restored: 1 of 1, 10 assertions |
| `MM9 Keith Bloodaxe quest acceptance grants the Yobboe quest key before handoff close` | `assets_dev/worlds/mm9` data exists in neither `HEAD` nor `upstream/main` | **Not verified** |

The restored files' hashes matched `HEAD`, and the files were removed after the run. In the ordinary working tree the first two tests
fail because the data is not on disk, and that does not count as a successful check. CI excludes MM9 tests.

`cmake` is not on `PATH`: building through `safe-heavy-run` needs `PATH` and `LD_LIBRARY_PATH` from
`/home/plane/.local/share/openyamm-toolchain/root/usr`. Without them the run ends with code 125 before building.

## First-pass scope

This section is historical: the tests ran on an old binary and do not count as confirmation.

- **Read:** the fork's diff for combat rules, inventory, alchemy, saves and HUD, `.github/workflows/minotaur-windows.yml`
  and the mod documentation.
- **Ran:** only the existing `build/tests/openyamm_unit_tests` (built 2026-10-31 10:20, 7 minutes before commit
  `34a1fc6`; whether its code matches HEAD was not checked) through `safe-heavy-run`, with the filter
  `*Axe*,*Lethal*,*Reagent*,*Bottle*,*Inspect*,*Levitate*,*Mind Resistance*,*Alchemy*`.
  Result: **115 of 118 test cases, 34,802 of 34,809 assertions**.
- **The 3 failures** (`AudioRegressionTests.cpp:837`, `LegacyLuaExportTests.cpp:19`, `Mm9RudeDialogueTests.cpp:274`)
  fail while loading data. This looks like files missing from the sparse checkout, but I did not prove it.
- **Not done:** builds, running the game, visual and manual checks, performance measurements, analysis of `MM8-Rel.exe`.
- **Branch composition:** 65 non-merge Codex commits and 7 merges. The patch covers 136 files excluding `docs/`, of which
  `game/` accounts for 61 files, +2151/−347.

## Confirmed issues

### A-1. Whole party copied every frame in the weapon tooltip — Low/Medium

- **Where:** `game/gameplay/GameMechanics.cpp:4518`, called from `game/ui/GameplayPartyOverlayRenderer.cpp:6317`.
- **What:** `buildItemRecoveryInspectSupplement` runs `Party trial = party` for the equip preview, and it is called from the
  tooltip render function, so on every frame while the tooltip is open.
- **Why it matters:** a deep copy of the party, inventories and inn roster in the render loop to produce one line of text.
- **When:** holding RMB over an unequipped weapon.
- **How to verify:** a profiler or a copy counter with the tooltip open. The frame cost has not been measured.
- **Fix:** cache the string by `objectDescriptionId`, slot and member while the tooltip is open. The calculation itself stays.
- **Test:** optional; a measurement is enough.

### A-2. Mod documentation mixes current facts with stale ones — Medium (documentation)

- **Where:** `docs/MINOTAUR_REBALANCE.md`, section "Compatibility and status" (skill ID 40, save format 95);
  `docs/ROADMAP.md`, section "Confirmed baseline: Windows build #3".
- **What:** the current values are ID 49 (`game/party/SkillData.cpp`), mod save format 143 and baseline
  `minotaur-1.20-c80ebaa9634d-25-1`. Both documents were logs with new entries prepended, so the current state was not
  visible anywhere.
- **When:** whenever a person or agent reads from the top down.
- **Fix:** **done in this task**: a status summary at the top, markers on stale lines, history preserved.

### A-3. New files in `docs/` are invisible to `git status` — Medium (process), fixed

- **Where:** `.gitignore:20-22` ignores `/docs/*` except two files. 51 of the 65 tracked `docs/` files are hidden by the
  sparse checkout.
- **What:** a new document does not appear in `git status` and is not committed without `git add -f`.
  Hidden documents are not found by searching the working tree; they can only be read with `git show HEAD:<path>`.
- **How to verify:** `git check-ignore -v docs/ROADMAP.en.md`.
- **Fix:** **done**, the `/docs/*` rules were removed from `.gitignore`. Correction to the first pass: `git add -f` did not solve this; paths outside the sparse patterns need `git add --sparse <path>`. The patterns were not changed.
- **Test:** `tools/check_doc_i18n.py` warns if documents become ignored again.

### A-4. A Lich receives Light/Dark immunities instead of Mind/Body — Medium, inherited from upstream

- **Where:** `game/party/Party.cpp:1083` (`applyLichState`). The test at `tests/ScriptedMapRegressionTests.cpp:5668` locks
  this behaviour in.
- **What:** a fresh check confirmed `UND-001` from the [research](undead-condition-immunities.en.md). Codex described the
  problem correctly and did not fix it without a decision on the fidelity target.
- **When:** a quest-promoted Lich hit by a poisonous Naga attack.
- **How to verify:** a poisoning hit on a Lich before and after the fix.
- **Fix:** decide the fidelity target, then make a separate change with an explicit save migration (`UND-003`).
- **Test:** required, and the existing test has to change.

## Likely issues needing verification or a decision

### B-1. Mod mechanics partly apply under the MAW ruleset — closed, out of the mod's scope

- **Where:** `game/gameplay/GameMechanics.cpp` (`halveTargetArmorClass` removed for GM Axe),
  `game/rules/maw/MawCombatRules.cpp:815` (its own `resistance` stage), `game/party/Party.cpp`
  (`synchronizeInnateAbilitySpells`).
- **What:** the changes live in shared code. From reading the code, under MAW the two-handed ×2 does not fire, the GM Recovery
  bonus is overridden by `characterAttackRecoveryTicks`, the AC-halving effect is gone, while Lethal Strike and Damage
  Resistance appear to remain. MAW's text for GM Axe (`texts.yml:744`) promises no AC halving, so the removal itself does not
  look like a regression.
- **Why it matters:** the mod's balance is defined only for Standard, and MAW ends up with an undocumented mix.
- **How to verify:** a Minotaur on a MAW profile, combat with `lethal_strike_trace` and `minotaur_damage_trace`.
- **Decision:** MAW is not supported; no fix or test is needed.

### B-2. Lethal Strike kills through a raised energy shield — known limitation, Deferred / Low Priority

- **Where:** `game/outdoor/OutdoorWorldRuntime.cpp:17921`, `game/indoor/IndoorWorldRuntime.cpp:16877`.
- **What:** health is zeroed regardless of the `applyActorShieldHit` result. The contract names only the
  `canReceiveDamage=false` protection and says nothing about shields. No test covers this scenario.
- **How to verify:** a target with a raised shield from `shields.yml`, with a forced successful roll.
- **Decision:** intended behaviour is not to kill; the fix is postponed until there is a practical need, [materials](../design/lethal-strike-protection-task.en.md).
- **Test:** T-4, deferred.

### B-3. A single `overflowItem` slot for several items — Low

- **Where:** `game/items/AlchemyBenchRuntime.cpp:769,785,793,803`.
- **What:** if two items overflowed at once, the second would overwrite the first and an item would be lost. By my analysis
  this is currently unreachable because the remainder returns to the cell just vacated, but that guarantee is implicit.
- **Fix:** an `assert` on an empty slot, or a vector. **Test:** not needed with an `assert`.

## Architectural risks

- **Russian text in code.** `minotaurRussianText` is used in 15 places, the strings are embedded in `GameMechanics.cpp` and the
  renderers, and the setting sits in `[debug]`. This is a workable compromise because these strings have no runtime
  localization. But text in the mechanics layer will get in the way of merges. If more strings appear, move them to a data table.
- **Merge cost.** The mod changes `GameMechanics`, `GameplayActionController` and the save format. The `saveSchemaVersion` table
  (`game/maps/SaveGame.cpp`) needs a new row at every upstream update. The design is right and fixtures catch mistakes, but the
  step is easy to forget, so it has been added to the rules.
- **CI selects tests by name patterns.** A new test with a non-matching name silently stays out of `minotaur-windows.yml`.
  Proposal: a common `Minotaur:` prefix for mod tests.

## Test gaps

| ID | Scenario | What to check |
|---|---|---|
| T-1, implemented | Mace in main hand + Axe in off hand | Allowed with Master Axe; Axe leads Recovery (100 vs 80); Expert and GM reductions count once; no Master Axe damage bonus |
| T-2, implemented | Axe in main hand + Mace in off hand | Refused, because `ItemRules::wield` does not allow Mace in the off hand |
| T-3, implemented | Luck from a worn item | `lethalStrikeLuck` equals the Stats value |
| T-4, deferred | Lethal Strike and a raised shield | The target does not die |
| T-5, closed | Minotaur under MAW | Out of the mod's scope |

## Checked and found correct

- **Damage Resistance.** The formula matches its description, the cap holds, and it applies to Physical only, once.
  A 1-point hit rounds down to 0, a direct consequence of the approved floor.
- **Order of the two-handed GM Axe ×2.** Monster-kind multiplier first, then the elemental part is separated, ×2 on the physical
  part only, an `int` overflow guard, and only then resistances. There is no double application.
- **Recovery.** One GM bonus for two axes, only for `Weapon` and melee, 30-tick floor. The UI has no copy of the calculation: the
  tooltip calls the same function and respects ruleset overrides. This is the best design decision in the mod.
- **Axe + Mace.** Mace in main hand with Axe in off hand works correctly and the reverse order is refused. There is no bug, and
  the behaviour is now documented.
- **Stacks.** Compatibility compares all `InventoryItem` fields, so no property is lost; `identified` is normalized only where
  it carries no meaning. Moves are transactional, and sale and purchase prices are per unit.
- **Lethal Strike.** Probabilities match the contract, with one trial per action and death through the ordinary terminal branch.
- **Saves.** Separating mod and upstream headers and requiring exact end-of-file consumption, especially for the two v85
  layouts, is very robust.
- **Levitate and Mind Resistance.** Hazard protection lives in the shared `Party::applyMovementEffects`; the fork carries no Mind
  Resistance changes of its own any more and uses the upstream fix.
- **Reporting honesty.** Automatic and manual checks are kept apart and assumptions are stated. This is a strength of Codex's documents.

## Style

Preferences, not defects: 25 added lines longer than 120 characters, 13 single-line `if`/`for` without braces, `ReagentStacks.h`
included above the file's primary header 11 times, stray blank lines at the start of `computeHeldInventoryPlacement`, a `static`
counter at `GameplayActionController.cpp:1278`. No mass cleanup is needed; fix them when touching those lines.

## Fit with upstream principles

| Principle | Assessment |
|---|---|
| Simplicity, no needless abstraction | Generally followed; `ReagentStacks` and `InventoryIconGeometry` are compact |
| No duplicated logic | Followed; `placeInventoryIcon` extracted the shared calculation from four upstream copies |
| Fix causes, not symptoms | Followed; geometry and save migrations are explicit |
| Do not edit generated Lua | No violations in the diff; no `.lua` files changed |
| Focused tests | Followed; coverage is strong, especially for saves |
| Code style | Partly; see the section above |

## Fix order

1. A-1: measure, then decide on the tooltip cache.
2. Decide fidelity for Liches and Vampires, then `UND-001…003`.

Lethal Strike and special protections are not in this order: Deferred / Low Priority.
