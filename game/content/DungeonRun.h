#pragma once

#include "engine/AssetFileSystem.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
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
    // The dungeon's difficulty tier (1 the easiest), which a rules route picks by; 0 without rules.
    int tier = 0;
    // The map's encounter group sizes times this (run rules: fewer monsters on the large maps).
    float groupScale = 1.0f;
};

// A numeric perk a level-up card may give: `amount` more of `stat` per level, up to maxLevel levels. The stats are
// the character's permanent bonuses (might ... luck, health, mana, armor, melee_attack, ranged_attack, melee_damage,
// ranged_damage, resistances) and "explosive_arrows", which makes the hero's arrows burst as a bow of Carnage's do.
struct DungeonRunPerk
{
    std::string id;
    std::string name;
    std::string stat;
    int amount = 0;
    int maxLevel = 1;
    // A skill the hero must have (canonical name), empty for none.
    std::string requiresSkill;
};

// A school of magic whose skill card also gives spell books: the school's spells are firstSpell ... firstSpell + 10
// (ranks 1-4 Normal, 5-7 Expert, 8-10 Master, 11 Grandmaster), combatSpell its weakest attack spell.
struct DungeonRunSchool
{
    std::string skill;
    uint32_t firstSpell = 0;
    uint32_t combatSpell = 0;
};

// A shop of the camp between two dungeons: a house of the game, whose stock, prices and trade are the engine's own.
struct DungeonRunShop
{
    // The stop (1-based) the shop follows.
    int afterStop = 0;
    // The camp menu's line.
    std::string name;
    uint32_t houseId = 0;
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
    // Level-ups each stop gives, spread evenly up to its guardian, and the levels each raises every hero by.
    std::vector<int> levelUps;
    int levelsPerLevelUp = 4;
    int startFood = 0;
    int startGold = 0;
    // After the guardian of this stop (1-based) the party is promoted; 0: never.
    int promotionAfterStop = 0;
    // Books a school card gives when it teaches the school and when it raises it.
    int booksNewSchool = 3;
    int booksRaisedSchool = 2;
    // The skills cards may teach and raise (canonical names).
    std::vector<std::string> skills;
    std::vector<DungeonRunPerk> perks;
    std::vector<DungeonRunSchool> schools;
    std::vector<DungeonRunShop> shops;

    int totalLevelUps() const;
    // Level-ups the stops before this one (0-based) gave in full.
    int levelUpsBefore(size_t stop) const;
    const DungeonRunSchool *school(const std::string &skill) const;
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
// The main menu's line after a run: the party's defeat, or the last dungeon left behind.
std::string dungeonRunEndText(bool defeated, size_t completedDungeons);
}
