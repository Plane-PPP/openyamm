#include "doctest/doctest.h"

#include "game/items/ReagentStacks.h"
#include "game/items/AlchemyBenchRuntime.h"
#include "game/items/InventoryBags.h"
#include "game/items/InventoryItemMixingRuntime.h"
#include "game/items/PriceCalculator.h"
#include "game/gameplay/HouseServiceRuntime.h"
#include "game/tables/HouseTable.h"
#include "game/maps/SaveGame.h"
#include "tests/RegressionGameData.h"

#include <array>
#include <filesystem>

using namespace OpenYAMM::Game;
namespace
{
const OpenYAMM::Tests::RegressionGameData &tables()
{
    REQUIRE_MESSAGE(OpenYAMM::Tests::regressionGameDataLoaded(), OpenYAMM::Tests::regressionGameDataFailure());
    return OpenYAMM::Tests::regressionGameData();
}
InventoryItem reagent(uint32_t count = 1, uint32_t id = 217)
{
    InventoryItem item;
    item.objectDescriptionId = id;
    item.quantity = count;
    return item;
}
Party partyWithMembers(size_t count = 1)
{
    Party party;
    party.setItemTable(&tables().itemTable);
    PartySeed seed;
    for (size_t i = 0; i < count; ++i)
    {
        Character c;
        c.name = "Alchemy tester";
        c.className = c.role = "Cleric";
        c.level = 20;
        c.health = c.maxHealth = 100;
        c.skills["Alchemy"] = {"Alchemy", 4, SkillMastery::Expert};
        seed.members.push_back(c);
    }
    party.seed(seed);
    return party;
}
AlchemyBenchTables benchTables()
{
    const auto &d = tables();
    return {d.itemTable, d.potionMixingTable, d.potionNoteTable, d.mergedPotionSettingTable,
        d.mergedReagentSettingTable};
}
InventoryItemMixResult mix(InventoryItem held, InventoryItem &target)
{
    const auto &d = tables();
    return InventoryItemMixingRuntime::mixItems(held, target, SkillMastery::Expert, 29,
        d.itemTable, d.potionMixingTable, d.mergedPotionSettingTable, d.mergedReagentSettingTable,
        &d.potionNoteTable);
}
}

TEST_CASE("Reagent stacks auto merge matrix and 125 units occupy three cells")
{
    for (const auto row : {std::array<uint32_t, 3>{10, 15, 25}, {25, 25, 50}, {49, 3, 52},
            {50, 1, 51}, {48, 5, 53}, {42, 15, 57}})
    {
        Character c;
        REQUIRE(c.addInventoryItem(reagent(row[0]), &tables().itemTable));
        REQUIRE(c.addInventoryItem(reagent(row[1]), &tables().itemTable));
        REQUIRE(c.inventory.size() == (row[2] + 49) / 50);
        CHECK(c.inventory[0].quantity == std::min(50u, row[2]));
        if (row[2] > 50) CHECK(c.inventory[1].quantity == row[2] - 50);
        CHECK(c.usedInventoryCells() == c.inventory.size());
    }
    Character c;
    REQUIRE(c.addInventoryItem(reagent(125), &tables().itemTable));
    REQUIRE(c.inventory.size() == 3);
    CHECK(c.inventory[0].quantity == 50);
    CHECK(c.inventory[1].quantity == 50);
    CHECK(c.inventory[2].quantity == 25);
}

TEST_CASE("Reagent stacks preserve identity metadata and do not stack potions or equipment")
{
    Character c;
    REQUIRE(c.addInventoryItem(reagent(25), &tables().itemTable));
    REQUIRE(c.addInventoryItem(reagent(25, 200), &tables().itemTable));
    InventoryItem stolen = reagent(25); stolen.stolen = true;
    REQUIRE(c.addInventoryItem(stolen, &tables().itemTable));
    InventoryItem modified = reagent(25); modified.rulesetFields.qualityPercent = 120;
    REQUIRE(c.addInventoryItem(modified, &tables().itemTable));
    CHECK(c.inventory.size() == 4);
    CHECK_FALSE(ReagentStacks::compatible(stolen, reagent(25), &tables().itemTable));
    REQUIRE(c.addInventoryItem(reagent(1, 220), &tables().itemTable));
    REQUIRE(c.addInventoryItem(reagent(1, 220), &tables().itemTable));
    CHECK(c.inventory.size() == 5);
    CHECK(c.inventory.back().quantity == 2);
    InventoryItem unique = reagent(); unique.artifactId = 1;
    CHECK_FALSE(ReagentStacks::eligible(unique, &tables().itemTable));
    CHECK_FALSE(ReagentStacks::eligible(reagent(1, 1), &tables().itemTable));
}

