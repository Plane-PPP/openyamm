#include "game/rules/roguelike/RoguelikeRules.h"

#include "engine/Localization.h"

#include "game/StringUtils.h"
#include "game/gameplay/GameMechanics.h"
#include "game/gameplay/GameplayRuntimeInterfaces.h"
#include "game/items/ItemGenerator.h"
#include "game/party/Party.h"
#include "game/rules/RandomStreams.h"
#include "game/rules/roguelike/RoguelikeTalents.h"
#include "game/tables/ClassSkillTable.h"
#include "game/tables/HouseTable.h"
#include "game/tables/ItemTable.h"
#include "game/tables/MapStats.h"
#include "game/tables/SpellTable.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
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
constexpr const char *RewardedVariable = "run.rewarded";
constexpr const char *RouteCountVariable = "run.route.count";
constexpr const char *ArrivalStopVariable = "run.arrival_stop";
constexpr const char *ArrivalSlainMonstersVariable = "run.arrival_slain";
constexpr const char *ArrivalSlainHealthVariable = "run.arrival_slain_health";
const std::string RouteVariablePrefix = "run.route.";

// The perks of the run's first version (OpenYAMM 2.4, 473788e4) in the order of its run.yml, by which their levels were
// kept. Never reorder or shorten: saves of that version are read by it.
constexpr std::array<const char *, 13> LegacyPerkIds = {
    "might", "endurance", "accuracy", "speed", "intellect", "personality", "health", "mana", "armor", "resistances",
    "melee_damage", "ranged_damage", "explosive_arrows"};

// The skills behind a talent's group.
constexpr std::array<const char *, 8> MeleeSkills = {
    "Sword", "Axe", "Mace", "Dagger", "Spear", "Staff", "Unarmed", "Armsmaster"};
constexpr std::array<const char *, 9> MagicSkills = {"FireMagic", "AirMagic", "WaterMagic", "EarthMagic",
    "SpiritMagic", "MindMagic", "BodyMagic", "LightMagic", "DarkMagic"};

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

// Adds `amount` of the talent's stat to the hero's permanent bonuses; false for a stat the rules do not know.
bool applyTalentStat(Character &character, const std::string &stat, int amount)
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
    state.rewarded = std::max(variableOr(variables, RewardedVariable, 0), 0);
    // A save from before arrivals were taken: its stop under way counts as it always did.
    state.arrivalStop = variableOr(variables, ArrivalStopVariable, state.entered);
    state.arrivalSlainMonsters = std::max(variableOr(variables, ArrivalSlainMonstersVariable, 0), 0);
    state.arrivalSlainHealth = std::max(variableOr(variables, ArrivalSlainHealthVariable, 0), 0);
    state.route.clear();
    const int routeLength = std::clamp(variableOr(variables, RouteCountVariable, 0), 0, 64);

    for (int stop = 0; stop < routeLength; ++stop)
    {
        const std::string name = RouteVariablePrefix + std::to_string(stop);
        state.route.push_back(variableOr(variables, name.c_str(), 0));
    }
}

void writeRunState(const RoguelikeRunState &state, std::unordered_map<std::string, int32_t> &variables)
{
    variables[DungeonRunEnteredVariable] = state.entered;
    variables[LevelUpsVariable] = state.levelUps;
    variables[GuardianVariable] = state.guardian;
    variables[GuardianActorVariable] = state.guardianActor;
    variables[DoorOfferedVariable] = state.doorOffered;
    variables[PromotedVariable] = state.promoted;
    variables[RewardedVariable] = state.rewarded;
    variables[ArrivalStopVariable] = state.arrivalStop;
    variables[ArrivalSlainMonstersVariable] = state.arrivalSlainMonsters;
    variables[ArrivalSlainHealthVariable] = state.arrivalSlainHealth;

    if (!state.route.empty())
    {
        variables[RouteCountVariable] = static_cast<int32_t>(state.route.size());

        for (size_t stop = 0; stop < state.route.size(); ++stop)
        {
            variables[RouteVariablePrefix + std::to_string(stop)] = state.route[stop];
        }
    }
}

