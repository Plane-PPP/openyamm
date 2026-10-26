#pragma once

#include "game/tables/MonsterTable.h"

#include <array>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace OpenYAMM::Game
{
struct MapStatsEntry;

// The MAW ruleset's map data (rulesets/maw/data/map_levels.yml): each map's monster level band, the horizontal
// starter maps and the Beyond Madness level of each map. Keyed by lowercase map file name.
class MawMapLevels
{
public:
    bool loadFromYaml(const std::string &text, std::string &errorMessage);
    // Low, mid and high B levels (some are halves); {1, 1, 1} for a map MAW does not list.
    std::array<double, 3> band(const std::string &mapFileName) const;
    // Whether MAW lists the map (it has a band): the maps the Journal's Progress tab counts.
    bool lists(const std::string &mapFileName) const;
    bool isHorizontalStarterMap(const std::string &mapFileName) const;
    bool isMadnessStartingMap(const std::string &mapFileName) const;
    // The fixed Beyond Madness level of a map, or 0 when it has none.
    int madnessLevel(const std::string &mapFileName) const;

private:
    std::unordered_map<std::string, std::array<double, 3>> m_bands;
    std::unordered_set<std::string> m_horizontalStarterMaps;
    std::unordered_set<std::string> m_madnessStartingMaps;
    std::unordered_map<std::string, int> m_madnessLevels;
};

// What a map's monster levels depend on besides the monster table.
struct MawMonsterLevelInputs
{
    std::string mapFileName;
    // The map's display name: maps named "The Arena" or "Arena" get no bolster.
    std::string mapName;
    // MapStats Monster1Pic..Monster3Pic.
    std::array<std::string, 3> encounterPictures;
    bool horizontalProgression = false;
    bool madness = false;
    bool austerity = false;
    // Levels banked on the other continents (getPartyLevel).
    double partyLevel = 0;
    int resetCount = 0;
    // A dimension map's power on the map: monsters sit at power x 10 + 20 over their base, without party bolster.
    std::optional<int> dimensionPower;
    // A bounty hunt's map: every monster sits at its table level plus these, whatever else applies (MON:591-593).
    std::optional<double> bountyLevel;
};

// The levels every monster of a map gains (bolsterLevel, zzMaw-Monsters.lua:417-445): the party's banked levels less
// 4 (^1.015 in Austerity), Beyond Madness's fixed map level, +20 per reset; a dimension map's power x 10 + 20. Corpse
// gold reads it too.
double mawMapBolster(const MawMapLevels &mapLevels, const MawMonsterLevelInputs &inputs);

// Every monster's level on a map under MAW (devdocs/maw/BALANCE_MODEL.md §7.1-7.4, zzMaw-Monsters.lua
// recalculateMonsterTable), by monster id. Levels are not capped at 255. The Arena (d42) and dimension maps have their
// own rules; bounty maps raise the levels of their monsters where they are spawned.
std::unordered_map<int, int> resolveMawMonsterLevels(
    const MonsterTable &monsterTable,
    const MawMapLevels &mapLevels,
    const MawMonsterLevelInputs &inputs);

// The monster rows MAW's level rules cover.
constexpr int MawMonsterRows = 651;

// The family's B variant: monsters come in A/B/C triples and every level-related number reads the B row.
int mawFamilyBId(int monsterId);
}
