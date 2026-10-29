#pragma once

#include "game/party/InventoryItem.h"

#include <vector>

namespace OpenYAMM::Game::ReagentStacks
{
constexpr uint32_t Limit = 50;
uint32_t takeCount(uint32_t quantity, bool shift, bool control);
bool eligible(const InventoryItem &item, const ItemTable *pTable);
bool compatible(const InventoryItem &left, const InventoryItem &right, const ItemTable *pTable);
// Atomic: return false without altering items if an unreasonable legacy quantity would expand the vector too far.
bool consolidate(std::vector<InventoryItem> &items, const ItemTable *pTable);
}