MapKillProgress runStopProgress(const RoguelikeRunState &state, MapKillProgress progress)
{
    if (state.entered > 0 && state.arrivalStop == state.entered)
    {
        progress.countedHealth = std::max(0.0, progress.countedHealth - state.arrivalSlainHealth);
        progress.slainHealth = std::max(0.0, progress.slainHealth - state.arrivalSlainHealth);
        progress.countedMonsters = std::max(0, progress.countedMonsters - state.arrivalSlainMonsters);
        progress.slainMonsters = std::max(0, progress.slainMonsters - state.arrivalSlainMonsters);
    }

    return progress;
}

std::vector<int32_t> drawRunRoute(const DungeonRunDefinition &run, uint64_t rootSeed)
{
    std::vector<int32_t> route;

    for (size_t index : drawDungeonRunRoute(run, rootSeed))
    {
        route.push_back(dungeonRunMapKey(run.dungeons[index].mapFileName));
    }

    return route;
}

bool ensureRunRoute(const DungeonRunDefinition &run, uint64_t rootSeed, RoguelikeRunState &state)
{
    if (state.entered <= 0 || !run.rules.has_value())
    {
        return false;
    }

    std::vector<std::optional<size_t>> fixed(run.rules->route.size());

    if (state.route.empty())
    {
        // The whole route the run had: a run under way ends as it began.
        const std::vector<std::string> legacyMaps = legacyDungeonRunMaps(rootSeed);

        for (size_t stop = 0; stop < fixed.size() && stop < legacyMaps.size(); ++stop)
        {
            fixed[stop] = dungeonRunIndexOfKey(run, dungeonRunMapKey(legacyMaps[stop]));
        }
    }
    else
    {
        // A kept route whose dungeons the run still has stays as it is.
        bool complete = true;

        for (size_t stop = 0; stop < state.route.size(); ++stop)
        {
            const std::optional<size_t> index = dungeonRunIndexOfKey(run, state.route[stop]);
            complete = complete && index.has_value();

            if (stop < fixed.size())
            {
                fixed[stop] = index;
            }
        }

        if (complete)
        {
            return false;
        }
    }

    // A map a later version's pool no longer has: that stop is drawn anew, the others keep theirs.
    std::vector<int32_t> route;

    for (size_t index : drawDungeonRunRoute(run, rootSeed, fixed))
    {
        route.push_back(dungeonRunMapKey(run.dungeons[index].mapFileName));
    }

    if (route.empty() || route == state.route)
    {
        return false;
    }

    state.route = std::move(route);
    return true;
}

const DungeonRunStop *runRouteStop(const DungeonRunDefinition &run, const RoguelikeRunState &state, size_t stop)
{
    if (stop >= state.route.size())
    {
        return nullptr;
    }

    const std::optional<size_t> index = dungeonRunIndexOfKey(run, state.route[stop]);
    return index.has_value() ? &run.dungeons[*index] : nullptr;
}

