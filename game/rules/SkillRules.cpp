#include "game/rules/SkillRules.h"

#include "game/gameplay/GameMechanics.h"
#include "game/party/Party.h"

#include <algorithm>

namespace OpenYAMM::Game
{
uint32_t SkillRules::skillRaiseCost(const Character &character, const std::string &canonicalSkill) const
{
    const CharacterSkill *pSkill = character.findSkill(canonicalSkill);
    return pSkill != nullptr ? pSkill->level + 1 : 1;
}

std::string SkillRules::skillDisplayName(const Character &, const std::string &canonicalSkill) const
{
    return displaySkillName(canonicalSkill);
}

std::vector<std::string> SkillRules::extraHouseSkills(const std::string &) const
{
    return {};
}

std::optional<uint32_t> SkillRules::skillLevelCap(const std::string &) const
{
    return std::nullopt;
}

bool SkillRules::canRaiseSkill(const Character &character, const std::string &canonicalSkill) const
{
    const CharacterSkill *pSkill = character.findSkill(canonicalSkill);

    if (pSkill == nullptr)
    {
        return false;
    }

    const std::optional<uint32_t> cap = skillLevelCap(canonicalSkill);
    return (!cap || pSkill->level < *cap)
        && character.pointsForRaisingSkill(canonicalSkill) >= skillRaiseCost(character, canonicalSkill);
}

std::string SkillRules::afterSkillRaised(Character &, const std::string &, const ClassSkillTable *) const
{
    return {};
}

uint32_t SkillRules::masteryRequiredLevel(const std::string &, SkillMastery mastery) const
{
    switch (mastery)
    {
        case SkillMastery::Expert:
            return 4;
        case SkillMastery::Master:
            return 7;
        case SkillMastery::Grandmaster:
            return 10;
        default:
            return 0;
    }
}

int SkillRules::masteryTeacherCost(const Character &, const std::string &, SkillMastery, int standardCost) const
{
    return standardCost;
}

std::optional<int> SkillRules::skillResetCost(const Character &) const
{
    return std::nullopt;
}

bool SkillRules::resetSkills(Character &) const
{
    return false;
}

void SkillRules::applyStatBonuses(Character &) const
{
}

void SkillRules::applyResourceBonuses(Character &) const
{
}

std::optional<CharacterCombatRows> SkillRules::combatRows(
    const Character &,
    const ItemTable *,
    const CharacterStatValues &) const
{
    return std::nullopt;
}

float SkillRules::regenerationHealthPerSecond(const Character &member) const
{
    constexpr float FiveGameMinuteTickRealSeconds = 10.0f;
    const CharacterSkill *pSkill = member.findSkill("Regeneration");
    return pSkill != nullptr ? float(pSkill->level * uint32_t(pSkill->mastery)) / FiveGameMinuteTickRealSeconds : 0.0f;
}

int SkillRules::meditationSpellPointsPerTick(const Character &member) const
{
    const CharacterSkill *pMeditation = member.findSkill("Meditation");

    if (pMeditation == nullptr || pMeditation->mastery == SkillMastery::None)
    {
        return 0;
    }

    const int skillLevel = std::max(0, int(pMeditation->level) + member.skillBonus("Meditation"));
    return int(pMeditation->mastery) + skillLevel / 10;
}

void SkillRules::updateRegeneration(Party &, const IGameplayWorldRuntime &, float) const
{
}

SkillTooltipText SkillRules::skillTooltip(
    const Party &,
    const Character &,
    const std::string &,
    const SkillTooltipText &table) const
{
    return table;
}

std::string SkillRules::statTooltip(const Party &, const Character &, const std::string &, const std::string &table) const
{
    return table;
}

std::string SkillRules::teacherSpeech(
    const std::string &text,
    const std::string &,
    SkillMastery,
    uint32_t,
    int) const
{
    return text;
}

float SkillRules::trainingPriceMultiplier(const Character &, float houseMultiplier) const
{
    return houseMultiplier;
}

std::optional<SkillRaiseCost> SkillRules::skillRaiseCostDetail(const Character &, const std::string &) const
{
    return std::nullopt;
}

IdentifyMonsterReveal SkillRules::identifyMonsterReveal(const Party &party, int monsterLevel) const
{
    const Character *pIdentifier = party.bestPartyWideUtilitySkillMember("IdentifyMonster");
    return pIdentifier != nullptr ? GameMechanics::resolveIdentifyMonsterReveal(*pIdentifier, monsterLevel)
                                  : IdentifyMonsterReveal {};
}

const SkillRules &standardSkillRules()
{
    static const SkillRules Rules;
    return Rules;
}

bool SkillRules::followerSkillBonusesStack() const
{
    return true;
}

std::optional<CharacterStatRowText> SkillRules::statRowText(const Party &, const Character &, const std::string &) const
{
    return std::nullopt;
}

std::array<std::optional<CharacterStatRowText>, 4> SkillRules::statRows(
    const Party &party,
    const Character &character) const
{
    return {statRowText(party, character, "Condition"), statRowText(party, character, "QuickSpell"),
        statRowText(party, character, "SpellPower"), statRowText(party, character, "Vitality")};
}

bool SkillRules::compactStatPowerRows() const
{
    return false;
}

std::optional<CharacterResistanceCut> SkillRules::resistanceCut(const Party &, const Character &,
    const std::string &) const
{
    return std::nullopt;
}

std::array<std::optional<CharacterResistanceCut>, 6> SkillRules::resistanceCuts(
    const Party &party,
    const Character &character) const
{
    std::array<std::optional<CharacterResistanceCut>, 6> cuts;
    const std::array<const char *, 6> names = {
        "FireResistance", "AirResistance", "WaterResistance", "EarthResistance", "MindResistance", "BodyResistance"};

    for (size_t index = 0; index < names.size(); ++index)
    {
        cuts[index] = resistanceCut(party, character, names[index]);
    }

    return cuts;
}

std::string SkillRules::statValueText(const std::string &text) const
{
    return text;
}
}
