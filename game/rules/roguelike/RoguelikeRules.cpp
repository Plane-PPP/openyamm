#include "game/rules/roguelike/RoguelikeRules.h"

#include "engine/Localization.h"

#include "game/StringUtils.h"
#include "game/gameplay/GameMechanics.h"
#include "game/gameplay/GameplayRuntimeInterfaces.h"
#include "game/party/Party.h"
#include "game/rules/RandomStreams.h"
#include "game/tables/ClassSkillTable.h"
#include "game/tables/MapStats.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace OpenYAMM::Game
{
namespace
{
constexpr const char *LevelUpsVariable = "run.level_ups";
constexpr const char *GuardianVariable = "run.guardian";
constexpr const char *GuardianActorVariable = "run.guardian_actor";
constexpr const char *DoorOfferedVariable = "run.door_offered";
constexpr const char *PromotedVariable = "run.promoted";

int variableOr(const std::unordered_map<std::string, int32_t> &variables, const char *pName, int fallback)
{
    const auto it = variables.find(pName);
    return it != variables.end() ? it->second : fallback;
}

int eventVariable(const Character &character, uint16_t key)
{
    const auto it = character.eventVariables.find(key);
    return it != character.eventVariables.end() ? it->second : 0;
}

const char *masteryName(SkillMastery mastery)
{
    switch (mastery)
    {
        case SkillMastery::Normal:
            return "Normal";
        case SkillMastery::Expert:
            return "Expert";
        case SkillMastery::Master:
            return "Master";
        case SkillMastery::Grandmaster:
            return "Grandmaster";
        default:
            return "";
    }
}

SkillMastery characterMastery(const Character &character, const std::string &skill)
{
    const CharacterSkill *pSkill = character.findSkillByCanonicalName(skill);
    return pSkill != nullptr ? pSkill->mastery : SkillMastery::None;
}

// The spell ranks of a school a mastery can learn.
uint32_t spellRanksForMastery(SkillMastery mastery)
{
    switch (mastery)
    {
        case SkillMastery::Normal:
            return 4;
        case SkillMastery::Expert:
            return 7;
        case SkillMastery::Master:
            return 10;
        case SkillMastery::Grandmaster:
            return 11;
        default:
            return 0;
    }
}

// Adds `amount` of the perk's stat to the hero's permanent bonuses; false for a stat the rules do not know.
bool applyPerkStat(Character &character, const std::string &stat, int amount)
{
    CharacterStatBonuses &bonuses = character.permanentBonuses;

    if (stat == "explosive_arrows")
    {
        character.playerBits.insert(RunExplosiveArrowsPlayerBit);
        return true;
    }

    if (stat == "resistances")
    {
        for (int *pResistance : {&bonuses.resistances.fire, &bonuses.resistances.air, &bonuses.resistances.water,
                 &bonuses.resistances.earth, &bonuses.resistances.mind, &bonuses.resistances.body})
        {
            *pResistance += amount;
        }

        return true;
    }

    const std::pair<const char *, int *> stats[] = {
        {"might", &bonuses.might},
        {"intellect", &bonuses.intellect},
        {"personality", &bonuses.personality},
        {"endurance", &bonuses.endurance},
        {"speed", &bonuses.speed},
        {"accuracy", &bonuses.accuracy},
        {"luck", &bonuses.luck},
        {"health", &bonuses.maxHealth},
        {"mana", &bonuses.maxSpellPoints},
        {"armor", &bonuses.armorClass},
        {"melee_attack", &bonuses.meleeAttack},
        {"ranged_attack", &bonuses.rangedAttack},
        {"melee_damage", &bonuses.meleeDamage},
        {"ranged_damage", &bonuses.rangedDamage},
    };

    for (const auto &[pName, pValue] : stats)
    {
        if (stat == pName)
        {
            *pValue += amount;
            return true;
        }
    }

    return false;
}

template <typename Value>
Value takeRandom(std::vector<Value> &values, RandomStream &stream)
{
    const size_t index = stream.uniform(0, static_cast<uint32_t>(values.size() - 1));
    Value value = std::move(values[index]);
    values.erase(values.begin() + static_cast<std::ptrdiff_t>(index));
    return value;
}
}

void readRunState(const std::unordered_map<std::string, int32_t> &variables, RoguelikeRunState &state)
{
    state.entered = std::max(variableOr(variables, DungeonRunEnteredVariable, 0), 0);
    state.levelUps = std::max(variableOr(variables, LevelUpsVariable, 0), 0);
    state.guardian = variableOr(variables, GuardianVariable, RoguelikeRunState::GuardianNone);
    state.guardianActor = variableOr(variables, GuardianActorVariable, -1);
    state.doorOffered = variableOr(variables, DoorOfferedVariable, 0);
    state.promoted = variableOr(variables, PromotedVariable, 0);
}

void writeRunState(const RoguelikeRunState &state, std::unordered_map<std::string, int32_t> &variables)
{
    variables[DungeonRunEnteredVariable] = state.entered;
    variables[LevelUpsVariable] = state.levelUps;
    variables[GuardianVariable] = state.guardian;
    variables[GuardianActorVariable] = state.guardianActor;
    variables[DoorOfferedVariable] = state.doorOffered;
    variables[PromotedVariable] = state.promoted;
}

uint32_t runSkillLevelForMastery(SkillMastery mastery)
{
    switch (mastery)
    {
        case SkillMastery::Normal:
            return 1;
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

std::vector<RunCard> dealRunCards(
    const DungeonRunRules &rules,
    const Character &character,
    const ClassSkillTable *pClassSkillTable,
    uint64_t rootSeed,
    size_t memberIndex)
{
    // Steps of skills the hero knows, skills the hero could learn, and perks.
    std::vector<RunCard> known;
    std::vector<RunCard> fresh;
    std::vector<RunCard> perks;

    const bool unrestricted = std::find(
        rules.unrestrictedCharacters.begin(), rules.unrestrictedCharacters.end(), character.characterDataId)
        != rules.unrestrictedCharacters.end();

    for (const std::string &skill : rules.skills)
    {
        const SkillMastery current = characterMastery(character, skill);
        const SkillMastery classCap = pClassSkillTable != nullptr
            ? pClassSkillTable->getEffectiveCap(character.className, character.raceId, skill)
            : SkillMastery::None;
        const SkillMastery cap = unrestricted ? SkillMastery::Grandmaster : classCap;

        if (current >= cap || current >= SkillMastery::Grandmaster)
        {
            continue;
        }

        RunCard card = {};
        card.kind = RunCard::Kind::Skill;
        card.skill = skill;
        card.mastery = static_cast<SkillMastery>(static_cast<int>(current) + 1);
        const std::string name = displaySkillName(skill);
        const int books = rules.school(skill) == nullptr
            ? 0
            : (current == SkillMastery::None ? rules.booksNewSchool : rules.booksRaisedSchool);
        card.label = current == SkillMastery::None
            ? "Learn " + name
            : name + ": " + masteryName(card.mastery);

        if (books > 0)
        {
            card.label += " (+" + std::to_string(books) + " spell books)";
        }

        (current == SkillMastery::None ? fresh : known).push_back(std::move(card));
    }

    for (size_t perkIndex = 0; perkIndex < rules.perks.size(); ++perkIndex)
    {
        const DungeonRunPerk &perk = rules.perks[perkIndex];
        const int level = eventVariable(character, static_cast<uint16_t>(RunPerkLevelVariableBase + perkIndex));

        if (level >= perk.maxLevel
            || (!perk.requiresSkill.empty() && characterMastery(character, perk.requiresSkill) == SkillMastery::None))
        {
            continue;
        }

        RunCard card = {};
        card.kind = RunCard::Kind::Perk;
        card.perkIndex = perkIndex;
        card.perkLevel = level + 1;
        card.label = perk.name;

        if (perk.maxLevel > 1)
        {
            card.label += " (" + std::to_string(card.perkLevel) + "/" + std::to_string(perk.maxLevel) + ")";
        }

        perks.push_back(std::move(card));
    }

    const uint64_t taken = static_cast<uint64_t>(std::max(eventVariable(character, RunCardsTakenVariable), 0));
    RandomStream stream(rootSeed ^ randomNameKey("dungeon_run.cards") ^ (uint64_t(memberIndex + 1) << 32) ^ taken);
    std::vector<RunCard> cards;

    // One step of a known skill and one perk where there are any, then anything left.
    for (std::vector<RunCard> *pPool : {&known, &perks})
    {
        if (!pPool->empty())
        {
            cards.push_back(takeRandom(*pPool, stream));
        }
    }

    std::vector<RunCard> rest = std::move(known);
    rest.insert(rest.end(), std::make_move_iterator(fresh.begin()), std::make_move_iterator(fresh.end()));
    rest.insert(rest.end(), std::make_move_iterator(perks.begin()), std::make_move_iterator(perks.end()));

    while (cards.size() < 3 && !rest.empty())
    {
        cards.push_back(takeRandom(rest, stream));
    }

    return cards;
}

std::vector<uint32_t> applyRunCard(
    const DungeonRunRules &rules,
    Character &character,
    const RunCard &card,
    const ClassMultiplierTable *pClassMultiplierTable,
    uint64_t rootSeed,
    size_t memberIndex,
    const std::function<bool(uint32_t)> &spellTaken)
{
    std::vector<uint32_t> bookSpells;
    const int taken = eventVariable(character, RunCardsTakenVariable);

    if (card.kind == RunCard::Kind::Skill)
    {
        const SkillMastery previous = characterMastery(character, card.skill);
        CharacterSkill *pSkill = character.findSkillByCanonicalName(card.skill);

        if (pSkill == nullptr)
        {
            pSkill = &(character.skills[card.skill] = CharacterSkill{card.skill, 0, SkillMastery::None});
        }

        pSkill->mastery = std::max(pSkill->mastery, card.mastery);
        pSkill->level = std::max(pSkill->level, runSkillLevelForMastery(card.mastery));

        if (const DungeonRunSchool *pSchool = rules.school(card.skill))
        {
            const size_t count = static_cast<size_t>(
                previous == SkillMastery::None ? rules.booksNewSchool : rules.booksRaisedSchool);
            const uint32_t ranks = spellRanksForMastery(pSkill->mastery);
            const uint32_t previousRanks = spellRanksForMastery(previous);
            // The spells the step opened come before the school's older ones.
            std::vector<uint32_t> opened;
            std::vector<uint32_t> older;

            for (uint32_t rank = 0; rank < ranks; ++rank)
            {
                const uint32_t spellId = pSchool->firstSpell + rank;

                if (character.knownSpellIds.contains(spellId) || (spellTaken && spellTaken(spellId)))
                {
                    continue;
                }

                if (spellId == pSchool->combatSpell)
                {
                    bookSpells.push_back(spellId);
                }
                else
                {
                    (rank >= previousRanks ? opened : older).push_back(spellId);
                }
            }

            RandomStream stream(
                rootSeed ^ randomNameKey("dungeon_run.books") ^ (uint64_t(memberIndex + 1) << 32) ^ uint64_t(taken));

            for (std::vector<uint32_t> *pPool : {&opened, &older})
            {
                while (bookSpells.size() < count && !pPool->empty())
                {
                    bookSpells.push_back(takeRandom(*pPool, stream));
                }
            }

            if (bookSpells.size() > count)
            {
                bookSpells.resize(count);
            }
        }
    }
    else if (card.perkIndex < rules.perks.size())
    {
        const DungeonRunPerk &perk = rules.perks[card.perkIndex];
        applyPerkStat(character, perk.stat, perk.amount);
        character.eventVariables[static_cast<uint16_t>(RunPerkLevelVariableBase + card.perkIndex)] = card.perkLevel;
    }

    character.eventVariables[RunCardsTakenVariable] = taken + 1;
    character.eventVariables[RunPendingCardsVariable] =
        std::max(eventVariable(character, RunPendingCardsVariable) - 1, 0);
    // Endurance, Bodybuilding and the like move the maximums; what the hero has left stays.
    GameMechanics::refreshCharacterBaseResources(character, false, pClassMultiplierTable);
    return bookSpells;
}

void grantRunLevelUp(const DungeonRunRules &rules, Party &party)
{
    for (size_t memberIndex = 0; memberIndex < party.members().size(); ++memberIndex)
    {
        Character *pMember = party.member(memberIndex);

        if (pMember == nullptr)
        {
            continue;
        }

        pMember->level += static_cast<uint32_t>(rules.levelsPerLevelUp);
        pMember->eventVariables[RunPendingCardsVariable] = eventVariable(*pMember, RunPendingCardsVariable) + 1;
        GameMechanics::refreshCharacterBaseResources(*pMember, false, party.classMultiplierTable());
    }
}

double runClearFraction(const DungeonRunRules &rules, const MapKillProgress &progress)
{
    const double target = progress.countedHealth * rules.clearShare;
    return target > 0 ? std::clamp(progress.slainHealth / target, 0.0, 1.0) : 0.0;
}

bool runClearThresholdPassed(const DungeonRunRules &rules, const MapKillProgress &progress)
{
    return progress.countedHealth > 0 && progress.slainHealth > progress.countedHealth * rules.clearShare;
}

int runLevelUpsDue(const DungeonRunRules &rules, size_t stop, const MapKillProgress &progress)
{
    const int stopLevelUps = stop < rules.levelUps.size() ? rules.levelUps[stop] : 0;
    const int before = rules.levelUpsBefore(stop);

    if (runClearThresholdPassed(rules, progress))
    {
        return before + stopLevelUps;
    }

    // The stop's last level-up comes with the guardian, not a hair before it.
    const int reached = static_cast<int>(std::floor(runClearFraction(rules, progress) * stopLevelUps + 1e-9));
    return before + std::clamp(reached, 0, std::max(stopLevelUps - 1, 0));
}

class RoguelikeRules::Experience : public ExperienceRules
{
public:
    explicit Experience(const RoguelikeRules &rules)
        : m_rules(rules)
    {
    }

    // After every kill: the level-ups the clearing has earned, the guardian's call and the guardian's death.
    std::string rewardMapProgress(Party &party, const MapKillProgress &progress) const override
    {
        const DungeonRunRules &rules = m_rules.rules();
        RoguelikeRunState &state = m_rules.state();
        std::string message;

        if (state.entered <= 0)
        {
            return message;
        }

        const int due = runLevelUpsDue(rules, static_cast<size_t>(state.entered - 1), progress);

        while (state.levelUps < due)
        {
            grantRunLevelUp(rules, party);
            ++state.levelUps;
            message = Engine::tr("Level up! Every hero gains {levels} levels and a card.",
                {{"levels", std::to_string(rules.levelsPerLevelUp)}});
        }

        if (state.guardian == RoguelikeRunState::GuardianNone && runClearThresholdPassed(rules, progress))
        {
            state.guardian = RoguelikeRunState::GuardianDue;
        }

        if (state.guardian == RoguelikeRunState::GuardianAlive
            && std::find(progress.slainLateActors.begin(), progress.slainLateActors.end(),
                   static_cast<size_t>(state.guardianActor)) != progress.slainLateActors.end())
        {
            state.guardian = RoguelikeRunState::GuardianSlain;
            message = Engine::tr("The guardian is slain. The way out is open.");
        }

        return message;
    }

    // A run has no training halls to pay; the level-ups come from clearing.
    void prepareNewParty(Party &party) const override
    {
        const DungeonRunRules &rules = m_rules.rules();
        party.addGold(rules.startGold - party.gold());
        party.addFood(rules.startFood - party.food());
    }

private:
    const RoguelikeRules &m_rules;
};

class RoguelikeRules::Monsters : public MonsterRules
{
public:
    explicit Monsters(const RoguelikeRules &rules)
        : m_rules(rules)
    {
    }

    // The run's own group sizes of the map.
    void adjustMapEncounters(MapStatsEntry &map) const override
    {
        for (const DungeonRunStop &stop : m_rules.run().dungeons)
        {
            if (toLowerCopy(stop.mapFileName) != toLowerCopy(map.fileName) || stop.groupScale == 1.0f)
            {
                continue;
            }

            for (MapEncounterInfo *pEncounter : {&map.encounter1, &map.encounter2, &map.encounter3})
            {
                if (pEncounter->maxCount <= 0)
                {
                    continue;
                }

                pEncounter->minCount =
                    std::max(1, static_cast<int>(std::lround(pEncounter->minCount * stop.groupScale)));
                pEncounter->maxCount = std::max(
                    pEncounter->minCount, static_cast<int>(std::lround(pEncounter->maxCount * stop.groupScale)));
            }
        }
    }

    // A dungeon of a run is entered once: nothing refills.
    int mapRespawnDays(const MapStatsEntry &) const override
    {
        return 0;
    }

    // The bar: the slain hit points against the guardian's threshold; full and gold once the guardian is slain.
    std::optional<RulesetMapProgress> mapProgress(const IGameplayWorldRuntime &world) const override
    {
        MapKillProgress progress;

        if (m_rules.state().entered <= 0 || !world.mapKillProgress(progress) || progress.countedHealth <= 0)
        {
            return std::nullopt;
        }

        const DungeonRunRules &rules = m_rules.rules();
        RulesetMapProgress bar = {};
        bar.percent = std::floor(runClearFraction(rules, progress) * 1000.0) / 10.0;
        bar.complete = m_rules.state().guardian == RoguelikeRunState::GuardianSlain;
        bar.slain = progress.slainMonsters;
        bar.counted = progress.countedMonsters;
        bar.needed = progress.countedMonsters;
        bar.requiredShare = rules.clearShare;
        bar.hidden = progress.hiddenMonsters;
        return bar;
    }

    RulesetScriptInfo scriptInfo(uint32_t continentId) const override
    {
        RulesetScriptInfo info = MonsterRules::scriptInfo(continentId);
        info.rulesetId = "roguelike";
        return info;
    }

private:
    const RoguelikeRules &m_rules;
};

RoguelikeRules::RoguelikeRules(DungeonRunDefinition run)
    : m_run(std::move(run))
    , m_pExperience(std::make_unique<Experience>(*this))
    , m_pMonsters(std::make_unique<Monsters>(*this))
{
    const GameRules &standard = standardGameRules();
    m_pGameRules = std::make_unique<GameRules>(GameRules{
        *m_pExperience,
        standard.combat,
        *m_pMonsters,
        standard.items,
        standard.skills,
        standard.spells,
        standard.classes,
        standard.potions,
        standard.crafting});
}

RoguelikeRules::~RoguelikeRules() = default;

const GameRules &RoguelikeRules::gameRules() const
{
    return *m_pGameRules;
}

const DungeonRunDefinition &RoguelikeRules::run() const
{
    return m_run;
}

const DungeonRunRules &RoguelikeRules::rules() const
{
    return *m_run.rules;
}

RoguelikeRunState &RoguelikeRules::state() const
{
    return m_state;
}
}
