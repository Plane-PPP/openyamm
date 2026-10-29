#include "doctest/doctest.h"
#include "game/items/ReagentStacks.h"
#include "game/items/InventoryItemMixingRuntime.h"
#include "game/items/AlchemyBenchRuntime.h"
#include "game/items/InventoryBags.h"
#include "game/items/PriceCalculator.h"
#include "game/maps/SaveGame.h"
#include "tests/RegressionGameData.h"

using namespace OpenYAMM::Game;
namespace
{
const OpenYAMM::Tests::RegressionGameData &data()
{
    REQUIRE_MESSAGE(OpenYAMM::Tests::regressionGameDataLoaded(), OpenYAMM::Tests::regressionGameDataFailure());
    return OpenYAMM::Tests::regressionGameData();
}
InventoryItem stack(uint32_t id, uint32_t count)
{
    InventoryItem item; item.objectDescriptionId = id; item.quantity = count; return item;
}
Party makeParty(const ItemTable &table)
{
    Party party; party.setItemTable(&table);
    Character c; c.name = "Bottle tester"; c.className = "Cleric"; c.level = 20;
    c.maxHealth = c.health = 100;
    c.skills["Alchemy"] = {"Alchemy", 4, SkillMastery::Expert};
    PartySeed seed; seed.members.push_back(c); party.seed(seed);
    party.member(0)->itemSkillBonuses["Alchemy"] = 25;
    return party;
}
uint32_t heldCount(const InventoryItem &held, uint32_t id)
{
    return held.objectDescriptionId == id ? held.quantity : 0;
}
InventoryItemMixResult brew(Party &party, InventoryItem &held, const ItemTable &table)
{
    const auto target = party.member(0)->inventory.front();
    return InventoryItemMixingRuntime::tryApplyHeldItemToInventoryItem(party, 0, held, target.gridX, target.gridY,
        table, data().potionMixingTable, data().mergedPotionSettingTable, data().mergedReagentSettingTable,
        &data().potionNoteTable);
}
AlchemyBenchTables bench(const ItemTable &table)
{
    return {table, data().potionMixingTable, data().potionNoteTable,
        data().mergedPotionSettingTable, data().mergedReagentSettingTable};
}
void fill(Party &party)
{
    while (party.member(0)->usedInventoryCells() < party.member(0)->inventoryCapacity())
        REQUIRE(party.tryAutoPlaceItemInMemberInventory(0, stack(1, 1)));
}
}
TEST_CASE("Bottle stacks use canonical empty bottle data and keep all prepared potions separate")
{
    const auto &table = data().itemTable;
    REQUIRE(table.get(220)->isEmptyAlchemyBottle);
    CHECK(table.get(220)->inventoryWidth == 1); CHECK(table.get(220)->inventoryHeight == 1);
    Character c;
    REQUIRE(c.addInventoryItem(stack(220, 45), &table));
    REQUIRE(c.addInventoryItem(stack(220, 10), &table));
    REQUIRE(c.inventory.size() == 2);
    CHECK(c.inventory[0].quantity == 50); CHECK(c.inventory[1].quantity == 5);
    Character other;
    REQUIRE(other.addInventoryItem(stack(220, 65), &table));
    REQUIRE(other.inventory.size() == 2);
    CHECK(other.inventory[0].quantity == 50); CHECK(other.inventory[1].quantity == 15);
    for (const auto &definition : table.entries())
        if (definition.hasPotionPower) CHECK_FALSE(ReagentStacks::eligible(stack(definition.itemId, 1), &table));
}
TEST_CASE("Bottle stacks cursor brewing consumes one from both inputs in either orientation")
{
    const auto &table = data().itemTable;
    for (bool heldBottle : {false, true})
    for (uint32_t reagentCount : {1u, 2u, 20u, 50u})
    for (uint32_t bottles : {1u, 2u, 20u, 50u})
    {
        Party party = makeParty(table);
        REQUIRE(party.tryAutoPlaceItemInMemberInventory(0, stack(heldBottle ? 217 : 220,
            heldBottle ? reagentCount : bottles)));
        InventoryItem held = stack(heldBottle ? 220 : 217, heldBottle ? bottles : reagentCount);
        const auto result = brew(party, held, table);
        REQUIRE(result.success);
        held = result.heldItemReplacement.value_or(InventoryItem{});
        CHECK(party.inventoryItemCount(217) + heldCount(held, 217) == reagentCount - 1);
        CHECK(party.inventoryItemCount(220) + heldCount(held, 220) == bottles - 1);
        CHECK(party.inventoryItemCount(221) + heldCount(held, 221) == 1);
        for (const auto &item : party.member(0)->inventory)
            if (item.objectDescriptionId == 221)
            {
                CHECK(item.quantity == 1); CHECK(item.standardEnchantPower == 39);
                CHECK(item.width == table.get(221)->inventoryWidth);
                CHECK(item.height == table.get(221)->inventoryHeight);
            }
    }
}
TEST_CASE("Bottle stacks full inventory refusal is atomic for two remaining stacks")
{
    const auto &table = data().itemTable;
    for (bool heldBottle : {false, true})
    {
        Party party = makeParty(table);
        REQUIRE(party.tryAutoPlaceItemInMemberInventory(0, stack(heldBottle ? 217 : 220, 20)));
        fill(party);
        const auto before = party.member(0)->inventory;
        InventoryItem held = stack(heldBottle ? 220 : 217, 20);
        const auto original = held;
        const auto result = brew(party, held, table);
        CHECK(result.handled); CHECK_FALSE(result.success); CHECK_FALSE(result.heldItemConsumed);
        CHECK(party.member(0)->inventory == before); CHECK(held == original);
    }
}
TEST_CASE("Bottle stacks last ingredients at bottom edge hand a tall potion to the freed cursor")
{
    ItemTable table = data().itemTable;
    std::string error;
    REQUIRE_MESSAGE(table.loadVisualRows({{"inventory", "221", "", "", "", "", "1", "2", "1", "1"}}, error), error);
    Party party = makeParty(table);
    REQUIRE(party.member(0)->addInventoryItemAt(stack(220, 1), 13, 8, &table));
    fill(party);
    InventoryItem held = stack(217, 1);
    const auto result = brew(party, held, table);
    REQUIRE(result.success);
    REQUIRE(result.heldItemReplacement);
    CHECK(result.heldItemReplacement->objectDescriptionId == 221);
    CHECK(result.heldItemReplacement->height == 2);
    CHECK(party.inventoryItemCount(220) == 0);
    CHECK(party.inventoryItemCount(221) == 0);
}
TEST_CASE("Bottle stacks bench consumes both stocks repeatedly and safely returns an overflow product")
{
    const auto &table = data().itemTable;
    Party party = makeParty(table);
    REQUIRE(party.tryGrantInventoryItem(stack(217, 20)));
    REQUIRE(party.tryGrantInventoryItem(stack(220, 20)));
    for (int count = 19; count >= 0; --count)
    {
        const auto outcome = AlchemyBenchRuntime::mix(party, {220, 217}, bench(table));
        REQUIRE(outcome.result.success);
        CHECK(party.inventoryItemCount(220) == count); CHECK(party.inventoryItemCount(217) == count);
        CHECK(party.inventoryItemCount(221) == 20 - count);
    }
    Party full = makeParty(table);
    REQUIRE(full.tryGrantInventoryItem(stack(217, 20)));
    REQUIRE(full.tryGrantInventoryItem(stack(220, 20)));
    fill(full);
    const auto result = AlchemyBenchRuntime::mix(full, {220, 217}, bench(table));
    REQUIRE(result.result.success); REQUIRE(result.overflowItem);
    CHECK(result.overflowItem->objectDescriptionId == 221); CHECK(result.overflowItem->quantity == 1);
    CHECK(full.inventoryItemCount(217) == 19); CHECK(full.inventoryItemCount(220) == 19);
}
TEST_CASE("Bottle stacks old v140 saves load unchanged and bottle splits bags sorting value round trip")
{
    const auto &table = data().itemTable;
    std::string error;
    auto old = loadGameDataFromPath(std::filesystem::path(OPENYAMM_SOURCE_DIR)
        / "tests/fixtures/minotaur-1.18-v140-stacks.oysav", error);
    REQUIRE_MESSAGE(old, error);
    // The mod140 envelope decodes to its upstream139 payload version.
    CHECK(old->sourceSaveVersion == 139);
    uint32_t total = 0;
    for (const auto &item : old->party.members[0].inventory)
    {
        if (item.objectDescriptionId == 217)
        {
            total += item.quantity;
        }
    }
    CHECK(total == 125);
    Party party = makeParty(table);
    REQUIRE(party.tryGrantInventoryItem(stack(220, 50)));
    InventoryItem portion;
    REQUIRE(InventoryBags::takeFromBag(party, 0, 0, 2, 0, 0, portion, 25));
    CHECK(portion.quantity == 25); CHECK(party.inventoryItemCount(220) == 25);
    std::optional<InventoryItem> replaced;
    REQUIRE(InventoryBags::placeInBag(party, 0, 1, 2, portion, 0, 0, replaced));
    REQUIRE(InventoryBags::sortAllBags(*party.member(0), 2, &table));
    CHECK(party.inventoryItemCount(220) == 50);
    old->party = party.snapshot();
    const auto path = std::filesystem::temp_directory_path() / "minotaur-bottle-hotfix.oysav";
    REQUIRE_MESSAGE(saveGameDataToPath(path, *old, error), error);
    const auto loaded = loadGameDataFromPath(path, error);
    REQUIRE_MESSAGE(loaded, error);
    CHECK(loaded->party.members[0].inventory == old->party.members[0].inventory);
    CHECK(loaded->party.members[0].rulesetState.storedBags == old->party.members[0].rulesetState.storedBags);
    const auto price = [&](uint32_t count)
    { return PriceCalculator::itemSellingPrice(nullptr, stack(220, count), *table.get(220), 1.f); };
    CHECK(price(25) == 25 * price(1));
    std::filesystem::remove(path);
}

