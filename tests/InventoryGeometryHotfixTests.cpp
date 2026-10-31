#include "doctest/doctest.h"
#include "game/ui/InventoryIconGeometry.h"
#include "game/items/ReagentStacks.h"
#include "game/items/InventoryBags.h"
#include "game/gameplay/ChestRuntime.h"
#include "game/maps/SaveGame.h"
#include "tests/RegressionGameData.h"

#include <algorithm>
#include <array>
#include <filesystem>

using namespace OpenYAMM::Game;
namespace
{
const ItemTable &table()
{
    REQUIRE_MESSAGE(OpenYAMM::Tests::regressionGameDataLoaded(), OpenYAMM::Tests::regressionGameDataFailure());
    return OpenYAMM::Tests::regressionGameData().itemTable;
}
InventoryItem stack(uint32_t id, uint32_t quantity)
{
    InventoryItem item;
    item.objectDescriptionId = id;
    item.quantity = quantity;
    return item;
}
bool validGrid(const std::vector<InventoryItem> &items)
{
    std::array<bool, Character::InventoryWidth * Character::InventoryHeight> used = {};
    for (const InventoryItem &item : items)
    {
        if (!item.width || !item.height || item.gridX + item.width > Character::InventoryWidth
            || item.gridY + item.height > Character::InventoryHeight)
        {
            return false;
        }
        for (int y = item.gridY; y < item.gridY + item.height; ++y)
        for (int x = item.gridX; x < item.gridX + item.width; ++x)
        {
            if (used[y * Character::InventoryWidth + x])
            {
                return false;
            }
            used[y * Character::InventoryWidth + x] = true;
        }
    }
    return true;
}
}

TEST_CASE("Inventory hotfix native icon scale and definition footprints never depend on quantity")
{
    REQUIRE_MESSAGE(table().get(217)->inventoryIconLogicalWidth == 63,
        "Native icon fixture unavailable: restore engine/icon_packages.txt and canonical icons_x2 PNGs");
    for (const ItemDefinition &definition : table().entries())
    {
        if (!ReagentStacks::eligible(stack(definition.itemId, 1), &table()))
        {
            continue;
        }
        for (const uint32_t quantity : {1u, 2u, 7u, 20u, 21u, 50u})
        {
            InventoryItem item = stack(definition.itemId, quantity);
            ReagentStacks::applyDefinitionGeometry(item, &table());
            CHECK(item.width == definition.inventoryWidth);
            CHECK(item.height == definition.inventoryHeight);
            for (const float scale : {0.5f, 1.f, 1.5f, 2.f, 3.f})
            {
                const float nativeWidth = definition.inventoryIconLogicalWidth * definition.inventoryDrawScale * scale;
                const float nativeHeight =
                    definition.inventoryIconLogicalHeight * definition.inventoryDrawScale * scale;
                const InventoryIconRect bounds{(14 - item.width) * 32.f * scale,
                    (9 - item.height) * 32.f * scale, item.width * 32.f * scale, item.height * 32.f * scale};
                const auto rect = placeInventoryIcon(bounds, nativeWidth, nativeHeight);
                CHECK(rect.width == nativeWidth);
                CHECK(rect.height == nativeHeight);
                CHECK(rect.x >= bounds.x - 0.501f);
                CHECK(rect.y >= bounds.y - 0.501f);
                CHECK(rect.x + rect.width <= bounds.x + bounds.width + 0.501f);
                CHECK(rect.y + rect.height <= bounds.y + bounds.height + 0.501f);
            }
        }
    }
    REQUIRE(table().get(217));
    CHECK(table().get(217)->inventoryWidth == 2);
    CHECK(table().get(217)->inventoryHeight == 1);
    CHECK(table().get(220)->inventoryWidth == 1);
    CHECK(table().get(220)->inventoryHeight == 2);
    CHECK(table().get(221)->inventoryWidth == table().get(220)->inventoryWidth);
    CHECK(table().get(221)->inventoryHeight == table().get(220)->inventoryHeight);
    // Approved correction of an upstream HD footprint that was smaller than its visible image.
    CHECK(table().get(1018)->inventoryWidth == 2);
    CHECK(table().get(1018)->inventoryHeight == 2);
}

