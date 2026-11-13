[Русский](MINOTAUR_REBALANCE.md) | **English**


## OpenYAMM 2.3 — integration, checks in progress

Target: `f4a6b8032759a218505532b2600ac56b6d80c7af` (v2.3). Project Medusa and the portrait/paperdoll
importer changes are ported while retaining the current C2 party creation sheet. Medusa content is pinned to
`347fb2185a29dfd3d2eca47e61f2a13e476cf65b`: Mount Nighon, `out10.odm`, position `3114, -11055, 513`.
The new `OYMINO1/v180` envelope maps to upstream179; previous production155 explicitly maps to154.
A frozen155 fixture was captured with the previous writer before merging. Lethal Strike, Axe and Minotaur DR
retain their agreed formulas; inherited upstream behavior is listed in the [report](research/minotaur-2.3-integration.md).
The Linux game and both test targets build. Focused unit profiles:269 cases/49,765assertions; runtime:26/3,328.
These are selected profiles, not the full suite. No Windows package or manual/GPU verification is claimed.
The Roguelike skeleton is ported on a separate branch:8 cases/599assertions and the original48-step scenario passed.
Run rules are unchanged. [Skeleton handoff](research/minotaur-2.3-roguelike.md).

## OpenYAMM 1.21 — Windows ZIP ready

Official `c24a0aaa6bb6f7956c84beed1bd44c88e62453c4`; balance preserved. Save mod155/official154,
historical mod143 maps to official142. The new creation sheet shows innate Lethal Strike.
[Audit, conflicts and verification scope](research/openyamm-1.21-integration.md).
Local unit258/258,49,851assertions; runtime7/7,163assertions passed. Windows CI passed; private campaign saves have not been manually tested.
Package `minotaur-1.21-c9949bbc7301-28-1`, source `c9949bbc7301240adda39e1503ff2985729d470b`.
[ZIP, measured resources and installation](research/windows-1.21-build.md).

> **Previous summary as of 2026-11-02.** Base OpenYAMM v1.20, mod save format 143 (official 142), Lethal Strike has ID 49.
> The sections below were written stage by stage, and earlier values belong to their own version. Feature status: [ROADMAP](ROADMAP.en.md).

## Historical OpenYAMM 1.20 — Windows ZIP packaged

Official 1.20 merged; Minotaur balance unchanged. Reagent/bottle stacks of 50 retain native multi-cell geometry.
Only insufficient HD footprints are enlarged, per user approval; icons are never shrunk for stacking.
Safe old-layout repair retains overflow. Mod 143 reads prior mod 140/132 and official schemas.
[Integration, compatibility and test scope](research/openyamm-1.20-stack-geometry.md).
[Successful Windows ZIP and installation](research/windows-1.20-build.md). Manual campaign/visual retest pending.

## Historical 1.18 package

## Current OpenYAMM 1.18 update

Merged official 1.18 without changing Minotaur balance. Ordinary reagents stack to 50;
RMB supplements show actual engine stat thresholds and armor/weapon Recovery.
Production 1.14 v132 fixture loads; new writer is mod 140/upstream 139.
156 unit/regression cases and 3 native Lethal Strike cases passed on Linux and Windows.
[Windows 1.18 package and measurements](research/windows-1.18-build.md): 137 MiB update, 9 changed packs, 123 reused.
Manual campaign/save/UI retesting remains pending; Minotaur balance is unchanged.
See [current notes](minotaur-1.18.md) for controls, compatibility, known boundaries and manual checks.

## Packaged shared gameplay fixes (2026-10-27)

Alchemy bonus calculation, Levitate hazard protection and Mind Resistance duration passed selected automatic tests
locally after the 2026-10-27 interruption. No Lethal Strike, Axe, Damage Resistance or save schema changes.
See [investigation](research/2026-10-27-gameplay-fixes.md) and
[Windows package/report](research/windows-gameplay-fixes-build.md). Windows packaging and 81 selected tests
are complete; manual gameplay testing remains pending.

# Initial OpenYAMM 1.14 port / Minotaur v0.4 + Luck — previous package

New official 1.14 appeared during 1.13 validation; exact 1.14 now supersedes that unpublished
intermediate port. That initial update targets installed `minotaur-1.10-b7e80f0583df-14-1`.
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

# Minotaur Rebalance — mod description

## Base at the time — OpenYAMM 1.10 / mod v0.4 + Luck

