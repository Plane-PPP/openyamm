#pragma once

#include <algorithm>
#include <cmath>
#include <utility>

namespace OpenYAMM::Game
{
struct InventoryIconRect
{
    float x = 0, y = 0, width = 0, height = 0;
};

// Center the original native extent over its definition-owned footprint. Quantity is irrelevant.
inline InventoryIconRect placeInventoryIcon(InventoryIconRect bounds, float width, float height)
{
    InventoryIconRect rect{std::round(bounds.x + (bounds.width - width) * 0.5f),
        std::round(bounds.y + (bounds.height - height) * 0.5f), width, height};
    if (width <= bounds.width)
    {
        rect.x = std::clamp(rect.x, bounds.x, bounds.x + bounds.width - width);
    }
    if (height <= bounds.height)
    {
        rect.y = std::clamp(rect.y, bounds.y, bounds.y + bounds.height - height);
    }
    return rect;
}
}
