[Русский](undead-condition-immunities.md) | **English**

# Immunities of player Liches and Vampires: MM7, MM8 and OpenYAMM

Date: 2026-10-31. Status: research complete; fixes NOT implemented.
Reviewed branch feature/minotaur-rebalance, HEAD 07e79ebb5b5b34c45dc1c27070b909cba10c8b7d,
and official upstream v1.20, 1add1d374848648887c08707199921335b4ba440.
No build, code/balance/resource/CI change or save migration was performed.

## User observation and practical conclusion

A Naga blow poisoned a Vampire and a Lich; the Lich was obtained through the real MM8 quest promotion.
This is a combat special effect, not a direct condition assignment by an object script.

- Vampire: no innate poison immunity is intended; being poisoned is not a bug in itself.
- Lich: a specific OpenYAMM defect was found — the transformation grants Light/Dark instead of Mind/Body.
  This leaves the Lich without the expected Body protection and explains its vulnerability to poisonous blows in this implementation.
- Disease is not poison: for special attacks, Disease is defended by Endurance/Luck, not Body.
- Immunity to a school's damage, resistance to a special effect and a direct ban on a condition are three different checks.
  No disease or poisoning of undead may be declared a bug outright, and no blanket ban on all conditions may be introduced.
- Even a correct restoration of Mind/Body does not prove that original MM7/MM8 never allowed
  Poison/Fear from a special attack. A policy of absolute condition immunities needs a separate decision.

## What the original sources confirm

