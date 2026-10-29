#include "game/items/ReagentStacks.h"

#include <algorithm>

namespace OpenYAMM::Game::ReagentStacks
{
uint32_t takeCount(uint32_t quantity, bool shift, bool control)
{
    return shift ? std::min(1u, quantity) : control ? quantity / 2 + quantity % 2 : quantity;
}

bool eligible(const InventoryItem &item, const ItemTable *pTable)
{
    const ItemDefinition *pDefinition = pTable != nullptr ? pTable->get(item.objectDescriptionId) : nullptr;
    return pDefinition != nullptr && pDefinition->equipStat == "Reagent"
        && pDefinition->rarity == ItemRarity::Common && item.rarity == ItemRarity::Common
        && item.artifactId == 0 && pDefinition->materialName != "Quest"
        && !pDefinition->hasContentFlag("Quest") && !pDefinition->hasContentFlag("QuestItem")
        && !pDefinition->hasContentFlag("Unique");
}

bool compatible(const InventoryItem &left, const InventoryItem &right, const ItemTable *pTable)
{
    if (!eligible(left, pTable) || !eligible(right, pTable))
    {
        return false;
    }
    InventoryItem a = left;
    InventoryItem b = right;
    a.quantity = b.quantity = 1;
    a.gridX = b.gridX = a.gridY = b.gridY = 0;
    a.width = b.width = a.height = b.height = 1;
    return a == b;
}

bool consolidate(std::vector<InventoryItem> &items, const ItemTable *pTable)
{
    std::vector<InventoryItem> packed;
    for (InventoryItem item : items)
    {
        if (!eligible(item, pTable))
        {
            packed.push_back(item);
            continue;
        }
        item.width = item.height = 1;
        for (InventoryItem &target : packed)
        {
            if (target.quantity < Limit && compatible(target, item, pTable))
            {
                const uint32_t moved = std::min(item.quantity, Limit - target.quantity);
                target.quantity += moved;
                item.quantity -= moved;
            }
        }
        if ((uint64_t(item.quantity) + Limit - 1) / Limit + packed.size() > 65536)
        {
            return false;
        }
        while (item.quantity > 0)
        {
            InventoryItem part = item;
            part.quantity = std::min(item.quantity, Limit);
            packed.push_back(part);
            item.quantity -= part.quantity;
        }
    }
    items = std::move(packed);
    return true;
}
}
