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
};

struct DungeonRunDefinition
{
    std::vector<DungeonRunStop> dungeons;
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
// The order of a run's dungeons: the indices 0..dungeonCount-1, shuffled by the root seed.
std::vector<size_t> dungeonRunOrder(size_t dungeonCount, uint64_t rootSeed);
// The dungeon that follows `entered` others, or nullptr when the run has none left.
const DungeonRunStop *dungeonRunStop(const DungeonRunDefinition &run, uint64_t rootSeed, size_t entered);
// The main menu's line after a run: the party's defeat, or the last dungeon left behind.
std::string dungeonRunEndText(bool defeated, size_t completedDungeons);
}
