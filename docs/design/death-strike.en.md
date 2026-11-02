[Русский](death-strike.md) | **English**

# Lethal Strike v0.4

> **LS-001 fixed in a resource hotfix:** topics load; Expert at Silk Nightwalker
> (Expert Perception), Master at Robert Morningstar (Master Mace), GM at Garic Senjac (GM Axe).
> The original build without the patch keeps the defect. The user confirmed topic appearance and the corrected layout.

Current contract: the user's v0.4 specification of 2026-10-15 with the Luck bonus approved on 2026-10-22. The former design name Death Strike
and the v0.2 formula were replaced; there is no second ability. Old calculations remain in Git history and marked
historical research. Identify Monster and the bestiary are **Deferred**, with no change to gameplay.

## Formula and order

S and V are the **base** numeric levels of Lethal Strike and Perception; items/hired NPCs add no bonuses
to them. Perception rank does not multiply V. Missing Perception gives V=0.

| Rank | P0, percent | Availability |
| --- | --- | --- |
| Normal | `0.60*S + B` | Minotaur / MinotaurLord |
| Expert | `0.90*S + 0.56*V + B` | Minotaur / MinotaurLord |
| Master | `1.10*S + 0.64*V + B` | Minotaur / MinotaurLord |
| Grandmaster | `1.30*S + 0.72*V + B` | MinotaurLord only, after training |

`B=0.5*floor(max(0,Luck)/10)` percentage points at all ranks. Luck is current
Luck (base + permanent + active magical/equipment bonuses, as in Stats),
not parameterBonus. At Luck 90, B=4.5; Expert S10/V10 gives P0=19.10%.
B enters P0 first, then KL and the overall 30% cap apply. Without the skill the chance is 0.
Old reference examples without Luck correspond to Luck 0–9.

`delta=PlayerLevel-MonsterLevel`.
At delta≥0: `KL=min(1.4,1+delta/60.0)`; otherwise `KL=30/(30+1.5*abs(delta))`.
NORMAL: KG=1 at HP>0. GIANT: KG=1 only at `HP>0 && maxHP>0 && 5*HP<3*maxHP`, otherwise KG=0.
The comparison is integer int64; exactly 60% is refused. A wounded GIANT has no probability penalty.
`P_percent=min(30.0,P0*KL*KG)`. There is no intermediate rounding or minimum chance.
One `uniform_real_distribution<double>(0,100)` trial; diagnostics add no RNG.
Identify Monster is not involved. There are no separate promotion bonuses or Master/GM effects.

PlayerLevel includes the existing levelModifier through the engine's shared calculation. Passive resistance
still uses its earlier base formula and does not change. MonsterLevel is the exported
cappedTargetMonsterLevel of the same bolster calculation that scales AC/ordinary damage; without bolster it is
the template Level. It is not a level derived by the classifier from HP.

The check is in the shared GameplayActionController, after ordinary melee damage/resistances
and on-hit effects, before switching the active character. Any allowed weapon, two hands and
Unarmed give one trial. There is no trial for miss, world object, shooting, spells, an already dead target,
death from ordinary damage or GIANT ≥60%. A successful hit with fully resisted damage
stays eligible. Undead/elementals are not automatically excluded.

## Acquisition, training and saves

On OpenYAMM 1.18 Lethal Strike uses ID 49: official Cybernetics takes ID 48,
and IDs 0–47 remain with the original, technology and MAW skills. Previously the ID was 40,
on the 1.13–1.14 ports it was 48. Saved skill names do not change; teacher rows were updated.
The current format OYMINO1/v140 corresponds to upstream 139; production mod 1.14/v132 is explicitly read as 131.
The compact 1.18 record is kept without restoring the baked blood-splat geometry.
[Integration and compatibility results](../minotaur-1.18.md). Character.skills is a string
map, not a fixed array. A new Minotaur/MinotaurLord receives Normal 1 on top of the chosen
skills for free. The shared synchronization applies to creation, seed/roster, hiring, reserve,
hydration of old saves and class change. `try_emplace` does not change an existing rank/mastery;
repeated loading is idempotent. Monsters are not Character and do not receive the skill.
Historical formats: on OpenYAMM 1.10 mod-format 95 has signature OYMINO1 and upstream 94 state.
Production mod 1.7/v94 is read as upstream 93 + combat Level.
Production mod 1.5/v89 is read as upstream 88 + combat Level, then the standard journal migrations run.
The old mod-v85 and official formats up to 94 are read with explicit distinction of the two layouts of 85;
a saved combat Level is not lost, and a missing one is restored through bolster. The string skill map does not change
its structure; LS was absent from the reference v84 fixture and migrates to Normal 1 without losing other skills.
Class availability is set by a new row in class_skills.txt: M/G/None; other caps are kept.
Numeric level growth uses the standard increasing point cost.

