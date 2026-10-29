#include "doctest/doctest.h"
#include "game/ui/InventoryIconGeometry.h"
#include "game/items/ReagentStacks.h"
#include "game/party/Party.h"
#include "tests/RegressionGameData.h"

using namespace OpenYAMM::Game;
TEST_CASE("Inventory hotfix compact icons stay inside last row right edge and neighboring cells at every scale")
{
    for (float scale : {0.5f, 1.f, 1.5f, 2.f, 3.f})
    for (uint32_t quantity : {1u, 3u, 4u, 20u, 50u})
    for (float width : {16.f, 32.f, 64.f, 128.f})
    for (float height : {24.f, 48.f, 96.f})
    {
        INFO(scale << " quantity=" << quantity);
        const float cell = 32 * scale;
        const InventoryIconRect bounds{10.25f + 13 * cell, 20.5f + 8 * cell, cell, cell};
        const auto rect = placeInventoryIcon(bounds, width * scale, height * scale, true);
        CHECK(rect.x >= bounds.x);
        CHECK(rect.y >= bounds.y);
        CHECK(rect.x + rect.width <= bounds.x + bounds.width + 0.001f);
        CHECK(rect.y + rect.height <= bounds.y + bounds.height + 0.001f);
        CHECK(rect.width / rect.height == doctest::Approx(width / height));
        const auto held = fitInventoryIcon(width * scale, height * scale, cell, cell, true);
        CHECK(rect.width == held.first);
        CHECK(rect.height == held.second);
    }
    const auto weapon = placeInventoryIcon({0, 0, 32, 32}, 64, 96, false);
    CHECK(weapon.width == 64); CHECK(weapon.height == 96);
}
TEST_CASE("Inventory hotfix reagent factories merging and old pickup preserve one cell logical bounds")
{
    REQUIRE(OpenYAMM::Tests::regressionGameDataLoaded());
    const auto &table = OpenYAMM::Tests::regressionGameData().itemTable;
    for (uint32_t id : {202u, 217u, 1018u})
    {
        const auto *definition = table.get(id);
        REQUIRE(definition);
        CHECK(definition->inventoryWidth == 1); CHECK(definition->inventoryHeight == 1);
        Character c;
        InventoryItem old; old.objectDescriptionId = id; old.quantity = 4;
        old.width = 2; old.height = 3; c.inventory.push_back(old);
        InventoryItem fresh = old; fresh.quantity = 3;
        REQUIRE(c.addInventoryItem(fresh, &table));
        REQUIRE(c.inventory.size() == 1);
        CHECK(c.inventory[0].quantity == 7);
        CHECK(c.inventory[0].width == 1); CHECK(c.inventory[0].height == 1);
    }
}