Exact v1.10: 3f06332001c7a841cc2c2d161f192a1252b25713. The mod's balance does not change.
[Save compatibility with 1.5/1.7](research/openyamm-1.10-compatibility.md): new record OYMINO1/v95.
The Luck bonus and the teacher and Stats fixes are kept. [Download Windows 1.10](https://github.com/Plane-PPP/openyamm/actions/runs/39941527807/artifacts/12498430051). CI succeeded;
manual verification of the new build is pending.

## Mod updates — ZIP patches

After a full 1.10 installation, later versions ship as small ZIP patches:
the program and changed resource packs, reusing all other files.
Extract the patch into the game folder, open the Minotaur-Update folder that appears and run
Apply-MinotaurUpdate.cmd. Saves and settings.ini stay in place; there is no need to move them when patching.
[Installation](../packaging/minotaur/README-RU.txt), [details](research/minotaur-incremental-updates.md).
The installer was verified on Windows 5.1; the [first 20.3 MB ZIP patch](https://github.com/Plane-PPP/openyamm/actions/runs/40000571371/artifacts/12522948534) was released for the full 1.10.
All 131 heavy packs are reused in place. A manual gameplay run is still pending.
Game mechanics and the save format did not change. Keep the regular OpenYAMM online update disabled.

## History — OpenYAMM 1.7 / mod v0.4 + Luck

Tag v1.7: 069df4a3f63c2dcaeed7b70694529154e7ccc205. Both fixes are embedded.
All Lethal Strike ranks receive `B=0.5*floor(max(0,Luck)/10)` percentage points to P0.
Current Luck 90 gives +4.5 points; then KL/GIANT and the 30% cap apply.
Axe/DR and the other coefficients are kept. [Migrating 1.5 saves](research/openyamm-1.7-compatibility.md).
[Download Windows 1.7+Luck](https://github.com/Plane-PPP/openyamm/actions/runs/39898467800/artifacts/12478309627). CI succeeded; manual verification is pending.

## History — OpenYAMM 1.5 / mod v0.4

Port to exact tag 1.5: 1414884155362e613977a3444a3c7f81ed4ee047.
Hotfix LS-001/LS-002 is embedded in resources; the mod's coefficients do not change.
[Save compatibility check](research/openyamm-1.5-compatibility.md).
Install the finished [full Windows build](https://github.com/Plane-PPP/openyamm/actions/runs/39297183309/artifacts/12211627532) separately;
carry over save copies, not old assets/settings.
Windows job 39297183309 succeeded; source SHA bad9c27, manual test of the new build pending.
Do not install the old hotfix 1 on the new build. LS-003 is still Planned: texts have not been received.

> **LS-002 fixed by a resource hotfix:** the four HP/SP/AC/DR rows fit inside
> the existing frame, Condition/Quick Spell returned to their original positions, Earth/Mind/Body
> have distinct Y values. Automatic geometry verified; the user confirmed the corrected layout.

The mod is developed in `Plane-PPP/openyamm`, branch `feature/minotaur-rebalance`.
Goal: strengthen Minotaur survivability and offer a choice between fast one-handed axes
and a powerful two-handed blow in MM8. The shared Axe rules apply to every character
who has learned the relevant mastery, not only Minotaurs.
Original MM8 behaviour and the current OpenYAMM implementation are not automatically assumed identical:
behaviour below is confirmed from this fork's sources.

## v0.4 fixes — resource hotfix 1

[Download ZIP](patches/Minotaur-v0.4-hotfix-1.zip), [installation and verification](patches/README.md).
Extract into the openyamm.exe folder from run 38784738765: assets/assets.zip appears, the existing
engine.zip is kept. Fixes teachers and Stats without changing the executable or balance.
Automatic tests 22/30155 passed; the user confirmed the layout and topic appearance; other manual checks are separate.

## Implemented physical resistance

For a character's base level L:

- Minotaur: `R = min(30, 5 + floor(L / 5))` percent.
- MinotaurLord: `R = min(30, 5 + floor(L / 5) + 7)` percent.
- Other classes: R = 0.
- After the ability: `D_after = floor(max(0, D_before) × (100 − R) / 100)`.
- Prevented damage: `D_before − D_after`; rounding is part of that difference.

| Class | Level 20 | Level 50 |
|---|---:|---:|
| Minotaur | 9% | 15% |
| MinotaurLord | 16% | 22% |

Gaining experience without training does not change R. Temporary level modifiers are not used.
Promotion adds 7 percentage points up to the overall 30% cap.
This is a design balance, implemented by commit
`6b87339d57c9c484f92820ec5c2606598681da9d`; it does not change at this stage.

The order is confirmed by `GameMechanics::resolveCharacterIncomingDamage` and the shared combat controller:
projectile protection → item multiplier for incoming physical damage → immunity checks
and resistance/luck → Master Plate (half) or GM Chain (two thirds) → Minotaur passive
resistance → applied to HP. Intermediate integer rounding is kept.
The passive ability does not apply to spells of other types, falls or direct HP changes
that bypass this path. Physical melee/projectile/party-wide hits use the shared calculation.

## Resistance diagnostics

```text
config set minotaur_damage_trace true
config get minotaur_damage_trace
config set minotaur_damage_trace false
```

Setting: `[debug] minotaur_damage_trace=false` by default in `settings.ini`.
File: `logs/combat_trace.log` relative to the working directory; the path is changed with
`[logging] combat_trace_file`. The general `combat_trace` need not be enabled.
Keep `combat_trace_append=true` so toggling settings does not erase the previous record.
Hits are not printed to the console.

`minotaur_damage` contains name/class/level, source and attack type, `before_passive`,
`passive_percent`, `passive_prevented`, `after_passive`, `final_damage`, `hp_damage`, `applied`.
`passive_reached=false` means an early return bypassed the passive ability;
the damage it prevented is then zero. With debug immortality, calculated damage and HP loss differ.
Misses and dodges are not hits and are not recorded by this channel.

Earlier, 7 tests / 1146 assertions and a syntax check of GameApplication.cpp were run.
These are results of a previous stage, not evidence of manual in-game verification or a Windows build.
The available manual trials of Windows build #3 were later confirmed by the user:
[CHANGELOG](../CHANGELOG.md#minotaur-windows-build-3--ручные-испытания). This is not a complete verification of the mod.

## Preparing test characters

Use a separate test save. `player add Minotaur Test` creates a level 1 character
if the party is not full. `goto` takes a numeric map ID, not a file name.
On map 1 (`goto 1`, wait for loading), `event 167` gives each party member 100000 XP per call.
It also changes the Shalwend release quest and adds gold: it is not a neutral XP editor.
Level 20 needs a total of 190000 XP, level 50 needs 1225000 XP.

For training: `gold set 1000000`, `goto 6`, then Assassin's School (`event 187` on map 6).
Select the Minotaur and train to the required level; this hall's limit is 200.
For promotion on map 1: `qbit set 1545`, `event 740`. The flag alone does not change the class.
For skill points, select a portrait and on map 1 repeat `qbit clear 268`, `event 497`:
this gives +2 points at base Luck ≥13. `event 102` raises base Luck by 2 up to the threshold of 16.
At Luck ≥20: `qbit clear 269`, `event 498` gives +5 points.
Local events take priority over global ones; IDs apply in the stated map context.

A new debug session with `[debug] preseed_party=true`, `party_seed_roster_id=29`,
`[startup] start_in_main_menu=false` and an empty `save_file` has Ulbrecht in the second slot:
MinotaurLord, level 50, XP 1225000, Axe GM10, Plate Master7, 0 free points.
ID 5 gives Arius (Minotaur 5), 13 gives Thanys (Minotaur 15), 21 gives Rionel (MinotaurLord 30).
This does not change an already loaded save. The roster has no ready level 20 Minotaur.
Skill level is raised with points; mastery is acquired from teachers separately.
Master/GM usually need rank 7/10 and the previous tier; further requirements depend on the skill.

## Implemented Axe

These are accepted balance decisions of the mod, implemented in the shared OpenYAMM mechanics:

| Tier | Behaviour |
|---|---|
| Normal | The existing Axe attack bonus is kept. |
| Expert | The existing Recovery reduction by the effective skill level of the leading weapon is kept. |
| Master | The existing Axe physical damage bonus is kept; a one-handed axe is allowed in the off hand. |
| Grandmaster, one-handed | Additional −Axe Recovery per melee action. |
| Grandmaster, two-handed | Guaranteed ×2 on the full physical part of the blow against an enemy. |

The old GM Axe in this fork set `halveTargetArmorClass` on a successful percentage check against skill level:
it temporarily halved the target's AC for `Axe × 60` seconds.
The new GM fully replaces that effect. Item AC effects, Sword/Dagger/Mace
and their own checks are unchanged. The new specialization makes no additional RNG roll.

### Two hands and equipment

A one-handed axe (`equipStat=Weapon`) is allowed in the left hand with Master Axe, like Sword with Master
and Dagger with Expert. Below Master, a left-hand request uses the ordinary main-hand variant;
it grants no hidden off-hand permission. A two-handed axe (`Weapon2`) stays a main-hand weapon.
A two-handed weapon excludes a weapon in the second hand; existing skill/class requirements are kept.

Axe+Axe, Axe+Sword and Axe+Dagger are allowed in both orders with the required mastery of each
weapon in the left hand. Equipment is stored in the existing slots; the save format does not change.
Class/race caps are kept: Minotaur (race 3) has Axe up to Master, MinotaurLord up to GM.
Sword is limited to Expert for both, Dagger is available at Expert. So an ordinary Minotaur can
hold Sword in the right hand and Master Axe in the left, but Axe right + Sword left requires Master Sword,
which is unavailable to them. The shared matrix of both orders is tested with given valid masteries,
not by raising class caps.

Mace is allowed only in the right hand: Mace right + Axe left works with Master Axe, while Axe right + Mace left is refused
because `ItemRules::wield` does not allow Mace in the off hand. Covered by automatic tests; not verified in game.

For a manual check, drag the weapon onto the corresponding half of the character doll,
save, load and check both hands and the statistics.

### Recovery

For two weapons the engine uses the slower **base** Recovery, before bonuses.
The main hand wins ties. Bases: Axe 100, Sword 90, Mace 80, Dagger 60 ticks.
So Axe leads Recovery in the listed mixed pairs regardless of hand order.
The Expert bonus comes only from the chosen leading weapon, not from both hands.

For melee:

```text
R_before_GM = base_weapon + shield_penalty + armor_penalty
              − Speed_bonus − Expert_weapon_bonus − Armsmaster_bonus − other_reductions
GM_bonus = effective_Axe if GM and at least one allowed one-handed Axe; otherwise 0
R_after_GM = R_before_GM − GM_bonus
R_final = max(30, R_after_GM)
Recovery_seconds = R_final / 60
```

`effective_Axe` is the learned rank plus existing `skillBonus` (items and other engine
sources); mastery comes from the learned skill. GM 10/20/30 adds −10/−20/−30 ticks.
With a leading Axe the Expert −Axe is kept, so the total contribution of Axe itself is −2×Axe.
Without other modifiers that is 80/60/40 ticks for GM 10/20/30 respectively.
With two Axes the additional bonus stays single, not double.

Speed uses the existing non-linear `parameterBonus` table, not the numeric stat directly.
Armsmaster reduces Recovery by the effective rank, GM Armsmaster by double the rank.
Haste gives the existing 25-tick reduction through `attackRecoveryReductionTicks`;
item and other existing modifiers are kept. The melee floor is 30 ticks (0.5 seconds).
A two-handed axe gets no new reduction: its Expert Recovery stays as before.
The new GM bonus does not apply to spells, arrows or other attack modes.

### Full physical damage of the two-handed GM Axe

Weapon damage includes the weapon's own physical modifier and its dice range.
Might (`parameterBonus`), the existing Master Axe (+effective Axe),
Armsmaster (Master +rank, GM +2×rank), physical enchantments and permanent/magical
melee bonuses are added to it. In a mixed pair the skill damage bonus is determined by the **main hand**:
Master Axe adds the rank when Axe is on the right; an axe on the left adds its weapon damage
but no second Master Axe bonus. The shared Armsmaster applies once. Might and Armsmaster are included before the new multiplier.

In the existing controller, elemental damage is part of the overall damage roll and is then separated.
So the doubling point is **after** existing strike modifiers and the item's multiplier
against a monster kind, but **before** the target's separate resistance checks:

```text
physical_before = max(0, total_melee_damage − elemental_total)
GM_multiplier = 2 for a two-handed Axe with GM and Physical; otherwise 1
physical_after = physical_before × GM_multiplier
final_physical = existing_target_resistance(physical_after)
final_total = final_physical + separately processed elemental components
```

The doubling is integer, with no rounding or chance roll; each physical component
passes defences exactly once. At extreme integer values the new multiplier saturates
to `int` capacity, leaving room for elemental components. Ordinary rounding/random
resistance checks stay as before. AC still affects hitting.

One-handed Axes get no ×2. Magical enchantment components and separate on-hit effects,
spells and independent damage are not doubled. A blow turned Irresistible by an item effect
is also not considered physical. Two-handed Axe Recovery does not change.
Comparison with the triple damage of Dagger is not used as a template: the new mechanic multiplies
the whole separated physical part, and the Dagger rules stay as they were.
The base damage range shown on the character sheet stays before application to a specific target;
the actual doubling is visible in the combat log.

## Axe diagnostics

```text
config set axe_damage_trace true
config get axe_damage_trace
config set axe_damage_trace false
```

In `settings.ini`: `[debug] axe_damage_trace=false` by default. The channel is independent of
`combat_trace` and `minotaur_damage_trace`, and uses the same file sink and path
`[logging] combat_trace_file` (by default `logs/combat_trace.log`).
To keep records when toggling, use `combat_trace_append=true`.
During play, open the file in a text editor; lines are flushed immediately.
A new executable is needed for the new settings commands; the old Windows package does not contain the changes.

One `axe_attack` line combines the damage and Recovery of one melee action with an Axe in either hand:
name/class/level, effective Axe and numeric mastery (1 Normal, 2 Expert, 3 Master, 4 GM),
IDs/names/types of both weapons, `two_handed_axe`, target, hit and `raw_damage`.
Bonuses: `might_bonus`, `armsmaster_bonus`, `weapon_skill_bonus`, `enchantment_total_bonus`,
`physical_enchantment_bonus`, `flat_bonus`, `weapon_min/max`, `target_multiplier`.
Physical damage: `physical_before_gm`, `gm_multiplier`, `physical_after_gm`,
`final_physical_damage`; the magical part is separate in `elemental_before_resistance`.
Recovery: `recovery_before_gm`, the positive reduction `gm_recovery_bonus`,
`recovery_after_gm`, `final_recovery_ticks`, `final_recovery_seconds`.

`damage_resolved=false` means the action did not reach dealing damage to an enemy
(for example, a miss or a swing with no target); zero intermediate fields are then not a hit.
Recovery is recorded for such actions too. The diagnostics only read already calculated data,
make no RNG rolls and do not change the party or combat mechanics.

## Verification status

| Feature | Sources / automatic checks | Manual Windows check |
|---|---|---|
| Passive resistance | Implemented; formula kept, regression tests pass | Build #3: Minotaur against dragonflies on Emerald Island, formula confirmed from logs |
| minotaur_damage_trace | Kept; independence and invariance tested | Logs studied by the user; full diagnostic coverage not claimed |
| Master Axe off-hand | Implemented; pairs, limits and save/load tested | Build #3: two axes, equipment and doll confirmed; mixed pairs/save-load still to check |
| GM one-handed Recovery | Implemented; levels/pairs/Speed/Armsmaster/Haste/floor tested | Build #3: additional bonuses confirmed from logs; full matrix still to check |
| GM two-handed physical ×2 | Implemented; full damage, elemental and resistances tested | Build #3: two-handed axes and ×2 confirmed; all targets/enchantments still to check |
| axe_damage_trace | Implemented; settings/save, real lines and state invariance tested | Axe/Recovery logs used; full manual coverage not claimed |
| Skill texts / documentation package | Updated; Actions #3 packaging succeeded | New Axe texts visible in game; documentation contents of the installed ZIP not separately compared |

Reproduction commands and final results of the current run are in `BUILD-NOTES.md`.
The headless check uses the real shared attack controller; graphics, sound and presentation
do not run. In-game checks of the doll/off-hand, animations, teachers,
interface texts and downloaded Windows assets are still required beyond the results confirmed above.
The current new mechanic is described below: Lethal Strike v0.4. Identify Monster and the bestiary are Deferred.

## Lethal Strike v0.4 — Implemented

> Known limitation, Deferred / Low Priority: the interaction with special protections is not yet clarified.
> [Contract](design/death-strike.en.md), [materials](design/lethal-strike-protection-task.en.md).

Full contract: [design](design/death-strike.en.md). This is the only name of the new skill.
Class Minotaur / MinotaurLord receive Normal 1 for free on top of existing skills. Other classes,
including characters of another profession with the Minotaur race, do not receive it. Creation, seed/roster,
hiring and old saves use idempotent granting; existing values are not reset.
A Minotaur can train up to Master, a Lord up to GM. Promotion does not grant GM automatically.

| Rank | P0 in percent |
| --- | --- |
| N | 0.60×S + B |
| E | 0.90×S + 0.56×V + B |
| M | 1.10×S + 0.64×V + B |
| GM | 1.30×S + 0.72×V + B |

B=`0.5*floor(max(0,currentLuck)/10)` percentage points; Luck 90 gives +4.5 points.
S/V are base Lethal Strike / Perception, without items. Perception rank does not matter;
no Perception means V=0. Identify Monster is not involved. `delta=PL-ML`;
KL=`min(1.4,1+delta/60)` at delta≥0, otherwise `30/(30+1.5*abs(delta))`.
Result=`min(30,P0*KL*KG)`, fractional precision. NORMAL: KG=1 at HP>0;
GIANT: KG=1 only at `0<5*HP<3*maxHP`, otherwise 0. Exactly 60% is refused; below 60% there is no penalty.
One check after an ordinary successful melee with any weapon/Unarmed, including dual wield;
not for shooting/magic/miss/objects/ordinary kill. No immunities by creature nature were added.

**LS-001 fixed by a resource hotfix:** all three topics have the correct schema and load.
The original Windows build without the patch contains the defect. The existing teacher services are kept:

| Lethal Strike | Teacher | Location | Requirements | Visit commands |
| --- | --- | --- | --- | --- |
| Expert | Silk Nightwalker, Expert Perception | Alvar, Nightwalker Cottage | LS 4 Normal, 2000 gold | `goto 3`, wait for loading, `event 31` |
| Master | Robert Morningstar, Master Mace | Garrote Gorge, Morningstar Residence | LS 7 Expert, 5000 gold | `goto 5`, wait for loading, `event 13` |
| GM | Garic Senjac, GM Axe | Balthazar Lair, Senjac's House | LS 10 Master, MinotaurLord, 8000 gold | `goto 35`, wait for loading, `event 225` |

Senjac is available after rescuing the Minotaurs/QBit 23. In a test save copy you can run
`qbit set 23`. Expert/Master at the former Axe teachers Herald/Jasp were removed; their Axe is kept.
The automatic test checks the actual loaded topics, the dialogue action, payment/raising through it
and the GM cap; the user has not yet manually verified the new configuration.

### Quick test in a separate save

Open the console and select the portrait of the desired character:

```text
player add Minotaur LethalTester
player level 50
skill set LethalStrike 10 2
skill set Perception 10 1
config set lethal_strike_trace true
config get lethal_strike_trace
```

`skill set <canonical-name> <level1–63> <mastery1–4>` is an explicit test command that spends no points.
It checks the class/race cap; GM for an ordinary Minotaur will be refused. To test a Lord:

```text
player class MinotaurLord
skill set LethalStrike 10 4
skill set Perception 10 1
```

`player class` changes the selected character's class for testing and does not run the promotion quest.
Also verify ordinary promotion and teachers in game; the old quest commands are listed above.
Raising through Skills must charge the new numeric level in points: 1→2 costs 2, 2→3 costs 3.
Other skills, the free points at the initial grant and LS after promotion are not lost.

For maximum visibility of the experimental mechanic: S20/V20/GM, final cap 30%.
With S10/V10/Expert, RMB must show `Base Lethal Strike Chance: 14.60%`.
Raise LS or Perception and reopen RMB: the line must update. With P0>30, P0 itself is shown;
the 30% limit applies to the final chance. Without the skill there is no positive P0.

Land ordinary melee attacks, including Unarmed, two-handed Axe, two Axes and an allowed mixed pair.
On a trigger you see `<name> delivers a Lethal Strike!`. This is the standard status bar with High priority
for 4 seconds: the next ordinary damage must not instantly overwrite it. The message appears once,
only if the instant death was actually processed. Death from ordinary damage gives no separate message.
The status does not require show_hits=true; with it disabled, LS stays visible.

### GIANT and the exact 60% boundary

Disable bolster for a simple numeric experiment with the existing profile option, or use
the actual maxHP/Level from the trace. Create one Green Dragon (ID 70, base maxHP 700):

```text
actor spawn 70 1
```

Actor index and template ID are different things. The first aimed melee with trace records `actor_index`.
Then use the obtained index N (replace N with the number in the commands):

```text
actor health N 420
lethal chance N
actor health N 419
lethal chance N
```

`lethal chance` reads the real actor and prints HP/maxHP, current Level, category, P0 and chance,
makes no RNG roll and deals no damage. At 420/700 the chance is 0, at 419/700 it is the full min(30,P0×KL).
With bolster, calculate the boundary from the actual maxHP; an exact 60% exists as an integer when maxHP is a multiple of 5.
In real combat the threshold is checked **after** ordinary damage: starting at 420 can still allow LS
if the ordinary blow took HP below 420 in the same action. Compare `hp_after` in the trace, not the starting HP.
Setting HP does not change maxHP, does not revive a dead actor and grants no XP/loot.

Check the NORMAL overrides 28–33, 58–63, 76–81, 91–93, 106–108 at full HP;
Phoenix 130–132 and Devil 220–222 must be GIANT. The rest of the approved registry is in the
[CSV](research/lethal-strike-classification.csv). An unknown/custom template gets NORMAL and a warning in the trace.

### Diagnostics

```text
config set lethal_strike_trace true
config set lethal_strike_trace false
config set minotaur_damage_trace true
config set axe_damage_trace true
```

All are off by default. LS uses the existing sink `[logging] combat_trace_file`,
usually `logs/combat_trace.log`; the general combat_trace need not be enabled.
Format example (numbers/IDs are illustrative, not a record of a measured fight):

```text
[CombatTrace] lethal_strike action_id=17 member_name="LethalTester" member_class="MinotaurLord" member_level=50 skill=10 mastery=4 perception=10 actor_index=9 monster_id=70 monster_name="Green Dragon" monster_level=70 category=GIANT unknown_template=0 hp_before=430 hp_after=410 max_hp=700 luck=0 luck_bonus=0 P0=20.2 KL=0.5 KG=1 chance=10.1 roll=3.25 success=1 death_processed=1 skip=""
```

Miss/ordinary_kill/no_actor_target/no_skill/giant_hp_threshold/death_rejected explain a skip;
`roll=none` means no roll was made. `hp_after` is after ordinary damage, before Lethal Strike.
`unknown_template` comes with a warning. Turning the trace off does not change RNG/damage/Recovery/result.
A successful LS is also recorded under the general combat_trace even without the separate channel.

### Stats and Russian texts

Damage Resistance is below Armor Class: Minotaur 10/20/50 = 7/9/15%; Lord 20/50 = 16/22%;
other classes 0%. The value uses the active combat function and changes after level/promotion.
RMB explains physical damage, the absence of protection from magic/elements and the 30% limit.
Check that nothing overlaps Condition/Quick Spell and that the existing RMB of other statistics work.

```text
config set minotaur_russian_text true
config set minotaur_russian_text false
```

The option translates the new mod texts, not the whole UI. With Expert S10/V10/Luck 0–9 the Russian P0 reads `Базовый шанс смертельного удара: 14,60%`;
the combat message reads `<имя> наносит смертельный удар!`. English remains the default.

### Compatibility and status

> Stale: the numbers below belong to OpenYAMM 1.10. Lethal Strike now has ID 49 and the new mod format is 143.
> The mapping table is `saveSchemaVersion` in `game/maps/SaveGame.cpp`.

Skills are saved under string keys; existing IDs 0–39 were not shifted, LS was added as 40.
The new mod-format 95 with signature OYMINO1 stores upstream 94 state and the cached combat Level.
Production mod 1.7/v94 is read as upstream 93 + combat Level.
Production mod 1.5/v89 is read as upstream 88 + combat Level with journal migration.
The old mod-v85 and official formats up to 94 are read, including both variants of 85;
for official old saves the monster level is restored through bolster. Old executables do not read the new mod format;
use copies of test saves. The ability is added once to old active/reserve characters
at hydration; existing progress is kept.

Automatically tested items and Windows packaging are listed in [BUILD-NOTES](../BUILD-NOTES.md).
New sounds/animations/GUI and LS frequency need a manual run by the user. A new build is not marked
Manually tested until such a run. Identify Monster/its caps/UI and the bestiary are unchanged and Deferred.

## Next build: teacher texts and navigation

The user will compose themed texts by rank. Add separate hints with the name
and location of the next specialist — [LS-003](research/lethal-strike-teacher-dialogue-followup.md). Still Planned, not Implemented.
The hotfix 1 fixes are already in the source resources and will enter engine.zip of the next full package
from feature/minotaur-rebalance; the old hotfix need not be installed separately on the new package.
