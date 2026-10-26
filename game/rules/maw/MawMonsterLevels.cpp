#include "game/rules/maw/MawMonsterLevels.h"

#include "game/rules/maw/MawExperienceCurve.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <utility>
#include <vector>

namespace OpenYAMM::Game
{
namespace
{
std::string lowerCopy(std::string value)
{
    for (char &character : value)
    {
        character = char(std::tolower(static_cast<unsigned char>(character)));
    }

    return value;
}

// MAW's round: halves go up.
double mawRound(double value)
{
    return std::floor(value + 0.5);
}

constexpr double HorizontalMultiplier = 2.5;
}

bool MawMapLevels::loadFromYaml(const std::string &text, std::string &errorMessage)
{
    try
    {
        const YAML::Node root = YAML::Load(text);

        for (const auto &entry : root["bands"])
        {
            const YAML::Node band = entry.second;

            if (!band.IsSequence() || band.size() != 3)
            {
                errorMessage = "band of " + entry.first.as<std::string>() + " must be [low, mid, high]";
                return false;
            }

            m_bands[lowerCopy(entry.first.as<std::string>())] = {
                band[0].as<double>(), band[1].as<double>(), band[2].as<double>()};
        }

        for (const YAML::Node &map : root["horizontal_starter_maps"])
        {
            m_horizontalStarterMaps.insert(lowerCopy(map.as<std::string>()));
        }

        for (const YAML::Node &map : root["madness"]["starting_maps"])
        {
            m_madnessStartingMaps.insert(lowerCopy(map.as<std::string>()));
        }

        for (const auto &entry : root["madness"]["levels"])
        {
            m_madnessLevels[lowerCopy(entry.first.as<std::string>())] = entry.second.as<int>();
        }
    }
    catch (const std::exception &exception)
    {
        errorMessage = std::string("map_levels.yml: ") + exception.what();
        return false;
    }

    if (m_bands.empty())
    {
        errorMessage = "map_levels.yml has no bands";
        return false;
    }

    return true;
}

std::array<double, 3> MawMapLevels::band(const std::string &mapFileName) const
{
    const auto band = m_bands.find(lowerCopy(mapFileName));
    return band != m_bands.end() ? band->second : std::array<double, 3>{1, 1, 1};
}

bool MawMapLevels::lists(const std::string &mapFileName) const
{
    return m_bands.contains(lowerCopy(mapFileName));
}

bool MawMapLevels::isHorizontalStarterMap(const std::string &mapFileName) const
{
    return m_horizontalStarterMaps.contains(lowerCopy(mapFileName));
}

bool MawMapLevels::isMadnessStartingMap(const std::string &mapFileName) const
{
    return m_madnessStartingMaps.contains(lowerCopy(mapFileName));
}

int MawMapLevels::madnessLevel(const std::string &mapFileName) const
{
    const auto level = m_madnessLevels.find(lowerCopy(mapFileName));
    return level != m_madnessLevels.end() ? level->second : 0;
}

int mawFamilyBId(int monsterId)
{
    return monsterId % 3 == 1 ? monsterId + 1 : monsterId % 3 == 0 ? monsterId - 1 : monsterId;
}

double mawMapBolster(const MawMapLevels &mapLevels, const MawMonsterLevelInputs &inputs)
{
    double bolster = std::max(inputs.partyLevel - 4, 0.0);

    if (inputs.austerity && inputs.partyLevel - 4 > 0)
    {
        bolster = std::pow(inputs.partyLevel - 4, 1.015);
    }

    const int madnessLevel = mapLevels.madnessLevel(inputs.mapFileName);

    if (inputs.madness && madnessLevel > 0 && !mapLevels.isMadnessStartingMap(inputs.mapFileName))
    {
        bolster = madnessLevel;
    }

    bolster += inputs.resetCount * 20.0;

    // A dimension map sets the bolster itself, and its monsters are not bolstered to the party.
    if (inputs.dimensionPower)
    {
        bolster = *inputs.dimensionPower * 10.0 + 20;
    }

    return bolster;
}

std::unordered_map<int, int> resolveMawMonsterLevels(
    const MonsterTable &monsterTable,
    const MawMapLevels &mapLevels,
    const MawMonsterLevelInputs &inputs)
{
    const auto baseLevel = [&monsterTable](int monsterId)
    {
        const MonsterTable::MonsterStatsEntry *pStats = monsterTable.findStatsById(int16_t(monsterId));
        return pStats != nullptr ? pStats->level : 0;
    };

    double bolster = mawMapBolster(mapLevels, inputs);
    const double resetBonus = inputs.resetCount * 20.0;
    const int madnessLevel = mapLevels.madnessLevel(inputs.mapFileName);
    const bool fixedMadnessLevel =
        inputs.madness && madnessLevel > 0 && !mapLevels.isMadnessStartingMap(inputs.mapFileName);
    const bool partyBolster = !fixedMadnessLevel && !inputs.dimensionPower;
    const bool arenaByName = inputs.mapName == "The Arena" || inputs.mapName == "Arena";

    if (arenaByName)
    {
        bolster = 0;
    }

    // Native families: B rows whose picture is one of the map's encounter pictures, in monster-id order (a picture
    // listed twice counts twice), the first three sorted by base level.
    std::vector<int> natives;

    for (int monsterId = 1; monsterId <= MawMonsterRows; ++monsterId)
    {
        const MonsterTable::MonsterStatsEntry *pStats = monsterTable.findStatsById(int16_t(monsterId));

        for (const std::string &picture : inputs.encounterPictures)
        {
            if (pStats != nullptr && !picture.empty() && picture + " B" == pStats->pictureName)
            {
                natives.push_back(monsterId);
            }
        }
    }

    if (natives.size() >= 2)
    {
        const auto order = [&](size_t first, size_t second)
        {
            if (baseLevel(natives[first]) > baseLevel(natives[second]))
            {
                std::swap(natives[first], natives[second]);
            }
        };
        order(0, 1);

        if (natives.size() == 3)
        {
            order(1, 2);
            order(0, 1);
        }
    }

    const std::array<double, 3> band = mapLevels.band(inputs.mapFileName);
    const double low = band[0];
    const double mid = band[1];
    const double high = band[2];
    const double mean = (low + mid + high) / 3;
    const double adjustMult = inputs.madness ? 1 : 1.5;
    const bool starterMap = mapLevels.isHorizontalStarterMap(inputs.mapFileName);
    std::unordered_map<int, int> levels;

    for (int monsterId = 1; monsterId <= MawMonsterRows; ++monsterId)
    {
        if (monsterTable.findStatsById(int16_t(monsterId)) == nullptr)
        {
            continue;
        }

        const double base = baseLevel(monsterId);
        const double familyLevel = baseLevel(mawFamilyBId(monsterId));
        double extraBolster = 0;

        if (familyLevel < low)
        {
            extraBolster = (low - familyLevel) / 2;
        }
        else if (familyLevel > high)
        {
            extraBolster = (high - familyLevel) / 2;
        }

        double adjust = 0;
        double nativeSlot = 0;

        for (size_t index = 0; index < natives.size(); ++index)
        {
            if (std::abs(monsterId - natives[index]) > 1)
            {
                continue;
            }

            const bool lowSlot = index == 0;
            const bool midSlot = index == 1 && natives.size() == 3;
            const bool highSlot = (index == 1 && natives.size() == 2) || index == 2;

            if (lowSlot || midSlot || highSlot)
            {
                nativeSlot = lowSlot ? low : midSlot ? mid : high;
                extraBolster = nativeSlot - familyLevel;
                adjust = (mean - nativeSlot) * adjustMult;
            }
        }

        if (arenaByName)
        {
            extraBolster = 0;
        }

        double level = partyBolster ? mawBolsteredLevel(bolster, base + extraBolster) : base + bolster + extraBolster;

        if ((inputs.horizontalProgression || inputs.madness) && !inputs.dimensionPower)
        {
            // MAW counts the reset bonus twice in this cap (BALANCE_MODEL.md §7.3).
            const double cap = partyBolster ? mawBolsteredLevel(bolster + resetBonus, base + extraBolster)
                                            : base + bolster + extraBolster + resetBonus;
            level = std::max(std::min((base + extraBolster) * HorizontalMultiplier, cap), 1.0);

            if (!starterMap)
            {
                // A map's monsters sit at about 2.5 times its band; the tier spread and the band's spread stay 1x.
                const double flattener = (base - familyLevel) * HorizontalMultiplier * 0.6;
                level = std::max(
                    base * HorizontalMultiplier + extraBolster * HorizontalMultiplier - 5 - flattener + adjust - 4
                        + resetBonus,
                    5.0);
            }
        }

        if (fixedMadnessLevel && !inputs.dimensionPower)
        {
            const double offset = (nativeSlot - mean) * 2 + (base - familyLevel) * 2;
            const double fixed = offset >= 0 ? mawBolsteredLevel(madnessLevel, offset)
                                             : 2.0 * madnessLevel - mawBolsteredLevel(madnessLevel, -offset);
            level = std::max(fixed, 5.0);
        }

        // The Arena (d42), where the Endless Arena brings every family: six times the base level, the tiers closer
        // together (zzMaw-Monsters.lua:577-589).
        if (inputs.mapFileName == "d42.blv")
        {
            level = std::max(base * 6 - (base - familyLevel) * 6 * 0.8 + adjust * 2, 5.0);
        }

        if (inputs.bountyLevel)
        {
            level = base + *inputs.bountyLevel;
        }

        levels[monsterId] = int(mawRound(std::max(level, 1.0)));
    }

    return levels;
}
}
