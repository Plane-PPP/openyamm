#include "game/rules/maw/MawGameState.h"

#include "game/rules/maw/MawExperienceCurve.h"

namespace OpenYAMM::Game
{
int MawDimensionRun::affixCount() const
{
    int count = 0;

    for (uint8_t affix : affixes)
    {
        count += affix != 0 ? 1 : 0;
    }

    return count;
}

size_t MawGameState::continentIndex(uint32_t continentId)
{
    return continentId >= 1 && continentId <= 3 ? continentId - 1 : 3;
}

void MawGameState::addBankedExperience(double experience, uint32_t continentId)
{
    bankedExperience += experience;
    const double level = mawLevelForExperience(bankedExperience);
    continentLevels[continentIndex(continentId)] += level - bankedLevel;
    bankedLevel = level;
}

double MawGameState::totalLevel() const
{
    double total = 0;

    for (double level : continentLevels)
    {
        total += level;
    }

    return total;
}

double MawGameState::levelOutside(uint32_t continentId) const
{
    return totalLevel() - continentLevels[continentIndex(continentId)];
}

int MawGameState::resetCount(const std::string &mapFileName) const
{
    const auto it = mapResetCounts.find(mapFileName);
    return it != mapResetCounts.end() ? it->second : 0;
}
}