TEST_CASE("Bottle stacks manual overflow split and transfer conserve quantity")
{
    const ItemTable &table = data().itemTable;
    Party party = makeParty(table);
    PartySeed seed;
    seed.members.push_back(*party.member(0));
    seed.members.push_back(seed.members.front());
    party.seed(seed);
    REQUIRE(party.member(0)->addInventoryItem(stack(220, 45), &table));
    std::optional<InventoryItem> cursor;
    REQUIRE(party.tryPlaceItemInMemberInventoryCell(0, stack(220, 10), 0, 0, cursor));
    REQUIRE(cursor);
    CHECK(cursor->quantity == 5);
    CHECK(party.member(0)->inventory.front().quantity == 50);
    REQUIRE(party.tryAutoPlaceItemInMemberInventory(1, *cursor));
    InventoryItem one;
    REQUIRE(party.takeItemFromMemberInventoryCell(0, 0, 0, one, ReagentStacks::takeCount(50, true, false)));
    CHECK(one.quantity == 1);
    REQUIRE(party.tryAutoPlaceItemInMemberInventory(1, one));
    CHECK(party.member(0)->inventory.front().quantity == 49);
    CHECK(party.member(1)->inventory.front().quantity == 6);
    CHECK(party.inventoryItemCount(220) == 55);
}
