#pragma once

#include <cstdint>
#include <string>

namespace OpenYAMM::Game
{
class GameplayScreenRuntime;
struct Character;
struct InventoryItem;
enum class BagTag : uint8_t;
struct GameplayHudTextureHandle;

class GameplayHudOverlaySupport
{
public:
    static void renderReagentQuantity(GameplayScreenRuntime &context, const InventoryItem &item,
        float x, float y, float width, float height, float scale);

    // Time left on a buff or effect for popups: "1h 5m", "3m 20s", "45s", or "Constant".
    static std::string formatRemainingDuration(float remainingSeconds);

    static bool tryPopulateItemInspectOverlayFromRenderedHudItems(
        GameplayScreenRuntime &context,
        int width,
        int height,
        bool requireOpaqueHitTest);

    static void updateCharacterInspectOverlay(
        GameplayScreenRuntime &context,
        int width,
        int height);

    static void updateBuffInspectOverlay(
        GameplayScreenRuntime &context,
        int width,
        int height,
        bool showGameplayHud);

    static void updateCharacterDetailOverlay(
        GameplayScreenRuntime &context,
        int width,
        int height);

    static void updateSpellInspectOverlay(
        GameplayScreenRuntime &context,
        int width,
        int height);

    // A glow around an item's icon in the colour the rules give its name (MAW's rarities, none for Common), drawn just
    // before the icon: its silhouette in that colour in two rings around it. The rarity_glow setting: always, while the
    // Show Item Rarity key is held, or never.
    static void renderItemRarityGlow(
        GameplayScreenRuntime &context,
        const InventoryItem &item,
        const GameplayHudTextureHandle &icon,
        float x,
        float y,
        float width,
        float height,
        float scale);

    // The inventory page shows the bag overview (InventoryBagOverview): a ruleset with several bags, desktop, a party
    // member (not the Adventurers' Inn roster) and no bag zoomed; a spell picking an inventory item shows the open bag
    // at full size.
    static bool bagOverviewActive(const GameplayScreenRuntime &context);
    // A bag tag's icon (the hammer of a crafting bag, the potion of an alchemy bag) in the square at x, y.
    static void renderBagTagIcon(GameplayScreenRuntime &context, BagTag tag, float x, float y, float size);
    // The tags of `bag` as small icons in the top corners of the box at x, y: the crafting bag's left, the alchemy
    // bag's right.
    static void renderBagTagMarks(
        GameplayScreenRuntime &context,
        const Character &character,
        uint8_t bag,
        float x,
        float y,
        float width,
        float size);

    static void renderGameplayMouseLookOverlay(
        GameplayScreenRuntime &context,
        int width,
        int height,
        bool active);
};
} // namespace OpenYAMM::Game