TEST_CASE("Reagent stacks full inventory insertion is atomic and can fill existing stack")
{
    Character c;
    REQUIRE(c.addInventoryItem(reagent(42), &tables().itemTable));
    while (c.usedInventoryCells() < c.inventoryCapacity())
    {
        REQUIRE(c.addInventoryItem(reagent(1, 1), &tables().itemTable));
    }
    const auto original = c.inventory;
    CHECK_FALSE(c.addInventoryItem(reagent(15), &tables().itemTable));
    CHECK(c.inventory == original);
    REQUIRE(c.addInventoryItem(reagent(8), &tables().itemTable));
    CHECK(c.inventory[0].quantity == 50);
    CHECK_FALSE(c.addInventoryItem(reagent(), &tables().itemTable));
}

TEST_CASE("Reagent stacks drag merge leaves excess on cursor and splitting conserves all units")
{
    Character c;
    REQUIRE(c.addInventoryItem(reagent(25), &tables().itemTable));
    std::optional<InventoryItem> cursor;
    REQUIRE(c.tryPlaceInventoryItemAt(reagent(30), 0, 0, cursor, &tables().itemTable));
    CHECK(c.inventory[0].quantity == 50);
    REQUIRE(cursor);
    CHECK(cursor->quantity == 5);
    const InventoryItem excess = *cursor;
    REQUIRE(c.tryPlaceInventoryItemAt(excess, 0, 0, cursor, &tables().itemTable));
    REQUIRE(cursor);
    CHECK(cursor->quantity == 5);
    CHECK(ReagentStacks::takeCount(49, false, true) == 25);
    CHECK(ReagentStacks::takeCount(49, true, false) == 1);
    CHECK(ReagentStacks::takeCount(49, false, false) == 49);
    CHECK(ReagentStacks::takeCount(49, true, true) == 1);
    InventoryItem one;
    REQUIRE(c.takeInventoryItemAt(0, 0, one, 1));
    CHECK(one.quantity == 1);
    CHECK(c.inventory[0].quantity == 49);
    InventoryItem half;
    REQUIRE(c.takeInventoryItemAt(0, 0, half, 25));
    CHECK(half.quantity + one.quantity + c.inventory[0].quantity == 50);
}

TEST_CASE("Reagent stacks sorting coalesces old singles and transferring preserves quantity")
{
    Party party = partyWithMembers(2);
    for (int i = 0; i < 50; ++i)
    {
        REQUIRE(party.member(0)->addInventoryItem(reagent())); // Deliberate legacy singles, no table.
    }
    REQUIRE(party.sortMemberInventory(0));
    REQUIRE(party.member(0)->inventory.size() == 1);
    CHECK(party.inventoryItemCount(217) == 50);
    InventoryItem half;
    REQUIRE(party.takeItemFromMemberInventoryCell(0, 0, 0, half, 25));
    REQUIRE(party.tryAutoPlaceItemInMemberInventory(1, half));
    CHECK(party.inventoryItemCount(217, 0) == 25);
    CHECK(party.inventoryItemCount(217, 1) == 25);
    CHECK(party.inventoryItemCount(217) == 50);
    REQUIRE(InventoryBags::sortAllBags(*party.member(1), 1, party.itemTable()));
    CHECK(party.inventoryItemCount(217) == 50);
}

TEST_CASE("Reagent stacks brewing consumes exactly one including last and both cursor orientations")
{
    InventoryItem held = reagent(50);
    for (uint32_t remaining = 50; remaining > 0; --remaining)
    {
        InventoryItem target = reagent(1, 220);
        const auto result = mix(held, target);
        REQUIRE(result.success);
        CHECK(target.objectDescriptionId == 221);
        CHECK(target.quantity == 1);
        CHECK(target.standardEnchantPower == 39);
        if (remaining > 1)
        {
            REQUIRE(result.heldItemReplacement);
            CHECK(result.heldItemReplacement->quantity == remaining - 1);
            held = *result.heldItemReplacement;
        }
        else CHECK_FALSE(result.heldItemReplacement);
    }
    InventoryItem target = reagent(50);
    const auto result = mix(reagent(1, 220), target);
    REQUIRE(result.success);
    CHECK(target.objectDescriptionId == 217);
    CHECK(target.quantity == 49);
    REQUIRE(result.heldItemReplacement);
    CHECK(result.heldItemReplacement->objectDescriptionId == 221);
    CHECK(result.heldItemReplacement->quantity == 1);
    const InventoryItem original = target;
    CHECK_FALSE(mix(reagent(), target).handled);
    CHECK(target == original);
}