float runGuardianHealthScale(const DungeonRunRules &rules, int strongestCount)
{
    return strongestCount == 1 && rules.uniqueGuardianHealthScale > 0.0f ? rules.uniqueGuardianHealthScale
                                                                          : rules.guardianHealthScale;
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

int runTalentRank(const Character &character, std::string_view id)
{
    const auto found = character.talentRanks.find(std::string(id));
    return found != character.talentRanks.end() ? static_cast<int>(found->second) : 0;
}

SkillMastery runSkillGroupMastery(const Character &character, const std::string &skillOrGroup)
{
    const auto best = [&character](const auto &skills)
    {
        SkillMastery mastery = SkillMastery::None;

        for (const char *pSkill : skills)
        {
            mastery = std::max(mastery, characterMastery(character, pSkill));
        }

        return mastery;
    };

    if (skillOrGroup == "melee")
    {
        return best(MeleeSkills);
    }

    if (skillOrGroup == "magic")
    {
        return best(MagicSkills);
    }

    return characterMastery(character, skillOrGroup);
}

bool runTalentAvailable(const DungeonRunTalent &talent, const Character &character)
{
    return talent.needsSkill.empty() || runSkillGroupMastery(character, talent.needsSkill) >= talent.needsMastery;
}

std::vector<RunCard> dealRunCards(
    const DungeonRunRules &rules,
    const Character &character,
    const ClassSkillTable *pClassSkillTable,
    uint64_t rootSeed,
    size_t memberIndex)
{
    // Steps of skills the hero knows, skills the hero could learn, and talents.
    std::vector<RunCard> known;
    std::vector<RunCard> fresh;
    std::vector<RunCard> talents;

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
        card.id = "skill:" + skill;
        card.skill = skill;
        card.mastery = static_cast<SkillMastery>(static_cast<int>(current) + 1);
        const std::string name = displaySkillName(skill);
        card.books = rules.school(skill) == nullptr
            ? 0
            : (current == SkillMastery::None ? rules.booksNewSchool : rules.booksRaisedSchool);
        card.label = current == SkillMastery::None
            ? "Learn " + name
            : name + ": " + masteryName(card.mastery);

        if (card.books > 0)
        {
            card.label += " (+" + std::to_string(card.books) + " spell books)";
        }

        (current == SkillMastery::None ? fresh : known).push_back(std::move(card));
    }

    for (const DungeonRunTalent &talent : rules.talents)
    {
        const int rank = runTalentRank(character, talent.id);

        if (rank >= talent.maxRank || !runTalentAvailable(talent, character))
        {
            continue;
        }

        RunCard card = {};
        card.kind = RunCard::Kind::Talent;
        card.id = "talent:" + talent.id;
        card.talent = talent.id;
        card.rank = rank + 1;
        card.maxRank = talent.maxRank;
        card.label = talent.name;

        if (talent.maxRank > 1)
        {
            card.label += " (" + std::to_string(card.rank) + "/" + std::to_string(talent.maxRank) + ")";
        }

        talents.push_back(std::move(card));
    }

    const uint64_t taken = static_cast<uint64_t>(std::max(eventVariable(character, RunCardsTakenVariable), 0));
    RandomStream stream(rootSeed ^ randomNameKey("dungeon_run.cards") ^ (uint64_t(memberIndex + 1) << 32) ^ taken);
    std::vector<RunCard> cards;
    const size_t offers = static_cast<size_t>(std::max(rules.cardOffers, 1));

    // One step of a known skill and one talent where there are any, then anything left.
    for (std::vector<RunCard> *pPool : {&known, &talents})
    {
        if (!pPool->empty() && cards.size() < offers)
        {
            cards.push_back(takeRandom(*pPool, stream));
        }
    }

    std::vector<RunCard> rest = std::move(known);
    rest.insert(rest.end(), std::make_move_iterator(fresh.begin()), std::make_move_iterator(fresh.end()));
    rest.insert(rest.end(), std::make_move_iterator(talents.begin()), std::make_move_iterator(talents.end()));

    while (cards.size() < offers && !rest.empty())
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
    else if (const DungeonRunTalent *pTalent = rules.talent(card.talent))
    {
        const int rank = runTalentRank(character, pTalent->id);

        // One rank a card, and its permanent number with it; a card of a rank the hero has adds nothing.
        if (rank < pTalent->maxRank)
        {
            if (!pTalent->stat.empty())
            {
                applyTalentStat(character, pTalent->stat, pTalent->amount);
            }

            character.talentRanks[pTalent->id] = static_cast<uint16_t>(rank + 1);
        }
    }

    character.eventVariables[RunCardsTakenVariable] = taken + 1;
    character.eventVariables[RunPendingCardsVariable] =
        std::max(eventVariable(character, RunPendingCardsVariable) - 1, 0);
    // Endurance, Bodybuilding and the like move the maximums; what the hero has left stays.
    GameMechanics::refreshCharacterBaseResources(character, false, pClassMultiplierTable);
    return bookSpells;
}

std::vector<uint32_t> grantRunItems(Party &party, const ItemTable &itemTable, const std::vector<uint32_t> &owed)
{
    std::vector<uint32_t> waiting;

    for (const uint32_t itemId : owed)
    {
        if (itemId == 0 || itemTable.get(itemId) == nullptr)
        {
            continue;
        }

        // The guardian's own piece, known for what it is.
        InventoryItem item = ItemGenerator::makeInventoryItem(itemId, itemTable, ItemGenerationMode::Generic);
        item.identified = true;

        if (!party.tryGrantInventoryItem(item))
        {
            waiting.push_back(itemId);
        }
    }

    return waiting;
}