TEST_CASE("Inventory hotfix multi cell merge needs no new cells and a split keeps its shape")
{
    Character c;
    REQUIRE(c.addInventoryItemAt(stack(220, 49), 13, 7, &table()));
    while (c.usedInventoryCells() < c.inventoryCapacity())
    {
        REQUIRE(c.addInventoryItem(stack(1, 1), &table()));
    }
    const auto before = c.inventory;
    CHECK_FALSE(c.addInventoryItem(stack(220, 3), &table()));
    CHECK(c.inventory == before);
    Character autoFill = c;
    REQUIRE(autoFill.addInventoryItem(stack(220, 1), &table()));
    CHECK(autoFill.inventory.front().quantity == 50);
    std::optional<InventoryItem> cursor;
    // A drop on the lower cell of the last-column bottle merges without requiring another1x2 area.
    REQUIRE(c.tryPlaceInventoryItemAt(stack(220, 3), 13, 8, cursor, &table()));
    REQUIRE(cursor);
    CHECK(cursor->quantity == 2);
    CHECK(cursor->width == 1);
    CHECK(cursor->height == 2);
    InventoryItem half;
    REQUIRE(c.takeInventoryItemAt(13, 8, half, ReagentStacks::takeCount(50, false, true)));
    CHECK(half.quantity == 25);
    CHECK(half.width == 1);
    CHECK(half.height == 2);
    CHECK(validGrid(c.inventory));
    Character second;
    REQUIRE(second.addInventoryItem(half, &table()));
    CHECK(validGrid(second.inventory));
    REQUIRE(InventoryBags::sortAllBags(second, 1, &table()));
    CHECK(second.inventory.front().height == 2);
}

TEST_CASE("Inventory hotfix legacy collisions repack only affected bags and preserve overflow idempotently")
{
    Character legacy;
    legacy.name = "Legacy full inventory";
    for (int y = 0; y < 9; ++y)
    for (int x = 0; x < 14; ++x)
    {
        InventoryItem item = stack(220, 50);
        item.gridX = uint8_t(x);
        item.gridY = uint8_t(y);
        legacy.inventory.push_back(item);
    }
    const size_t overflow = InventoryBags::restoreDefinitionGeometry(legacy, &table(), 1);
    CHECK(overflow == 70);
    CHECK(legacy.inventory.size() == 56);
    CHECK(validGrid(legacy.inventory));
    CHECK(InventoryBags::storedItemCount(legacy) == 70);
    uint32_t total = 0;
    for (const auto &item : legacy.inventory) total += item.quantity;
    for (const auto &[bag, items] : legacy.rulesetState.storedBags)
        for (const auto &item : items) total += item.quantity;
    CHECK(total == 6300);
    const auto inventory = legacy.inventory;
    const auto stored = legacy.rulesetState.storedBags;
    CHECK(InventoryBags::restoreDefinitionGeometry(legacy, &table(), 1) == 0);
    CHECK(legacy.inventory == inventory);
    CHECK(legacy.rulesetState.storedBags == stored);
    const auto recovered = InventoryBags::takeOverflow(legacy, 1);
    CHECK(recovered.size() == 70);
    for (const auto &item : recovered)
    {
        CHECK(item.quantity == 50);
        CHECK(item.width == 1);
        CHECK(item.height == 2);
    }
}

