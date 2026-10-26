#include "game/rules/ClassRules.h"

#include "game/party/Party.h"
#include "game/party/SkillData.h"

namespace OpenYAMM::Game
{
std::optional<int> ClassRules::baseMaxHealth(const Character &, const ClassMultiplierEntry &) const
{
    return std::nullopt;
}

std::optional<int> ClassRules::baseMaxSpellPoints(const Character &, const ClassMultiplierEntry &) const
{
    return std::nullopt;
}

void ClassRules::applyClassBonuses(Character &) const
{
}

bool ClassRules::learnsSpellsFromBooks(const Character &) const
{
    return true;
}

bool ClassRules::isDragon(const Character &character) const
{
    const std::string canonicalClass = canonicalClassName(character.className);
    return canonicalClass == "Dragon" || canonicalClass == "GreatWyrm";
}

PortraitCounter ClassRules::portraitCounter(const Character &) const
{
    return {};
}

std::string ClassRules::displayName(const Party &, const Character &member) const
{
    return displayClassName(!member.className.empty() ? member.className : member.role);
}

void ClassRules::update(Party &, const IGameplayWorldRuntime &, float) const
{
}

bool ClassRules::offersMercenaries() const
{
    return true;
}

uint32_t ClassRules::mercenarySkillPoints(uint32_t level) const
{
    return level > 0 ? level * 5 - 5 : 0;
}

std::vector<uint32_t> ClassRules::mercenaryClassKinds(uint32_t, std::vector<uint32_t> offeredKinds) const
{
    return offeredKinds;
}

std::vector<std::string> ClassRules::raceNotes(const Character &) const
{
    return {};
}

bool ClassRules::showsCreationSkillMatrix() const
{
    return false;
}

std::optional<float> ClassRules::unstuckLift(double) const
{
    return std::nullopt;
}

std::optional<std::string> ClassRules::partyCharge(Party &, IGameplayWorldRuntime &, std::optional<size_t>) const
{
    return std::nullopt;
}

std::optional<FollowerHireRefusal> ClassRules::followerHireRefusal(uint32_t) const
{
    return std::nullopt;
}

const ClassRules &standardClassRules()
{
    static const ClassRules Rules;
    return Rules;
}
}