std::optional<uint32_t> pickRunGuardianItem(
    const ItemTable &itemTable,
    ItemRarity rarity,
    const std::string &worldId,
    uint64_t rootSeed,
    int stop,
    const std::function<bool(uint32_t)> &taken)
{
    std::vector<uint32_t> pool;
    std::vector<uint32_t> fallback;

    for (const ItemDefinition &entry : itemTable.entries())
    {
        if (entry.itemId == 0 || entry.rarity != rarity || entry.value <= 0)
        {
            continue;
        }

        if (taken && taken(entry.itemId))
        {
            continue;
        }

        (entry.availableInWorld(worldId) ? pool : fallback).push_back(entry.itemId);
    }

    if (pool.empty())
    {
        pool = std::move(fallback);
    }

    if (pool.empty())
    {
        return std::nullopt;
    }

    std::sort(pool.begin(), pool.end());
    RandomStream stream(rootSeed ^ randomNameKey("dungeon_run.guardian_reward") ^ (uint64_t(std::max(stop, 0)) << 24));
    return pool[stream.uniform(0, static_cast<uint32_t>(pool.size() - 1))];
}

size_t migrateRunBottles(Character &character, const ItemTable &itemTable)
{
    // MM6's "Potion Bottle" row that run.yml's camp stock named in OpenYAMM 2.4 (patch #51).
    constexpr uint32_t LegacyRunBottleItemId = 1765;
    const ItemDefinition *pBottle = nullptr;

    for (const ItemDefinition &entry : itemTable.entries())
    {
        if (entry.isEmptyAlchemyBottle)
        {
            pBottle = &entry;
            break;
        }
    }

    if (pBottle == nullptr || pBottle->itemId == LegacyRunBottleItemId)
    {
        return 0;
    }

    size_t changed = 0;
    const auto convert = [pBottle, &changed](std::vector<InventoryItem> &items)
    {
        for (InventoryItem &item : items)
        {
            if (item.objectDescriptionId == LegacyRunBottleItemId)
            {
                item.objectDescriptionId = pBottle->itemId;
                item.width = std::max<uint8_t>(1, pBottle->inventoryWidth);
                item.height = std::max<uint8_t>(1, pBottle->inventoryHeight);
                ++changed;
            }
        }
    };

    convert(character.inventory);

    for (auto &[bag, items] : character.rulesetState.storedBags)
    {
        convert(items);
    }

    return changed;
}

