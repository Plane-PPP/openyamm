#pragma once

#include "game/events/RunCardDialog.h"

#include <cstddef>
#include <string>
#include <vector>

namespace OpenYAMM::Game
{
class Party;
struct Character;
struct DungeonRunRules;
struct DungeonRunTalent;
struct RunCard;

// What the card screen shows for the cards a hero of a dungeon run is offered (game/events/RunCardDialog.h): texts in
// the interface language and pictures. It reads the cards the rules dealt and changes nothing: the choice is given by
// the run's own code (GameApplicationDungeonRun.cpp), whichever way it was made.
RunCardDialog buildRunCardDialog(
    const DungeonRunRules &rules,
    const Party &party,
    size_t memberIndex,
    const std::vector<RunCard> &cards,
    int stop);
// A talent's effect line at a rank, its numbers written in: "{value}" and "{value2}" of the talent's text.
std::string runTalentEffectText(const DungeonRunTalent &talent, int rank);
}
