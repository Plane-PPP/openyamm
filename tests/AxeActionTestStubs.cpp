#include "game/gameplay/GameplayFxService.h"
#include "game/gameplay/GameplayItemService.h"
#include "game/gameplay/GameplayScreenRuntime.h"
#include "game/gameplay/GameplaySpellActionController.h"
#include "game/gameplay/TurnBasedCombatRuntime.h"

#include <stdexcept>

namespace OpenYAMM::Game
{
// Action tests exercise the real world/party pipeline with a null presentation runtime.
// Fail explicitly if a test accidentally enters an untested rendering or casting path.
GameAudioSystem *GameplayScreenRuntime::audioSystem() const
{
    throw std::logic_error("Unexpected audio runtime in headless action test");
}

GameplayFxService &GameplayScreenRuntime::fxService() const
{
    throw std::logic_error("Unexpected FX runtime in headless action test");
}

TurnBasedCombatRuntime &GameplayScreenRuntime::turnBasedCombatRuntime() const
{
    throw std::logic_error("Unexpected presentation runtime in headless action test");
}

// OpenYAMM 2.7: an ordinary attack asks whether the game is a solo run with talents (cooldown credit of a bow shot).
// The action tests play a party: none.
bool GameplayScreenRuntime::soloTalents() const
{
    return false;
}

// OpenYAMM 2.7's potion upkeep orders: not in these action tests.
const MergedPotionSettingTable *GameplayScreenRuntime::mergedPotionSettingTable() const
{
    return nullptr;
}

GameplayItemService &GameplayScreenRuntime::itemService() const
{
    throw std::logic_error("Unexpected item service in headless action test");
}

std::string GameplayItemService::restockUpkeepPotions(GameplayScreenRuntime &, bool)
{
    throw std::logic_error("Unexpected potion upkeep in headless action test");
}

bool GameplayItemService::useCarriedItem(GameplayScreenRuntime &, size_t, const QuickItemSlots::PartyLocation &)
{
    throw std::logic_error("Unexpected item use in headless action test");
}

void GameplayFxService::queueSurfaceEffect(
    const std::string &, const std::array<float, 3> &, const std::array<float, 3> &, float)
{
    throw std::logic_error("Unexpected FX in headless action test");
}

void GameplayFxService::queueWorldEffect(
    const std::string &, const std::array<float, 3> &, const std::array<float, 4> &, float)
{
    throw std::logic_error("Unexpected FX in headless action test");
}

GameplaySpellActionController::AttackCastResult GameplaySpellActionController::tryBeginAttackCast(
    GameplayScreenRuntime &, GameplaySpellService &, size_t, const std::string &, const TargetQueries &)
{
    throw std::logic_error("Unexpected spell casting in melee action test");
}
GameplaySpellService::MemberSpellCastResolution GameplaySpellService::castSpellFromMember(
    GameplayScreenRuntime &, size_t, uint32_t, const std::string &, bool, const TargetQueries *)
{
    throw std::logic_error("Unexpected upkeep spell casting in melee action test");
}

}