TEST_CASE("Inventory hotfix genuine old mod saves summarize restore and round trip under the new schema")
{
    for (const char *name : {"minotaur-1.14-v132.oysav", "minotaur-1.18-v140-stacks.oysav",
             "minotaur-1.18-v140-bottles.oysav"})
    {
        std::string error;
        const auto path = std::filesystem::path(OPENYAMM_SOURCE_DIR) / "tests/fixtures" / name;
        auto old = loadGameDataFromPath(path, error);
        REQUIRE_MESSAGE(old, error);
        const auto summary = loadGameSaveSummaryFromPath(path, error);
        REQUIRE_MESSAGE(summary, error);
        CHECK(summary->mapFileName == old->mapFileName);
        CHECK(summary->saveName == old->saveName);
        CHECK(summary->previewBmp == old->previewBmp);
        const auto original = old->party;
        Party party;
        party.setItemTable(&table());
        party.restoreSnapshot(original);
        CHECK(validGrid(party.member(0)->inventory));
        CHECK(party.member(0)->experience == original.members[0].experience);
        CHECK(party.member(0)->level == original.members[0].level);
        CHECK(party.member(0)->className == original.members[0].className);
        CHECK(party.snapshot().questBits == original.questBits);
        if (const auto *pSkill = original.members[0].findSkill("LethalStrike"))
        {
            REQUIRE(party.member(0)->findSkill("LethalStrike"));
            CHECK(party.member(0)->findSkill("LethalStrike")->level == pSkill->level);
            CHECK(party.member(0)->findSkill("LethalStrike")->mastery == pSkill->mastery);
        }
        old->party = party.snapshot();
        const auto next = std::filesystem::temp_directory_path() / "minotaur-multicell-roundtrip.oysav";
        REQUIRE_MESSAGE(saveGameDataToPath(next, *old, error), error);
        auto loaded = loadGameDataFromPath(next, error);
        REQUIRE_MESSAGE(loaded, error);
        CHECK(loaded->sourceSaveVersion == 142);
        CHECK(loaded->party.members[0].inventory == old->party.members[0].inventory);
        CHECK(loaded->party.members[0].rulesetState.storedBags == old->party.members[0].rulesetState.storedBags);
        CHECK(loaded->party.questBits == old->party.questBits);
        CHECK(loaded->mapFileName == old->mapFileName);
        CHECK(loaded->cameraYawRadians == old->cameraYawRadians);
        const auto newSummary = loadGameSaveSummaryFromPath(next, error);
        REQUIRE_MESSAGE(newSummary, error);
        CHECK(newSummary->mapFileName == summary->mapFileName);
        std::filesystem::remove(next);
    }
}

TEST_CASE("Inventory hotfix resized chest stacks retain hidden overflow and reveal it as room opens")
{
    GameplayChestViewState chest;
    chest.gridWidth = chest.gridHeight = 2;
    for (int i = 0; i < 4; ++i)
    {
        GameplayChestItemState entry;
        entry.item = stack(220, 50);
        entry.itemId = 220;
        entry.quantity = 50;
        entry.gridX = uint8_t(i % 2);
        entry.gridY = uint8_t(i / 2);
        chest.items.push_back(entry);
    }
    restoreChestDefinitionGeometry(chest, &table());
    CHECK(chest.items.size() == 2);
    CHECK(chest.hiddenItems.size() == 2);
    restoreChestDefinitionGeometry(chest, &table());
    CHECK(chest.items.size() == 2);
    CHECK(chest.hiddenItems.size() == 2);
    uint32_t total = 0;
    while (!chest.items.empty())
    {
        GameplayChestItemState item;
        REQUIRE(takeChestItem(chest, 0, item));
        CHECK(item.height == 2);
        CHECK(item.item.height == 2);
        total += item.quantity;
    }
    CHECK(total == 200);
    CHECK(chest.hiddenItems.empty());
}

