#pragma once

#include "game/app/KeyboardBindings.h"

#include <SDL3/SDL_scancode.h>

#include <optional>
#include <string>

namespace OpenYAMM::Game
{
struct GameplayInputFrame;
enum class BagTag : uint8_t;
class GameplayScreenRuntime;

class GameplayPartyOverlayInputController
{
public:
    static void handleUtilitySpellOverlayInput(
        GameplayScreenRuntime &context,
        const GameplayInputFrame &input);
    static void handleSpellbookOverlayInput(
        GameplayScreenRuntime &context,
        const GameplayInputFrame &input);
    static void handleCharacterOverlayInput(
        GameplayScreenRuntime &context,
        const GameplayInputFrame &input);
    // The skill the action switches on or off on the open skills page (MAW's Cover, P, and Mana Shield, M) when the
    // party member shown has it; while it does, its key's other uses (Adventurer's Inn, map) wait.
    static std::optional<std::string> skillSwitchForAction(GameplayScreenRuntime &context, KeyboardAction action);
    // Opens the Workbench (alchemy and crafting) on the open inventory, as its button does; false while an item is
    // held, which it says.
    static bool openWorkbench(GameplayScreenRuntime &context);
    // MAW's multibag keys (the Bag 1-4 actions, zzMaw-MultiBag.lua:38-67): on the inventory page the bag zooms (again,
    // back to the overview); in a shop's or chest's inventory the active member's bag opens. True when the key acted
    // there, so it does not also select a party member.
    static bool openBagForKey(GameplayScreenRuntime &context, int bag);
    // Whether a chest or a shop's or chest's party inventory shows the bag buttons (ChestInventoryBag1-4) now.
    static bool partyBagButtonsShown(const GameplayScreenRuntime &context);
    // Whether the bag keys act now: several bags, and a chest, a shop's or chest's inventory or the inventory page.
    static bool bagKeysActive(const GameplayScreenRuntime &context);
    // Whether the bag keys act only with Shift held, so that the numbers alone select a member: MAW's way and the
    // default of the bag_number_keys setting, in a party of more than one (zzMaw-MultiBag.lua:3-25).
    static bool bagKeysNeedShift(const GameplayScreenRuntime &context);
    // Tags or untags the open bag of the member the inventory page shows (the zoomed bag, or in the overview the bag
    // opened last) as a crafting or alchemy bag (zzzMaw_CraftingBag.lua:30-47), with a status line. False, with
    // nothing changed, off the inventory page or with a single bag.
    static bool toggleBagTag(GameplayScreenRuntime &context, BagTag tag);
};
} // namespace OpenYAMM::Game
