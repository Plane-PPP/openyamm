#include "game/rules/roguelike/RoguelikeTalents.h"

#include "game/party/Party.h"
#include "game/rules/roguelike/RoguelikeRules.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace OpenYAMM::Game::RunTalents
{
namespace
{
int wholeNumber(float value)
{
    return static_cast<int>(std::lround(value));
}

// Spell Surge's strength stays on a spell in flight this long when the rules give no time: a missile rolls its
// damage when it lands.
constexpr float SurgeFlightSeconds = 3.0f;
}

Learned learned(const DungeonRunRules &rules, const Character &hero, std::string_view id)
{
    if (hero.talentRanks.empty())
    {
        return {};
    }

    const int rank = runTalentRank(hero, id);
    const DungeonRunTalent *pTalent = rank > 0 ? rules.talent(id) : nullptr;
    return pTalent != nullptr ? Learned{pTalent, std::min(rank, pTalent->maxRank)} : Learned{};
}

CharacterAttackExtras attackExtras(const DungeonRunRules &rules, const Character &attacker)
{
    CharacterAttackExtras extras = {};

    if (const Learned sweep = learned(rules, attacker, SweepingBlows))
    {
        extras.sweepTargets = std::max(0, wholeNumber(sweep.value()));
        extras.sweepArcDegrees = sweep.pTalent->arc;
        extras.sweepShare = sweep.pTalent->share / 100.0f;
    }

    if (const Learned fan = learned(rules, attacker, Multishot))
    {
        extras.fanArrows = std::max(0, wholeNumber(fan.value()));
        extras.fanArcDegrees = fan.pTalent->arc;
        extras.fanShare = fan.pTalent->share / 100.0f;
    }

    if (const Learned chain = learned(rules, attacker, PiercingArrows))
    {
        extras.chainCount = std::max(0, wholeNumber(chain.value()));
        extras.chainShare = chain.pTalent->share / 100.0f;
        extras.chainRadius = chain.pTalent->radius > 0.0f ? chain.pTalent->radius : 1024.0f;
    }

    return extras;
}

void noteKill(const DungeonRunRules &rules, Character &killer)
{
    if (const Learned onslaught = learned(rules, killer, Onslaught))
    {
        killer.talentState.onslaughtStacks = 1;
        killer.talentState.onslaughtSeconds = std::max(onslaught.pTalent->seconds, 1.0f);
    }
}

float attackRecoveryScale(const DungeonRunRules &rules, const Character &attacker)
{
    if (attacker.talentState.onslaughtSeconds <= 0.0f)
    {
        return 1.0f;
    }

    const Learned onslaught = learned(rules, attacker, Onslaught);
    return onslaught ? 1.0f / (1.0f + std::max(0.0f, onslaught.value()) / 100.0f) : 1.0f;
}

int leech(const DungeonRunRules &rules, const Character &attacker, int damage)
{
    const Learned bloodthirst = learned(rules, attacker, Bloodthirst);

    if (!bloodthirst || damage <= 0 || bloodthirst.value() <= 0.0f)
    {
        return 0;
    }

    return std::max(1, static_cast<int>(std::lround(double(damage) * bloodthirst.value() / 100.0)));
}

float pinSeconds(const DungeonRunRules &rules, const Character &shooter)
{
    const Learned pinning = learned(rules, shooter, PinningShot);
    return pinning ? std::max(0.0f, pinning.value()) : 0.0f;
}

int absorbDamage(
    const DungeonRunRules &rules, const Character &member, bool shieldInHand, int damage, int maximumHealth)
{
    if (damage <= 0 || member.talentRanks.empty())
    {
        return damage;
    }

    double amount = damage;

    if (const Learned wall = learned(rules, member, ShieldWall); wall && shieldInHand)
    {
        amount *= 1.0 - std::clamp(double(wall.value()), 0.0, 100.0) / 100.0;
    }

    if (const Learned unbroken = learned(rules, member, Unbroken); unbroken && maximumHealth > 0)
    {
        const double threshold = double(unbroken.value()) / 100.0 * maximumHealth;

        if (amount > threshold)
        {
            amount = threshold + (amount - threshold) * unbroken.pTalent->share / 100.0;
        }
    }

    if (const Learned cheat = learned(rules, member, CheatDeath);
        cheat && member.talentState.cheatDeathSeconds > 0.0f)
    {
        amount *= cheat.pTalent->share / 100.0;
    }

    return static_cast<int>(std::lround(amount));
}

int cheatDeath(
    const DungeonRunRules &rules, Character &member, int damage, int currentHealth, int maximumHealth, int stop)
{
    const Learned cheat = learned(rules, member, CheatDeath);

    if (!cheat || damage < currentHealth || currentHealth <= 0 || maximumHealth <= 0 || stop <= 0)
    {
        return damage;
    }

    const auto used = member.eventVariables.find(RunCheatDeathStopVariable);

    if (used != member.eventVariables.end() && used->second == stop)
    {
        return damage;
    }

    const int left = std::max(1, static_cast<int>(std::ceil(double(cheat.value()) / 100.0 * maximumHealth)));
    member.eventVariables[RunCheatDeathStopVariable] = stop;
    member.talentState.cheatDeathSeconds = cheat.pTalent->seconds;
    return std::max(0, currentHealth - left);
}

float coverChance(const DungeonRunRules &rules, const Character &member, bool shieldInHand)
{
    const Learned wall = learned(rules, member, ShieldWall);
    return wall && shieldInHand ? std::max(0.0f, wall.value2()) : 0.0f;
}

double spellDamageScale(
    const DungeonRunRules &rules, const Character &caster, uint32_t spellId, int monstersNearTarget)
{
    if (caster.talentRanks.empty())
    {
        return 1.0;
    }

    double scale = 1.0;

    if (const Learned attunement = learned(rules, caster, Attunement))
    {
        scale *= 1.0 + attunement.value() / 100.0;
    }

    if (const Learned overload = learned(rules, caster, Overload); overload && monstersNearTarget > 1
        && std::find(overload.pTalent->spells.begin(), overload.pTalent->spells.end(), spellId)
            != overload.pTalent->spells.end())
    {
        const int extra = std::min(monstersNearTarget - 1, std::max(1, overload.pTalent->targets));
        scale *= 1.0 + double(overload.value()) * extra / 100.0;
    }

    if (const Learned surge = learned(rules, caster, SpellSurge);
        surge && (surgeReady(rules, caster) || caster.talentState.surgeDamageSeconds > 0.0f))
    {
        scale *= 1.0 + surge.pTalent->share / 100.0;
    }

    return scale;
}

int manaCost(const DungeonRunRules &rules, const Character &caster, int cost)
{
    if (cost <= 0 || caster.talentRanks.empty())
    {
        return cost;
    }

    if (surgeReady(rules, caster))
    {
        return 0;
    }

    if (const Learned attunement = learned(rules, caster, Attunement))
    {
        const double share = 1.0 - std::clamp(double(attunement.value2()), 0.0, 100.0) / 100.0;
        return std::max(1, static_cast<int>(std::lround(cost * share)));
    }

    return cost;
}

bool surgeReady(const DungeonRunRules &rules, const Character &caster)
{
    const Learned surge = learned(rules, caster, SpellSurge);
    // Every spell number N is free: N - 1 paid casts come before it.
    return surge && caster.talentState.spellCasts >= std::max(1, wholeNumber(surge.value())) - 1;
}

void noteCast(const DungeonRunRules &rules, Character &caster)
{
    const Learned surge = learned(rules, caster, SpellSurge);

    if (!surge)
    {
        return;
    }

    if (surgeReady(rules, caster))
    {
        caster.talentState.spellCasts = 0;
        caster.talentState.surgeDamageSeconds =
            surge.pTalent->seconds > 0.0f ? surge.pTalent->seconds : SurgeFlightSeconds;
    }
    else
    {
        ++caster.talentState.spellCasts;
    }
}

void applyPassives(const DungeonRunRules &rules, Character &member)
{
    const Learned resolve = learned(rules, member, Resolve);

    if (!resolve)
    {
        return;
    }

    const int amount = wholeNumber(resolve.value());
    CharacterResistanceSet &resistances = member.magicalBonuses.resistances;

    for (int *pResistance : {&resistances.fire, &resistances.air, &resistances.water, &resistances.earth,
             &resistances.mind, &resistances.body, &resistances.spirit, &resistances.light, &resistances.dark})
    {
        *pResistance += amount;
    }
}

bool immuneToSpecialAttacks(const DungeonRunRules &rules, const Character &member)
{
    const Learned resolve = learned(rules, member, Resolve);
    return resolve && resolve.value2() >= 1.0f;
}

float springBindingCircle(const DungeonRunRules &rules, Character &member)
{
    const Learned circle = learned(rules, member, BindingCircle);

    if (!circle || circle.value() <= 0.0f)
    {
        return 0.0f;
    }

    float &cooldown = member.talentState.cooldownSeconds[std::string(BindingCircle)];

    if (cooldown > 0.0f)
    {
        return 0.0f;
    }

    cooldown = std::max(0.0f, circle.pTalent->cooldown);
    return circle.value();
}

void update(Party &party, float deltaSeconds)
{
    for (size_t memberIndex = 0; memberIndex < party.members().size(); ++memberIndex)
    {
        Character *pMember = party.member(memberIndex);

        if (pMember == nullptr || pMember->talentRanks.empty())
        {
            continue;
        }

        SoloTalentState &state = pMember->talentState;

        for (auto &[id, seconds] : state.cooldownSeconds)
        {
            seconds = std::max(0.0f, seconds - deltaSeconds);
        }

        state.surgeDamageSeconds = std::max(0.0f, state.surgeDamageSeconds - deltaSeconds);
        state.cheatDeathSeconds = std::max(0.0f, state.cheatDeathSeconds - deltaSeconds);
        state.onslaughtSeconds = std::max(0.0f, state.onslaughtSeconds - deltaSeconds);

        if (state.onslaughtSeconds <= 0.0f)
        {
            state.onslaughtStacks = 0;
        }
    }
}
}