| MM8 teacher | Location / house ID | Rank | Skill / gold | New topic |
| --- | --- | --- | --- | --- |
| Silk Nightwalker, NPC 184 | Alvar, house 519 | Expert | 4 / 2000 | 29000 |
| Robert Morningstar, NPC 131 | Garrote Gorge, house 539 | Master | Expert 7 / 5000 | 29001 |
| Garic Senjac, NPC 117 | Balthazar Lair, house 635 | GM | Master 10 / 8000 | 29002 |

The original Perception topic 387, Mace topic 319, Axe topic 311 and other services are kept. These are ordinary additional dialogue
items; promotion/points/gold are checked by the standard MasteryTeacherDialog.
Access is confirmed by map events: out03:event31 → house 519; out05:event13 → house 539;
d24:event225 → house 635 after QBit 23 (rescuing the Minotaurs). These story restrictions are kept.

## Death and interface

The world runtime gets an explicit `killPartyAttackActor`, not artificial damage. It uses the existing
terminal branch: dying/corpse, XP, reputation, MonsterKilled hooks, death sound and drops; a repeated call
on a dead target is refused. No second MonsterDamage/PainReflection/lifesteal is created.
An MM9 actor with canReceiveDamage=false keeps the existing protection; no exceptions by MM8 nature were added.

Skills shows the level and mastery and the standard raise button. RMB contains coefficients,
limits and the dynamic `Base Lethal Strike Chance: XX.XX%` (P0 without the cap).
Success triggers `<name> delivers a Lethal Strike!` through the ordinary status bar, High priority, 4 seconds,
including show_hits=false. Ordinary Normal-priority messages do not overwrite it. Sound/graphics need
a manual check; an automatic test is not a listening/visual acceptance.

Stats contains Damage Resistance directly below Armor Class. The value is taken each frame
from the existing GameMechanics::minotaurDamageResistancePercent; other classes have 0%.
In the hotfix HP/SP/AC/DR occupy four rows of 14 logical pixels inside the existing frame;
Condition/QuickSpell stay in their original positions. Earth/Mind/Body have Y 247.733/265.733/283.733.
RMB explains physical damage only,
the level, the promotion bonus and the 30% maximum.

The project has no shared runtime localization of these UI strings. The option `[debug] minotaur_russian_text=true`
enables the Russian new descriptions/name/combat message and the comma in P0; the rest of the interface
is not translated. English is the default. This is an independent text option and does not change formulas.

## Special protections — Deferred / Low Priority

A known low-priority limitation. From reading the code, Lethal Strike currently bypasses a raised energy shield and `self:setInvulnerable`.

- Intended behaviour: instant death should not bypass story energy shields and real invulnerability.
- The real rules and semantics of these protections must be clarified against current code and published OpenYAMM content.
- The fix is postponed until there is a practical need.
- The MAW ruleset is out of the mod's scope; compatibility with it is not required.

Materials for possible future work: [Lethal Strike and protected targets](lethal-strike-protection-task.en.md).

## Checks and further work

[Testing instructions](../MINOTAUR_REBALANCE.en.md), [BUILD-NOTES](../../BUILD-NOTES.md),
[ROADMAP](../ROADMAP.en.md). Analytics: [v0.4 CSV](../research/lethal-strike-probabilities-v0.4.csv),
[approved classification](../research/lethal-strike-classification.csv).
Automatic check results and the exact Windows SHA are recorded in BUILD-NOTES.
A new Windows build is not considered Manually tested until confirmed by the user.