TEST_CASE("Inventory hotfix overflow is serialized and restored before a world can accept the items")
{
    Character legacy;
    legacy.name = "Overflow save";
    legacy.className = "MinotaurLord";
    legacy.level = 50;
    legacy.experience = 123456;
    legacy.skills["LethalStrike"] = {"LethalStrike", 7, SkillMastery::Master};
    for (int y = 0; y < 9; ++y)
    for (int x = 0; x < 14; ++x)
    {
        InventoryItem item = stack(220, 50);
        item.gridX = uint8_t(x);
        item.gridY = uint8_t(y);
        legacy.inventory.push_back(item);
    }
    Party::Snapshot snapshot;
    snapshot.members.push_back(legacy);
    snapshot.questBits.insert(1234);
    Party party;
    party.setItemTable(&table());
    party.restoreSnapshot(snapshot);
    GameSaveData save;
    save.mapFileName = "out01.odm";
    save.savedGameMinutes = 12345;
    save.cameraYawRadians = 1.25f;
    save.party = party.snapshot();
    const auto path = std::filesystem::temp_directory_path() / "minotaur-overflow-retention.oysav";
    std::string error;
    REQUIRE_MESSAGE(saveGameDataToPath(path, save, error), error);
    const auto loaded = loadGameDataFromPath(path, error);
    REQUIRE_MESSAGE(loaded, error);
    Party restored;
    restored.setItemTable(&table());
    restored.restoreSnapshot(loaded->party);
    CHECK(restored.member(0)->inventory == party.member(0)->inventory);
    CHECK(restored.member(0)->rulesetState.storedBags == party.member(0)->rulesetState.storedBags);
    CHECK(restored.member(0)->experience == 123456);
    CHECK(restored.member(0)->findSkill("LethalStrike")->mastery == SkillMastery::Master);
    CHECK(restored.hasQuestBit(1234));
    CHECK(loaded->savedGameMinutes == 12345);
    CHECK(loaded->cameraYawRadians == 1.25f);
    CHECK(InventoryBags::takeOverflow(*restored.member(0), 1).size() == 70);
    std::filesystem::remove(path);
}

TEST_CASE("Inventory hotfix migration reserves unchanged equipment and quest item coordinates")
{
    Character c;
    c.inventory.push_back(stack(220, 50));
    InventoryItem unchanged = stack(1, 1);
    unchanged.gridY = 1;
    unchanged.identified = false;
    unchanged.stolen = true;
    c.inventory.push_back(unchanged);
    CHECK(InventoryBags::restoreDefinitionGeometry(c, &table(), 1) == 0);
    CHECK(validGrid(c.inventory));
    const auto found = std::find_if(c.inventory.begin(), c.inventory.end(),
        [](const auto &item) { return item.objectDescriptionId == 1; });
    REQUIRE(found != c.inventory.end());
    CHECK(*found == unchanged);
    CHECK(c.inventoryItemAt(0, 1)->objectDescriptionId == 1);
}

TEST_CASE("Inventory hotfix heterogeneous multi cell stacks sort using their complete native areas")
{
    Character c;
    for (const auto [id, count] : {std::pair{1018u, 125u}, {217u, 21u}, {220u, 51u}, {200u, 50u}, {1004u, 3u}})
    {
        REQUIRE(c.addInventoryItem(stack(id, count), &table()));
    }
    CHECK(c.usedInventoryCells() == 22);
    CHECK(validGrid(c.inventory));
    REQUIRE(InventoryBags::sortAllBags(c, 1, &table()));
    CHECK(c.usedInventoryCells() == 22);
    CHECK(validGrid(c.inventory));
    for (const InventoryItem &item : c.inventory)
    {
        CHECK(item.width == table().get(item.objectDescriptionId)->inventoryWidth);
        CHECK(item.height == table().get(item.objectDescriptionId)->inventoryHeight);
        CHECK(item.quantity <= 50);
    }
}

TEST_CASE("Inventory hotfix old prepared potions recover their bottle footprint without becoming stackable")
{
    Character c;
    c.inventory.push_back(stack(221, 1));
    InventoryItem second = stack(221, 1);
    second.gridY = 1;
    c.inventory.push_back(second);
    CHECK(InventoryBags::restoreDefinitionGeometry(c, &table(), 1) == 0);
    REQUIRE(c.inventory.size() == 2);
    CHECK(validGrid(c.inventory));
    for (const auto &item : c.inventory)
    {
        CHECK(item.height == 2);
        CHECK(item.quantity == 1);
        CHECK_FALSE(ReagentStacks::eligible(item, &table()));
    }
}