TEST_CASE("Reagent stacks alchemy bench repeatedly consumes one and exposes correct stock")
{
    Party party = partyWithMembers();
    REQUIRE(party.tryGrantInventoryItemStartingAt(0, reagent(50)));
    for (int i = 0; i < 50; ++i) REQUIRE(party.tryGrantInventoryItemStartingAt(0, reagent(1, 220)));
    const auto ingredients = AlchemyBenchRuntime::collectIngredients(party, benchTables());
    const auto ingredient = std::find_if(ingredients.begin(), ingredients.end(),
        [](const auto &row) { return row.itemId == 217; });
    REQUIRE(ingredient != ingredients.end());
    CHECK(ingredient->count == 50);
    for (int i = 49; i >= 0; --i)
    {
        const auto outcome = AlchemyBenchRuntime::mix(party, {220, 217}, benchTables());
        REQUIRE(outcome.result.success);
        CHECK(party.inventoryItemCount(217) == i);
        CHECK(party.inventoryItemCount(221) == 50 - i);
    }
}

TEST_CASE("Reagent stacks quantity round trips in compact saves and partial sale is priced by units")
{
    std::string error;
    auto save = loadGameDataFromPath(std::filesystem::path(OPENYAMM_SOURCE_DIR)
        / "tests/fixtures/minotaur-1.14-v132.oysav", error);
    REQUIRE_MESSAGE(save, error);
    save->party.members[0].inventory.clear();
    REQUIRE(save->party.members[0].addInventoryItem(reagent(125), &tables().itemTable));
    const auto path = std::filesystem::temp_directory_path() / "minotaur-reagent-stack-save.oysav";
    REQUIRE_MESSAGE(saveGameDataToPath(path, *save, error), error);
    const auto loaded = loadGameDataFromPath(path, error);
    REQUIRE_MESSAGE(loaded, error);
    CHECK(loaded->party.members[0].inventory == save->party.members[0].inventory);
    CHECK(loaded->party.members[0].findSkill("LethalStrike")->level == 7);
    const auto *definition = tables().itemTable.get(217);
    REQUIRE(definition);
    InventoryItem part = reagent(25);
    InventoryItem single = reagent();
    CHECK(PriceCalculator::itemSellingPrice(nullptr, part, *definition, 1.0f)
        == 25 * PriceCalculator::itemSellingPrice(nullptr, single, *definition, 1.0f));
    std::filesystem::remove(path);
}

TEST_CASE("Reagent stacks merchant sale of a split portion preserves the rest and buy prices count units")
{
    Party party = partyWithMembers();
    REQUIRE(party.tryGrantInventoryItemStartingAt(0, reagent(50)));
    InventoryItem portion;
    REQUIRE(party.takeItemFromMemberInventoryCell(0, 0, 0, portion, 25));
    std::optional<InventoryItem> replaced;
    REQUIRE(party.tryPlaceItemInMemberInventoryCell(0, portion, 1, 0, replaced));
    HouseEntry shop;
    shop.type = "Alchemist";
    shop.priceMultiplier = 1.0f;
    const auto &data = tables();
    const int price = HouseServiceRuntime::sellPrice(party, data.itemTable, data.standardItemEnchantTable,
        data.specialItemEnchantTable, shop, portion);
    const int before = party.gold();
    std::string text;
    REQUIRE(HouseServiceRuntime::trySellInventoryItem(party, data.itemTable, data.standardItemEnchantTable,
        data.specialItemEnchantTable, shop, 0, 1, 0, text));
    CHECK(party.gold() == before + price);
    CHECK(party.inventoryItemCount(217) == 25);
    CHECK(HouseServiceRuntime::buyPrice(party, data.itemTable, data.standardItemEnchantTable,
        data.specialItemEnchantTable, shop, portion)
        == 25 * HouseServiceRuntime::buyPrice(party, data.itemTable, data.standardItemEnchantTable,
            data.specialItemEnchantTable, shop, reagent()));
}

