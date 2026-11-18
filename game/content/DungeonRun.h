#pragma once

#include "engine/AssetFileSystem.h"
#include "game/party/SkillData.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace OpenYAMM::Game
{
// A dungeon run: a ruleset package with a run.yml plays its dungeons one after another instead of a continent. The
// party starts in the first dungeon, every way out of a map leads into the next one, and the run ends when the party
// is defeated or no dungeon is left. The order is a shuffle of the list by the save's random root seed, so a run never
// repeats a dungeon and a loaded save keeps its order. A ruleset without a run.yml is not a run and plays as before.

// One dungeon and where the party arrives in it. The original BLV maps have no party start, so the entrance is the
// destination of the outdoor event leading there, with its yaw in evt.MoveToMap units (2048 to a turn).
struct DungeonRunStop
{
    std::string mapFileName;
    int32_t x = 0;
    int32_t y = 0;
    int32_t z = 0;
    int32_t yawUnits = 0;
    // The dungeon's difficulty tier (1 the easiest), which a rules route picks by; 0 without rules. With `stages` in
    // run.yml: the first of them.
    int tier = 0;
    // The stops of a rules route (their stages) the dungeon may stand at: run.yml's `stages`, or its `tier` alone.
    std::vector<int> stages;
    // The map's encounter group sizes times this (run rules: fewer monsters on the large maps).
    float groupScale = 1.0f;
};

// A talent a level-up card may give, one rank a card, up to maxRank. Its id is stable: it names the hero's rank in
// Character::talentRanks and in saves, whatever the order of run.yml. A talent with a `stat` adds `amount` of it to the
// hero's permanent bonuses with every rank (might ... luck, health, mana, armor, melee_attack, ranged_attack,
// melee_damage, ranged_damage, resistances) or, for "explosive_arrows", makes the hero's arrows burst as a bow of
// Carnage's do. Every other talent is an effect of the run's rules (game/rules/roguelike/RoguelikeTalents.h), which
// read the rank's numbers here.
struct DungeonRunTalent
{
    std::string id;
    // English name and effect line (keys of the translations); {value} and {value2} are the rank's numbers.
    std::string name;
    std::string text;
    // An item icon (icons_x2) for the card.
    std::string icon;
    std::string stat;
    int amount = 0;
    int maxRank = 1;
    // What the hero must have before the card is offered: a skill (canonical name), or a group of skills, any of
    // which will do ("melee": a melee weapon skill or Armsmaster; "magic": a school of magic), at this mastery or
    // above. Empty: nothing.
    std::string needsSkill;
    SkillMastery needsMastery = SkillMastery::Normal;
    // The rank's numbers, values[rank - 1] (the last repeats for a rank beyond the list).
    std::vector<float> values;
    std::vector<float> values2;
    // Percent of a normal blow or shot, seconds an effect lasts, seconds before it may come again, world units,
    // degrees either side of the view, and a count of monsters.
    float share = 0.0f;
    float seconds = 0.0f;
    float cooldown = 0.0f;
    float radius = 0.0f;
    float arc = 0.0f;
    int targets = 0;
    // The spells the talent reads (Overload's area spells).
    std::vector<uint32_t> spells;

    float value(int rank) const;
    float value2(int rank) const;
};

// A school of magic whose skill card also gives spell books: the school's spells are firstSpell ... firstSpell + 10
// (ranks 1-4 Normal, 5-7 Expert, 8-10 Master, 11 Grandmaster), combatSpell its weakest attack spell.
struct DungeonRunSchool
{
    std::string skill;
    uint32_t firstSpell = 0;
    uint32_t combatSpell = 0;
};

// Goods a camp shop always has on its shelf besides its own stock: `count` of an item in one stack.
struct DungeonRunShopStack
{
    uint32_t itemId = 0;
    uint32_t count = 1;
};

// A shop of the camp between two dungeons: a house of the game, whose stock, prices and trade are the engine's own.
struct DungeonRunShop
{
    // The stop (1-based) the shop follows.
    int afterStop = 0;
    // The camp menu's line.
    std::string name;
    uint32_t houseId = 0;
    // Stacks that take the last places of the shop's standard shelf (the camp's alchemy reagents).
    std::vector<DungeonRunShopStack> stock;
};

// What the guardian of a stop leaves the party: gold and one rare item of a rarity ("artifact", "relic"; empty: none).
struct DungeonRunGuardianReward
{
    int gold = 0;
    std::string item;
};

// The rules of a run (run.yml `rules`). Every number is a starting value for testing, kept in data to be tuned
// without a build. A run.yml without the block plays as the bare skeleton: every dungeon in a shuffled order.
struct DungeonRunRules
{
    // The tier of each stop of the run; its length is the run's length.
    std::vector<int> route;
    // The share of the map's monster hit points the party must slay before the guardian comes, and the guardian's
    // hit points against its kind's.
    double clearShare = 0.5;
    float guardianHealthScale = 4.0f;
    // The guardian's hit points times this instead when its kind stood alone on the map at the first fill (a boss of
    // its own); 0: guardianHealthScale.
    float uniqueGuardianHealthScale = 0.0f;
    // Level-ups each stop gives, spread evenly up to its guardian, and the levels each raises every hero by.
    std::vector<int> levelUps;
    int levelsPerLevelUp = 4;
    // Cards a level-up offers each hero, one of which is taken.
    int cardOffers = 4;
    // A level-up brings a hero who is out round and gives back this share of the maximum hit points (at least up to
    // it); the dead, the petrified and the eradicated stay as they are.
    double levelUpHealShare = 0.25;
    // Food a rest in the camp costs (the rest of the game elsewhere keeps its own price).
    int campRestFood = 2;
    int startFood = 0;
    int startGold = 0;
    // After the guardian of this stop (1-based) the party is promoted; 0: never.
    int promotionAfterStop = 0;
    // Books a school card gives when it teaches the school and when it raises it.
    int booksNewSchool = 3;
    int booksRaisedSchool = 2;
    // The skills cards may teach and raise (canonical names).
    std::vector<std::string> skills;
    std::vector<DungeonRunTalent> talents;
    std::vector<DungeonRunSchool> schools;
    std::vector<DungeonRunShop> shops;
    // The guardians' rewards, one per stop (none for a stop beyond the list).
    std::vector<DungeonRunGuardianReward> guardianRewards;
    // The cards' pictures by skill (canonical name): an item icon or a HUD texture.
    std::unordered_map<std::string, std::string> skillIcons;
    // Heroes (character_data.txt ids) whose cards know no class caps: any skill of the pool up to Grandmaster.
    std::vector<uint32_t> unrestrictedCharacters;

    int totalLevelUps() const;
    // Level-ups the stops before this one (0-based) gave in full.
    int levelUpsBefore(size_t stop) const;
    const DungeonRunSchool *school(const std::string &skill) const;
    const DungeonRunTalent *talent(std::string_view id) const;
};

struct DungeonRunDefinition
{
    std::vector<DungeonRunStop> dungeons;
    // The dungeon every run starts in (`first` in run.yml, an index into dungeons); only the others are shuffled.
    // Without it a run may start in any dungeon.
    std::optional<size_t> firstDungeon;
    std::optional<DungeonRunRules> rules;
};

// The named global variable, saved with the game, that counts the dungeons the run entered, the current one included.
inline constexpr const char *DungeonRunEnteredVariable = "dungeon_run.entered";

// "engine/rulesets/<id>/run.yml".
std::string dungeonRunDefinitionPath(const std::string &rulesetId);
std::optional<DungeonRunDefinition> parseDungeonRunDefinition(const std::string &yamlText, std::string &errorMessage);
// The ruleset's run. A ruleset without one (Standard, MAW) gives nullopt and no error.
std::optional<DungeonRunDefinition> loadDungeonRunDefinition(
    const Engine::AssetFileSystem &assetFileSystem,
    const std::string &rulesetId,
    std::string &errorMessage);
// The indices 0..dungeonCount-1, shuffled by the root seed.
std::vector<size_t> dungeonRunOrder(size_t dungeonCount, uint64_t rootSeed);
// The order of a run's dungeons: that shuffle, with the run's first dungeon moved to the front. With a rules route:
// one dungeon of each stop's tier, taken from that shuffle without repeats (any dungeon left when the tier has none).
std::vector<size_t> dungeonRunOrder(const DungeonRunDefinition &run, uint64_t rootSeed);
// The dungeon that follows `entered` others, or nullptr when the run has none left.
const DungeonRunStop *dungeonRunStop(const DungeonRunDefinition &run, uint64_t rootSeed, size_t entered);
// Whether the dungeon may stand at this stage of a rules route.
bool dungeonRunStopAtStage(const DungeonRunStop &stop, int stage);
// A dungeon's key in a save (the route a run keeps, run.route.<n>): its map file name's hash, lower case, 31 bits.
int32_t dungeonRunMapKey(std::string_view mapFileName);
// The dungeon of the run with this key, if the run still has it.
std::optional<size_t> dungeonRunIndexOfKey(const DungeonRunDefinition &run, int32_t key);
// The route of a run with rules: one dungeon per stop of rules.route, standing at the stop's stage, none twice, drawn
// from the seed's shuffle. A stop of `fixed` that holds a dungeon keeps it (and no other stop takes that one).
std::vector<size_t> drawDungeonRunRoute(
    const DungeonRunDefinition &run, uint64_t rootSeed, const std::vector<std::optional<size_t>> &fixed = {});
// The maps an OpenYAMM 2.3-2.4 run (rules v0 and v1: eight MM6 dungeons by tier, route 1 2 3 3, nothing kept in the
// save) went through with this seed, in order: a run saved then keeps its dungeons when loaded.
std::vector<std::string> legacyDungeonRunMaps(uint64_t rootSeed);
// The main menu's line after a run: the party's defeat, or the last dungeon left behind.
std::string dungeonRunEndText(bool defeated, size_t completedDungeons);
}
