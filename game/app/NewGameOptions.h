#pragma once

#include "game/content/RulesetOptions.h"

#include <cstdint>
#include <optional>

namespace OpenYAMM::Game
{
// What the new-game flow hands to the new session besides the party.
struct NewGameOptions
{
    uint32_t continentId = 0;
    // The debug God Lich party keeps its debug loadout.
    bool preserveDebugLoadout = false;
    RulesetSelection ruleset;
    // A chosen random root seed instead of a fresh one (the debug console's `run new <seed>`).
    std::optional<uint64_t> randomRootSeed;
};
}