bool migrateLegacyRunPerks(Character &character)
{
    bool moved = false;

    for (size_t index = 0; index < LegacyPerkIds.size(); ++index)
    {
        const auto found =
            character.eventVariables.find(static_cast<uint16_t>(RunLegacyPerkLevelVariableBase + index));

        if (found == character.eventVariables.end())
        {
            continue;
        }

        if (found->second > 0)
        {
            uint16_t &rank = character.talentRanks[LegacyPerkIds[index]];
            rank = static_cast<uint16_t>(std::max<int>(rank, std::min<int>(found->second, 255)));
        }

        character.eventVariables.erase(found);
        moved = true;
    }

    return moved;
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

        const auto has = [pMember](CharacterCondition condition)
        {
            return pMember->conditions.test(static_cast<size_t>(condition));
        };

        if (rules.levelUpHealShare <= 0.0 || has(CharacterCondition::Dead) || has(CharacterCondition::Petrified)
            || has(CharacterCondition::Eradicated))
        {
            continue;
        }

        // The level-up's breath: a share of the new maximum, and at least up to that share for a hero below zero.
        const int maximum = std::max(
            1, GameMechanics::calculateEffectiveCharacterMaxHealth(*pMember, party.classMultiplierTable()));
        const int share = std::max(1, static_cast<int>(std::ceil(maximum * rules.levelUpHealShare)));
        pMember->health = std::max(pMember->health, std::min(maximum, std::max(pMember->health + share, share)));
        pMember->conditions.reset(static_cast<size_t>(CharacterCondition::Unconscious));
        pMember->conditionStartGameMinutes[static_cast<size_t>(CharacterCondition::Unconscious)] = 0.0f;
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
    std::string rewardMapProgress(Party &party, const MapKillProgress &mapKill) const override
    {
        const DungeonRunRules &rules = m_rules.rules();
        RoguelikeRunState &state = m_rules.state();
        std::string message;

        if (state.entered <= 0)
        {
            return message;
        }

        // The stop's own clearing: the monsters dead before the party came are not the party's.
        const MapKillProgress progress = runStopProgress(state, mapKill);

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
        MapKillProgress mapKill;

        if (m_rules.state().entered <= 0 || !world.mapKillProgress(mapKill))
        {
            return std::nullopt;
        }

        const MapKillProgress progress = runStopProgress(m_rules.state(), mapKill);

        if (progress.countedHealth <= 0)
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

// The run's part of combat, spells and classes: its talents' effects (RoguelikeTalents.h); everything else is
// Standard's.
namespace
{
// Whether the hero carries a shield (Shield Wall).
bool shieldInHand(const Party &party, const Character &member)
{
    const ItemDefinition *pItem =
        party.itemTable() != nullptr && member.equipment.offHand != 0
            ? party.itemTable()->get(member.equipment.offHand)
            : nullptr;
    return pItem != nullptr && pItem->equipStat == "Shield";
}
}

class RoguelikeRules::Combat : public CombatRules
{
public:
    explicit Combat(const RoguelikeRules &rules)
        : m_rules(rules)
    {
    }

    // Sweeping Blows, Multishot and Piercing Arrows.
    CharacterAttackExtras characterAttackExtras(const Character &attacker) const override
    {
        return RunTalents::attackExtras(m_rules.rules(), attacker);
    }

    // Onslaught: the hero's weapon attacks recover faster while its haste lasts.
    void resolveCharacterAttack(CharacterAttackResult &attack, const CharacterAttackOnMonster &context) const override
    {
        CombatRules::resolveCharacterAttack(attack, context);

        if (context.pCharacter != nullptr)
        {
            attack.recoverySeconds *= RunTalents::attackRecoveryScale(m_rules.rules(), *context.pCharacter);
        }
    }

    bool usesPartyHitEffects() const override
    {
        return true;
    }

    // A hero's hit landed: Onslaught counts a kill, Bloodthirst heals, an arrow of Pinning Shot slows its target.
    void applyPartyHitEffects(Party &party, const PartyHitOnMonster &hit) const override
    {
        CombatRules::applyPartyHitEffects(party, hit);
        Character *pMember = party.member(hit.memberIndex);

        if (pMember == nullptr || pMember->talentRanks.empty())
        {
            return;
        }

        const DungeonRunRules &rules = m_rules.rules();

        if (hit.killed)
        {
            RunTalents::noteKill(rules, *pMember);
        }

        if (const int heal = RunTalents::leech(rules, *pMember, hit.damage); heal > 0)
        {
            party.healMember(hit.memberIndex, heal);
        }

        if (!hit.melee && hit.spellId == 0 && !hit.killed && hit.pWorld != nullptr && hit.targetActorIndex)
        {
            if (const float seconds = RunTalents::pinSeconds(rules, *pMember); seconds > 0.0f)
            {
                hit.pWorld->debuffActor(*hit.targetActorIndex, GameplayActorHitDebuff::Slow, seconds, PinnedSlow);
            }
        }
    }

    // Shield Wall, Unbroken and Cheat Death take their share of a hit before it reaches the hero's hit points.
    int absorbCharacterDamage(Party &party, size_t memberIndex, int damage) const override
    {
        damage = CombatRules::absorbCharacterDamage(party, memberIndex, damage);
        Character *pMember = party.member(memberIndex);

        if (pMember == nullptr || damage <= 0 || pMember->talentRanks.empty())
        {
            return damage;
        }

        const DungeonRunRules &rules = m_rules.rules();
        const int maximum = Party::effectiveMaximumHealth(*pMember);
        damage = RunTalents::absorbDamage(rules, *pMember, shieldInHand(party, *pMember), damage, maximum);
        const int left =
            RunTalents::cheatDeath(rules, *pMember, damage, pMember->health, maximum, m_rules.state().entered);

        if (left < damage)
        {
            party.requestSpeech(memberIndex, SpeechId::CheatedDeath);
        }

        return left;
    }

    // Shield Wall: a hero with a shield may step into a blow or an arrow meant for another; spells are not caught.
    std::optional<CoverOutcome> coverMonsterAttack(Party &party, const MonsterAttackCover &attack) const override
    {
        if (const std::optional<CoverOutcome> cover = CombatRules::coverMonsterAttack(party, attack))
        {
            return cover;
        }

        if (attack.spell || attack.pRng == nullptr)
        {
            return std::nullopt;
        }

        for (size_t memberIndex = 0; memberIndex < party.members().size(); ++memberIndex)
        {
            const Character *pMember = party.member(memberIndex);

            if (memberIndex == attack.targetIndex || pMember == nullptr || !GameMechanics::canAct(*pMember))
            {
                continue;
            }

            const float chance =
                RunTalents::coverChance(m_rules.rules(), *pMember, shieldInHand(party, *pMember));

            if (chance > 0.0f && std::uniform_real_distribution<float>(0.0f, 100.0f)(*attack.pRng) < chance)
            {
                return CoverOutcome{memberIndex, false};
            }
        }

        return std::nullopt;
    }

    // Attunement, Overload and a surging spell.
    int rollSpellDamage(const CharacterSpellCast &cast, bool *pCritical) const override
    {
        const int damage = CombatRules::rollSpellDamage(cast, pCritical);

        if (cast.pCaster == nullptr || damage <= 0 || cast.fromScroll)
        {
            return damage;
        }

        const uint32_t spellId = cast.pSpell != nullptr ? static_cast<uint32_t>(std::max(cast.pSpell->id, 0)) : 0;
        return static_cast<int>(std::lround(
            damage * RunTalents::spellDamageScale(m_rules.rules(), *cast.pCaster, spellId, cast.monstersNearTarget)));
    }

    // Resolve at its last rank.
    bool immuneToMonsterSpecialAttacks(const Character &member) const override
    {
        return CombatRules::immuneToMonsterSpecialAttacks(member)
            || RunTalents::immuneToSpecialAttacks(m_rules.rules(), member);
    }

    // Binding Circle: a monster standing within the circle struck the hero, and every hostile within it is held.
    void applyMonsterHitEffects(
        Party &party,
        IGameplayWorldRuntime *pWorld,
        size_t memberIndex,
        const ActorRulesetFacts &attacker,
        uint32_t attackerActorId,
        int appliedDamage,
        CombatDamageType type) const override
    {
        CombatRules::applyMonsterHitEffects(party, pWorld, memberIndex, attacker, attackerActorId, appliedDamage, type);
        Character *pMember = party.member(memberIndex);
        const DungeonRunRules &rules = m_rules.rules();
        const RunTalents::Learned circle = pMember != nullptr
            ? RunTalents::learned(rules, *pMember, RunTalents::BindingCircle)
            : RunTalents::Learned{};

        if (!circle || pWorld == nullptr || appliedDamage <= 0)
        {
            return;
        }

        const auto cooldown = pMember->talentState.cooldownSeconds.find(std::string(RunTalents::BindingCircle));

        if (cooldown != pMember->talentState.cooldownSeconds.end() && cooldown->second > 0.0f)
        {
            return;
        }

        const float radius = circle.pTalent->radius;
        std::vector<size_t> held;
        bool attackerNear = false;

        for (size_t actorIndex = 0; actorIndex < pWorld->mapActorCount(); ++actorIndex)
        {
            GameplayRuntimeActorState actor = {};

            if (!pWorld->actorRuntimeState(actorIndex, actor) || actor.isDead || actor.currentHp <= 0)
            {
                continue;
            }

            const float distance =
                std::hypot(actor.preciseX - pWorld->partyX(), actor.preciseY - pWorld->partyY()) - float(actor.radius);

            if (distance > radius || std::abs(actor.preciseZ - pWorld->partyFootZ()) > radius)
            {
                continue;
            }

            attackerNear = attackerNear || actor.actorId == attackerActorId;

            if (actor.hostileToParty)
            {
                held.push_back(actorIndex);
            }
        }

        if (!attackerNear || held.empty())
        {
            return;
        }

        const float seconds = RunTalents::springBindingCircle(rules, *pMember);

        for (size_t actorIndex : held)
        {
            pWorld->debuffActor(actorIndex, GameplayActorHitDebuff::Slow, seconds, HeldSlow);
        }
    }

    // The talents' timers.
    void updateCombat(Party &party, IGameplayWorldRuntime &world, float deltaSeconds) const override
    {
        CombatRules::updateCombat(party, world, deltaSeconds);
        RunTalents::update(party, deltaSeconds);
    }

private:
    // The actor slow's power: Pinning Shot halves the monster's speed; Binding Circle holds it almost still (a
    // tenth, as the circle of a MAW solo run does).
    static constexpr float PinnedSlow = 2.0f;
    static constexpr float HeldSlow = 10.0f;

    const RoguelikeRules &m_rules;
};

class RoguelikeRules::Spells : public SpellRules
{
public:
    explicit Spells(const RoguelikeRules &rules)
        : m_rules(rules)
    {
    }

    // Attunement's cheaper spells and Spell Surge's free one; a scroll costs nothing anyway.
    int manaCost(const CharacterSpellCast &cast, int tableCost) const override
    {
        const int cost = SpellRules::manaCost(cast, tableCost);
        return cast.pCaster != nullptr && !cast.fromScroll
            ? RunTalents::manaCost(m_rules.rules(), *cast.pCaster, cost)
            : cost;
    }

    // Spell Surge counts the hero's casts (a scroll, which spends no spell points, is not one).
    void afterCast(
        Party &party,
        size_t casterIndex,
        uint32_t spellId,
        const SpellTable &spellTable,
        bool indoor,
        bool fromSpellbook) const override
    {
        SpellRules::afterCast(party, casterIndex, spellId, spellTable, indoor, fromSpellbook);

        if (Character *pCaster = party.member(casterIndex))
        {
            RunTalents::noteCast(m_rules.rules(), *pCaster);
        }
    }

private:
    const RoguelikeRules &m_rules;
};

class RoguelikeRules::Classes : public ClassRules
{
public:
    explicit Classes(const RoguelikeRules &rules)
        : m_rules(rules)
    {
    }

    // Resolve's resistances, after the class's own bonuses.
    void applyClassBonuses(Character &member) const override
    {
        ClassRules::applyClassBonuses(member);
        RunTalents::applyPassives(m_rules.rules(), member);
    }

private:
    const RoguelikeRules &m_rules;
};

class RoguelikeRules::Items : public ItemRules
{
public:
    explicit Items(const RoguelikeRules &rules)
        : m_rules(rules)
    {
    }

    // A camp shop's own stacks (the alchemy reagents) take the last places of its standard shelf.
    void adjustShopStock(
        std::vector<InventoryItem> &stock,
        const HouseEntry &house,
        const Party &party,
        const ItemTable &itemTable,
        std::mt19937 &rng) const override
    {
        ItemRules::adjustShopStock(stock, house, party, itemTable, rng);
        const CampaignState::HouseStockState *pState = party.campaign().houseStockState(house.id);

        if (pState == nullptr || &stock != &pState->standardStock)
        {
            return;
        }

        for (const DungeonRunShop &shop : m_rules.rules().shops)
        {
            if (shop.houseId != house.id || shop.stock.empty())
            {
                continue;
            }

            size_t slot = stock.size();

            for (auto stack = shop.stock.rbegin(); stack != shop.stock.rend() && slot > 0; ++stack)
            {
                if (itemTable.get(stack->itemId) == nullptr)
                {
                    continue;
                }

                InventoryItem item =
                    ItemGenerator::makeInventoryItem(stack->itemId, itemTable, ItemGenerationMode::Shop);
                item.quantity = stack->count;
                stock[--slot] = std::move(item);
            }

            return;
        }
    }

private:
    const RoguelikeRules &m_rules;
};

RoguelikeRules::RoguelikeRules(DungeonRunDefinition run)
    : m_run(std::move(run))
    , m_pExperience(std::make_unique<Experience>(*this))
    , m_pMonsters(std::make_unique<Monsters>(*this))
    , m_pCombat(std::make_unique<Combat>(*this))
    , m_pSpells(std::make_unique<Spells>(*this))
    , m_pClasses(std::make_unique<Classes>(*this))
    , m_pItems(std::make_unique<Items>(*this))
{
    const GameRules &standard = standardGameRules();
    m_pGameRules = std::make_unique<GameRules>(GameRules{
        *m_pExperience,
        *m_pCombat,
        *m_pMonsters,
        *m_pItems,
        standard.skills,
        *m_pSpells,
        *m_pClasses,
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
