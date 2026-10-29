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

// Only compact stack items are fitted; other item art retains its original extent.
inline std::pair<float, float> fitInventoryIcon(
    float width, float height, float cellWidth, float cellHeight, bool compact)
{
    if (compact && width > 0 && height > 0)
    {
        const float ratio = std::min({1.0f, std::max(0.0f, cellWidth) / width,
            std::max(0.0f, cellHeight) / height});
        width *= ratio;
        height *= ratio;
    }
    return {width, height};
}

inline InventoryIconRect placeInventoryIcon(
    InventoryIconRect bounds, float width, float height, bool compact)
{
    const auto extent = fitInventoryIcon(width, height, bounds.width, bounds.height, compact);
    InventoryIconRect result{std::round(bounds.x + (bounds.width - extent.first) * 0.5f),
        std::round(bounds.y + (bounds.height - extent.second) * 0.5f), extent.first, extent.second};
    if (compact)
    {
        result.x = std::clamp(result.x, bounds.x, bounds.x + std::max(0.0f, bounds.width - result.width));
        result.y = std::clamp(result.y, bounds.y, bounds.y + std::max(0.0f, bounds.height - result.height));
    }
    return result;
}
}
