#pragma once

#include "game/content/DungeonRun.h"
#include "game/rules/CombatRules.h"

#include <cstdint>
#include <string_view>

namespace OpenYAMM::Game
{
struct Character;
class Party;

// The effects of a dungeon run's talents (run.yml `talents`; adapted from the talents of a MAW solo run for a party
// of four: all passive). Each function reads the hero's rank (Character::talentRanks) and the rank's numbers from the
// rules, and does nothing for a hero without the talent. The run's rules call them from the shared hooks of combat,
// spells and classes (RoguelikeRules.cpp); a MAW solo run keeps its own talents (game/rules/maw/MawSoloTalents.h).
// The timed parts (Onslaught's haste, the cut after Cheat Death, Spell Surge's count, Binding Circle's cooldown) live
// in Character::talentState and are not saved; Cheat Death's use in a dungeon is (RunCheatDeathStopVariable).
namespace RunTalents
{
inline constexpr std::string_view SweepingBlows = "sweeping_blows";
inline constexpr std::string_view Onslaught = "onslaught";
inline constexpr std::string_view ShieldWall = "shield_wall";
inline constexpr std::string_view Multishot = "multishot";
inline constexpr std::string_view PiercingArrows = "piercing_arrows";
inline constexpr std::string_view PinningShot = "pinning_shot";
inline constexpr std::string_view Attunement = "attunement";
inline constexpr std::string_view Overload = "overload";
inline constexpr std::string_view SpellSurge = "spell_surge";
inline constexpr std::string_view BindingCircle = "binding_circle";
inline constexpr std::string_view Resolve = "resolve";
inline constexpr std::string_view Bloodthirst = "bloodthirst";
inline constexpr std::string_view Unbroken = "unbroken";
inline constexpr std::string_view CheatDeath = "cheat_death";

// The talent and the hero's rank of it, when the hero has a rank of a talent the rules know.
struct Learned
{
    const DungeonRunTalent *pTalent = nullptr;
    int rank = 0;

    explicit operator bool() const
    {
        return pTalent != nullptr && rank > 0;
    }

    float value() const
    {
        return pTalent != nullptr ? pTalent->value(rank) : 0.0f;
    }

    float value2() const
    {
        return pTalent != nullptr ? pTalent->value2(rank) : 0.0f;
    }
};

Learned learned(const DungeonRunRules &rules, const Character &hero, std::string_view id);

// Sweeping Blows, Multishot and Piercing Arrows: what the hero's ordinary swing or shot does besides its own.
CharacterAttackExtras attackExtras(const DungeonRunRules &rules, const Character &attacker);
// Onslaught: a kill starts the haste, or renews it.
void noteKill(const DungeonRunRules &rules, Character &killer);
// Onslaught: the share of a weapon attack's recovery the hero keeps while the haste lasts (1: none).
float attackRecoveryScale(const DungeonRunRules &rules, const Character &attacker);
// Bloodthirst: the hit points a hit of `damage` gives back to the hero who dealt it.
int leech(const DungeonRunRules &rules, const Character &attacker, int damage);
// Pinning Shot: the seconds an arrow's hit slows its target for (0: none).
float pinSeconds(const DungeonRunRules &rules, const Character &shooter);
// What is left of a hit on the hero: Shield Wall with a shield in hand, Unbroken, and the cut after Cheat Death.
int absorbDamage(
    const DungeonRunRules &rules, const Character &member, bool shieldInHand, int damage, int maximumHealth);
// Cheat Death, after everything else took its share: a hit that would fell the hero (hit points `currentHealth`)
// leaves a share of the maximum instead and starts the cut, once in a dungeon (`stop`: its number in the run).
int cheatDeath(
    const DungeonRunRules &rules, Character &member, int damage, int currentHealth, int maximumHealth, int stop);
// Shield Wall: the chance, in percent, that the hero with a shield in hand takes a blow meant for another.
float coverChance(const DungeonRunRules &rules, const Character &member, bool shieldInHand);
// Attunement, Overload (an area spell into a crowd: `monstersNearTarget` counts the target too) and a surging spell.
double spellDamageScale(
    const DungeonRunRules &rules, const Character &caster, uint32_t spellId, int monstersNearTarget);
// Attunement and Spell Surge: what the spell costs the hero.
int manaCost(const DungeonRunRules &rules, const Character &caster, int cost);
// Spell Surge: whether the hero's next spell is the free one.
bool surgeReady(const DungeonRunRules &rules, const Character &caster);
// Spell Surge: a cast is counted; the free one starts the count again and leaves its strength on the spell in flight.
void noteCast(const DungeonRunRules &rules, Character &caster);
// Resolve: its resistances, added to the hero's magical bonuses when the party's numbers are worked out.
void applyPassives(const DungeonRunRules &rules, Character &member);
// Resolve at the ranks that say so: no special attack of a monster takes hold on the hero.
bool immuneToSpecialAttacks(const DungeonRunRules &rules, const Character &member);
// Binding Circle: the seconds the monsters around the struck hero are held for, when the circle is ready; it then
// rests for its cooldown. 0: no circle now.
float springBindingCircle(const DungeonRunRules &rules, Character &member);
// Time passes: the haste, the cut, the surge's strength and the cooldowns run out.
void update(Party &party, float deltaSeconds);
}
}