The [original MM8 manual, Resistances section](https://manualmachine.com/gamespc/mightandmagicviiidayofthedestroyer/1119628-user-manual/)
promises the player Vampire full immunity to Mind. Body/poison is not listed as an innate immunity for them.
Vampire/Nosferatu are player classes of MM8; MM7 has no corresponding player class, though it has Vampire monsters.
Their monster tables must not be transferred to the hero.

The [GrayFace MM8 Patch changelog](https://grayface.github.io/mm/MM8PatchReadMe/), section 2.5,
separately fixes the non-working Vampire Mind immunity. So "that is how the unpatched game behaved" does not always
mean the intended rule. This changelog does not declare Vampires immune to poison or disease.

[GrayFace: MM7/MM8 mechanics](https://grayface.github.io/mm/mechanics/#Chance-To-Hit) separates resistance
to special attacks: Poison/Dead/Eradicated use Body; Afraid/Insane/Paralyze use Mind; Disease/Sleep/Weak use the
Endurance modifier. Luck also takes part. The stated formula for a successful effect after an attempt is:
30/(30+LuckEffect+OtherEffect). This is not a per-swing poisoning probability and not a magical damage formula.

For MM7/MM8 Liches the expected set is Mind/Body, not Light/Dark; the four elemental resistances at
transformation are raised to at least 20. Direct evidence of the transformation mechanism:
[MMExtension ChangePlayerClass](https://grayface.github.io/mm/ext/ref/), where the Lich flag triggers the face change and
grants immunities. The [primary GrayFace hook](https://github.com/GrayFace/MMExtension/blob/9d73c0ff0482bd231ea522f752f4fe9b26640925/Scripts/Core/events.lua#L2071)
distinguishes an ordinary class change from the transformation. The exact Mind/Body set additionally agrees with
an [observation by a player of original MM8](https://www.reddit.com/r/MightAndMagic/comments/vqjvjo/), who compared
the promotion of necromancers with the ready-made Hevatia/Vetrinus. That source is a user observation, not
a disassembly and not sufficient verification of all conditions on its own. The same message notes
zero base resistances of the two ready-made Liches; this must not be mixed with the current promotion case.

## Why "undead never get Poison/Disease/Fear" cannot be written

The [reference code of the OpenEnroth MM7 reconstruction](https://github.com/OpenEnroth/OpenEnroth/blob/a644b752f05af0cdbb1c5d96d120d6967bd8b790/src/Engine/Objects/Character.cpp)
was checked: CalculateIncommingDamage, ReceiveSpecialAttackEffect, GetActualResistance, SetCondition.
The separate special-attack path takes the numeric resistance and makes a saving throw; it has no class-Lich
condition ban. GetActualResistance limits a Lich's value to 200. So for this reconstructed path,
at Body 200/LuckEffect 0, 30/230≈13.04% remains after a special-effect attempt, before separate item/buff protection.
This illustrates the difference between mechanisms; it is NOT a measured probability of original MM8 and NOT a claim about
the accuracy of every detail of current OpenEnroth. Its damage calculation still has TODOs about types/Spirit.

The [reconstruction's condition protection table](https://github.com/OpenEnroth/OpenEnroth/blob/a644b752f05af0cdbb1c5d96d120d6967bd8b790/src/Engine/Conditions.cpp)
checks items and Protection from Magic separately. Direct SetCondition also gives no general class-Lich
ban. Immunity to magic must not be turned into a promise of universal condition immunity.

Research limitation: the local reference/OpenEnroth-git, mmext-scripts and re_mm8/mm8_orig/MM8-Rel.exe
are absent. The [Rizin guide](../../re_mm8/RIZIN_GUIDE.md) was read; the binary check could not be performed.
Under project rules, public OpenEnroth was used only after confirming the local reference was absent.
Original MM7/MM8 were not run in this research; neither retail, GOG nor GrayFace are marked as
reproduced. The native accuracy of immunity from a specific Naga special attack in MM8 remains open.

## Short rule matrix

| Character/effect | What is established |
|---|---|
| MM8 Vampire/Nosferatu: Mind | Immunity intended; the original had a defect fixed by GrayFace |
| MM8 Vampire/Nosferatu: Poison | No innate full Body/Poison protection; poisoning is acceptable |
| MM8 Vampire/Nosferatu: Disease | No general immunity; the combat save uses Endurance/Luck |
| MM7/MM8 Lich: schools | Mind/Body expected; not Light/Dark and not immunity to all elements |
| Lich: Poison/Fear/Insane/Paralyze from a special attack | Body/Mind take part; an absolute ban on all original paths is not confirmed |
| Lich: Disease/Sleep/Weak | A single Mind/Body immunity does not block these conditions |
| Any undead: scripted object effect | The path assigning the condition needs a separate check |
| Lich/Vampire monsters | Separate monster definitions; not a source of player character immunities |

## Confirmed implementation in OpenYAMM 1.20

The links below are pinned to the reviewed revision; the paths in the working tree are the same.

1. [Party::applyLichState](https://github.com/Plane-PPP/openyamm/blob/07e79ebb5b5b34c45dc1c27070b909cba10c8b7d/game/party/Party.cpp#L1083)
   sets four elemental base≥20, light/dark≥65000 and permanentImmunities.light/dark=true.
   Mind/Body and permanentConditionImmunities are not granted. setMemberClassName calls this on a real
   transformation (classOnly=false); applyLichTransformation does too. This matches the user's specified path.
2. [Start of the history](https://github.com/pjasicek/openyamm/commit/5bace84609e90da75a6c8fac421a4a60722b4ea5):
   these Light/Dark flags were introduced upstream on 7 May 2026 and are present in official 1.20.
   The latest Minotaur port did not add them. In tests/ScriptedMapRegressionTests.cpp around 5667 an old test even
   asserts light 65000/light+dark immunity — the test locks in this mistaken semantics rather than refuting it.
3. [Stats/damage](https://github.com/Plane-PPP/openyamm/blob/07e79ebb5b5b34c45dc1c27070b909cba10c8b7d/game/gameplay/GameMechanics.cpp#L2548):
   Mind/Body Infinite come from the corresponding permanent/magical flags. className=Lich or
   raceId=Vampire alone does not create them in Standard. The number 65000 in another field does not replace the correct flag.
4. [Combat path](https://github.com/Plane-PPP/openyamm/blob/07e79ebb5b5b34c45dc1c27070b909cba10c8b7d/game/gameplay/GameplayCombatController.cpp#L1010):
   monsterSpecialAttackSaveBonus and memberImmuneToMonsterSpecialAttack choose the relevant school. Poison of all three
   degrees is blocked by Body.infinite; Fear/Insane/Paralyze by Mind.infinite. Disease is not on this list.
   Then applyMonsterSpecialAttack checks other protections and calls applyMemberCondition.
5. [Party::applyMemberCondition / hasConditionImmunity](https://github.com/Plane-PPP/openyamm/blob/07e79ebb5b5b34c45dc1c27070b909cba10c8b7d/game/party/Party.cpp#L1183)
   check condition flags/items/effects, with no general undead/Mind/Body check. EVT assignments go
   directly here: EventRuntime.cpp around 3381/3680. This is a different path from the confirmed Naga blow.
6. Creating a new character resets permanentImmunities/conditionImmunities in GameApplication.cpp:2762.
   For Standard no later path granting Vampire Mind immunity was found. RosterEntry copies numeric
   resistances, not class immunities. For example, Elsbeth has Mind 10/Body 10; Gethric/Infaustus/Artorius 0/0;
   Hevatia/Vetrinus 0/0. These are data of the project's shared MMerge table, not evidence of a vanilla roster.
7. MAW is checked separately: MawClassRules.cpp:85–104 already contains racial Undead→Body+Mind and Vampire→Mind,
   granted through magicalImmunities in applyClassBonuses. This does not work as a fix for Standard and must not be
   summed again in future work. MAW conditions/legendary items have additional rules.

### Nagas specifically in the reviewed shared table

assets_dev/engine/data_tables/monster_data.txt:

| ID | Name | Level | Special |
|---|---|---:|---|
|169|Naga|20|Poison1x2|
|170|Naga Warrior|28|Poison1x3|
|171|Naga Queen|38|Poison2x4|

This is a physical blow with a separate poison effect. shouldApplyMonsterSpecialAttack uses Level×multiplier,
with the attempt limited to 100%; then come immunity/save/items/Protection from Magic. In the base table the attempts are
40%, 84%, 100%; these are NOT final poisoning chances, and they change with runtime level/bolster.
A promoted Lich in the current code has Body.infinite=false, so the innate protection check does not stop
this path. A Vampire also has no Body immunity, but for them this matches the expected set of schools.

## Recorded defects and next checks — not implemented

- UND-001: Lich promotion assigns Light/Dark instead of Mind/Body; inherited from upstream. Fix the correct
  schools and check magical damage, Stats and combat conditions separately. Do not grant Disease immunity "along the way".
- UND-002: Standard Vampire/Nosferatu do not receive the promised Mind immunity; do not confuse with acceptable Poison.
- UND-003: pre-made Liches and old saves cannot be fixed by just a new promotion call;
  the origin of incorrect permanent Light/Dark values has to be considered. Do not erase legitimate item
  and event bonuses without an explicit verifiable migration. Nothing in existing saves has been changed now.

Before coding, choose the target compatibility: reproducing unpatched-game bugs, corrected
class schools, or additionally absolute protection from condition special attacks. The last option must not be
called an automatically confirmed vanilla rule: current OpenYAMM Infinite blocks the special effect
without a roll, while the MM7 reconstruction shows a numeric save. A focused check of original MM8 is needed.

Minimum check: an ordinary Necromancer, the same after the quest, a hired Lich; Vampire and Nosferatu; Standard separately
from MAW. Check pure Body/Mind damage, Poison 1/2/3 and Fear/Insane/Paralyze through a real blow, Disease separately;
then item immunities, Protection from Magic, EVT effects and save/load. For the Naga, record the actually
used Body/Mind, the Infinite flags and the source of the condition. Do not add separate bans on sleep/weakness/disease/curse
on undead lore alone.
