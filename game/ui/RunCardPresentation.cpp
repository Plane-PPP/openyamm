#include "game/ui/RunCardPresentation.h"

#include "engine/Localization.h"

#include "game/content/DungeonRun.h"
#include "game/party/Party.h"
#include "game/party/SkillData.h"
#include "game/rules/roguelike/RoguelikeRules.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace OpenYAMM::Game
{
namespace
{
// 2 as "2", 2.5 as "2.5".
std::string numberText(float value)
{
    const float rounded = std::round(value * 10.0f) / 10.0f;
    std::ostringstream stream;

    if (rounded == std::floor(rounded))
    {
        stream << static_cast<long long>(rounded);
    }
    else
    {
        stream << rounded;
    }

    return stream.str();
}

std::string masteryText(SkillMastery mastery)
{
    return mastery == SkillMastery::None
        ? Engine::tr("Not learned")
        : Engine::localize(masteryDisplayName(mastery), "mastery");
}

int eventVariableOr(const Character &character, uint16_t key, int fallback)
{
    const auto found = character.eventVariables.find(key);
    return found != character.eventVariables.end() ? found->second : fallback;
}

RunCardDisplay skillCard(const DungeonRunRules &rules, const Character &hero, const RunCard &card)
{
    RunCardDisplay display = {};
    const DungeonRunSchool *pSchool = rules.school(card.skill);
    const SkillMastery previous = static_cast<SkillMastery>(std::max(static_cast<int>(card.mastery) - 1, 0));
    const std::string skillName = Engine::localize(displaySkillName(card.skill), "skill");
    const int64_t level = static_cast<int64_t>(runSkillLevelForMastery(card.mastery));
    // The captions are small capitals in the mockup: written so in each language.
    display.kind = pSchool != nullptr ? Engine::trc("run card", "BOOKS") : Engine::trc("run card", "SKILL");
    display.title = skillName;
    display.stepFrom = masteryText(previous);
    display.stepTo = masteryText(card.mastery);
    display.pips = static_cast<int>(SkillMastery::Grandmaster);
    display.pipsBefore = static_cast<int>(previous);
    display.pipsAfter = static_cast<int>(card.mastery);
    display.effect.push_back(Engine::tr("Skill level {level}", {{"level", level}}));

    if (card.books > 0)
    {
        display.effect.push_back(Engine::tr("Up to {count} spell books", {{"count", int64_t(card.books)}}));
    }

    display.detail = previous == SkillMastery::None
        ? Engine::tr("{hero} learns {skill}: skill level {level}.",
            {{"hero", hero.name}, {"skill", skillName}, {"level", level}})
        : Engine::tr("{skill} rises to {mastery}: skill level {level}.",
            {{"skill", skillName}, {"mastery", masteryText(card.mastery)}, {"level", level}});

    if (card.books > 0)
    {
        display.detail += " " + Engine::tr("Up to {count} books of spells the hero does not have come with it.",
            {{"count", int64_t(card.books)}});
    }

    const auto icon = rules.skillIcons.find(card.skill);
    display.icon = icon != rules.skillIcons.end() ? icon->second : std::string();
    // A school without its sign shows its weakest attack spell.
    display.iconSpell = pSchool != nullptr ? pSchool->combatSpell : 0;
    return display;
}

RunCardDisplay talentCard(const DungeonRunRules &rules, const RunCard &card)
{
    RunCardDisplay display = {};
    const DungeonRunTalent *pTalent = rules.talent(card.talent);
    display.kind = Engine::trc("run card", "TALENT");
    display.title = pTalent != nullptr ? Engine::tr(pTalent->name) : card.talent;
    display.stepFrom = Engine::tr("Rank {rank}", {{"rank", int64_t(std::max(card.rank - 1, 0))}});
    display.stepTo = std::to_string(card.rank) + " / " + std::to_string(std::max(card.maxRank, 1));
    display.pips = std::max(card.maxRank, 1);
    display.pipsBefore = std::max(card.rank - 1, 0);
    display.pipsAfter = card.rank;

    if (pTalent != nullptr)
    {
        const std::string effect = runTalentEffectText(*pTalent, card.rank);
        display.effect.push_back(effect);
        display.detail = effect.empty() ? display.title : effect + ".";
        display.icon = pTalent->icon;
    }

    return display;
}
}

std::string runTalentEffectText(const DungeonRunTalent &talent, int rank)
{
    if (talent.text.empty())
    {
        return Engine::tr(talent.name);
    }

    return Engine::tr(talent.text,
        {{"value", numberText(talent.value(rank))}, {"value2", numberText(talent.value2(rank))}});
}

RunCardDialog buildRunCardDialog(
    const DungeonRunRules &rules,
    const Party &party,
    size_t memberIndex,
    const std::vector<RunCard> &cards,
    int stop)
{
    RunCardDialog dialog = {};
    dialog.memberIndex = memberIndex;
    dialog.totalCards = std::max(rules.totalLevelUps(), 1);
    dialog.stop = std::max(stop, 1);
    dialog.stops = std::max<int>(static_cast<int>(rules.route.size()), 1);
    const Character *pHero = party.member(memberIndex);

    if (pHero == nullptr)
    {
        return dialog;
    }

    dialog.cardNumber = std::clamp(eventVariableOr(*pHero, RunCardsTakenVariable, 0) + 1, 1, dialog.totalCards);

    for (const Character &member : party.members())
    {
        dialog.membersOwed.push_back(eventVariableOr(member, RunPendingCardsVariable, 0) > 0);
    }

    for (const RunCard &card : cards)
    {
        dialog.cards.push_back(
            card.kind == RunCard::Kind::Skill ? skillCard(rules, *pHero, card) : talentCard(rules, card));
    }

    return dialog;
}
}
