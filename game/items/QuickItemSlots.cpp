#include "game/items/QuickItemSlots.h"

#include "game/items/InventoryBags.h"
#include "game/items/InventoryItemUseRuntime.h"
#include "game/party/Party.h"

#include <algorithm>

namespace OpenYAMM::Game
{
namespace QuickItemSlots
{
bool canHold(
    const InventoryItem &item,
    const ItemTable &itemTable,
    const MergedPotionSettingTable *pPotionSettingTable,
    const MergedReagentSettingTable *pReagentSettingTable)
{
    const InventoryItemUseAction action =
        InventoryItemUseRuntime::classifyItemUse(item, itemTable, pPotionSettingTable, pReagentSettingTable);
    return action == InventoryItemUseAction::ConsumePotion || action == InventoryItemUseAction::CastScroll;
}

std::optional<Location> findItem(const Character &character, const QuickItemSlot &slot, int bagCount)
{
    if (slot.itemId == 0)
    {
        return std::nullopt;
    }

    std::optional<Location> found;
    uint16_t foundPower = 0;

    for (int bag = 0; bag < std::max(1, bagCount); ++bag)
    {
        for (const InventoryItem &item : InventoryBags::itemsInBag(character, uint8_t(bag)))
        {
            if (item.objectDescriptionId != slot.itemId)
            {
                continue;
            }

            const bool better = !found
                || (slot.bottleRule == QuickItemBottleRule::Weakest ? item.standardEnchantPower < foundPower
                                                                    : item.standardEnchantPower > foundPower);

            if (better)
            {
                found = Location{uint8_t(bag), item.gridX, item.gridY};
                foundPower = item.standardEnchantPower;
            }
        }
    }

    return found;
}

std::optional<PartyLocation> findPartyItem(
    const Party &party,
    size_t ownerIndex,
    const QuickItemSlot &slot,
    int bagCount)
{
    const std::vector<Character> &members = party.members();

    if (ownerIndex < members.size())
    {
        if (const std::optional<Location> own = findItem(members[ownerIndex], slot, bagCount))
        {
            return PartyLocation{ownerIndex, *own};
        }
    }

    for (size_t memberIndex = 0; memberIndex < members.size(); ++memberIndex)
    {
        if (memberIndex == ownerIndex)
        {
            continue;
        }

        if (const std::optional<Location> other = findItem(members[memberIndex], slot, bagCount))
        {
            return PartyLocation{memberIndex, *other};
        }
    }

    return std::nullopt;
}

size_t countPartyItems(const Party &party, const QuickItemSlot &slot, int bagCount)
{
    size_t count = 0;

    for (const Character &member : party.members())
    {
        count += countItems(member, slot, bagCount);
    }

    return count;
}

size_t countItems(const Character &character, const QuickItemSlot &slot, int bagCount)
{
    if (slot.itemId == 0)
    {
        return 0;
    }

    size_t count = 0;

    for (int bag = 0; bag < std::max(1, bagCount); ++bag)
    {
        const std::vector<InventoryItem> &items = InventoryBags::itemsInBag(character, uint8_t(bag));
        count += size_t(std::count_if(items.begin(), items.end(),
            [&slot](const InventoryItem &item) { return item.objectDescriptionId == slot.itemId; }));
    }

    return count;
}
}
}