TEST_CASE("Reagent stacks held items floor objects chests and corpses retain quantity in saves")
{
    GameSaveData save;
    save.mapFileName = "out01.odm";
    save.heldInventoryItemActive = true;
    save.heldInventoryItem = reagent(7);
    auto &world = save.outdoorWorldStates["out01.odm"];
    OutdoorWorldRuntime::WorldItemState dropped;
    dropped.worldItemId = 1;
    dropped.item = reagent(50);
    world.worldItems.push_back(dropped);
    GameplayChestItemState loot;
    loot.item = reagent(25);
    loot.itemId = 217;
    loot.quantity = 25;
    world.activeChestView = GameplayChestViewState{};
    world.activeChestView->items.push_back(loot);
    world.activeCorpseView = GameplayCorpseViewState{};
    world.activeCorpseView->items.push_back(loot);
    const auto path = std::filesystem::temp_directory_path() / "minotaur-reagent-world-save.oysav";
    std::string error;
    REQUIRE_MESSAGE(saveGameDataToPath(path, save, error), error);
    auto loaded = loadGameDataFromPath(path, error);
    REQUIRE_MESSAGE(loaded, error);
    CHECK(loaded->heldInventoryItem.quantity == 7);
    const auto &restored = loaded->outdoorWorldStates.at("out01.odm");
    REQUIRE(restored.worldItems.size() == 1);
    CHECK(restored.worldItems[0].item == dropped.item);
    REQUIRE(restored.activeChestView);
    CHECK(restored.activeChestView->items[0].item.quantity == 25);
    REQUIRE(restored.activeCorpseView);
    CHECK(restored.activeCorpseView->items[0].item.quantity == 25);
    std::filesystem::remove(path);
}
TEST_CASE("Reagent stacks compact save omits baked blood geometry without losing splat metadata")
{
    GameSaveData save;
    save.mapFileName = "out01.odm";
    save.party = partyWithMembers().snapshot();
    save.hasOutdoorRuntimeState = true;
    save.hasIndoorSceneState = true;
    OutdoorWorldRuntime::BloodSplatState outdoor;
    outdoor.sourceActorId = 7; outdoor.radius = 123; outdoor.x = 42;
    outdoor.vertices.resize(1200);
    save.outdoorWorld.bloodSplats.push_back(outdoor);
    IndoorWorldRuntime::BloodSplatState indoor;
    indoor.sourceActorId = 8; indoor.radius = 124; indoor.y = 43;
    indoor.vertices.resize(1200);
    save.indoorScene.worldRuntime.bloodSplats.push_back(indoor);
    const auto a = std::filesystem::temp_directory_path() / "minotaur-compact-geometry-a.oysav";
    const auto b = std::filesystem::temp_directory_path() / "minotaur-compact-geometry-b.oysav";
    std::string error;
    REQUIRE_MESSAGE(saveGameDataToPath(a, save, error), error);
    save.outdoorWorld.bloodSplats[0].vertices.clear();
    save.indoorScene.worldRuntime.bloodSplats[0].vertices.clear();
    REQUIRE_MESSAGE(saveGameDataToPath(b, save, error), error);
    CHECK(std::filesystem::file_size(a) == std::filesystem::file_size(b));
    const auto loaded = loadGameDataFromPath(a, error);
    REQUIRE_MESSAGE(loaded, error);
    REQUIRE(loaded->outdoorWorld.bloodSplats.size() == 1);
    REQUIRE(loaded->indoorScene.worldRuntime.bloodSplats.size() == 1);
    CHECK(loaded->outdoorWorld.bloodSplats[0].sourceActorId == 7);
    CHECK(loaded->outdoorWorld.bloodSplats[0].radius == 123);
    CHECK(loaded->outdoorWorld.bloodSplats[0].vertices.empty());
    CHECK(loaded->indoorScene.worldRuntime.bloodSplats[0].sourceActorId == 8);
    CHECK(loaded->indoorScene.worldRuntime.bloodSplats[0].radius == 124);
    CHECK(loaded->indoorScene.worldRuntime.bloodSplats[0].vertices.empty());
    std::filesystem::remove(a); std::filesystem::remove(b);
}
