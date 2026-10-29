#include "doctest/doctest.h"
#include "game/items/ItemRuntime.h"
#include "game/items/ReagentStacks.h"
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
}
TEST_CASE("Inventory hotfix known reagents and every potion use their own names with either raw flag")
{
    int checked = 0;
    for (const ItemDefinition &definition : data().itemTable.entries())
    {
        if (definition.equipStat != "Reagent" && definition.equipStat != "Bottle") continue;
        InventoryItem item; item.objectDescriptionId = definition.itemId;
        INFO(definition.itemId << " " << definition.name);
        for (bool identified : {false, true})
        {
            item.identified = identified;
            const auto expected = !identified && ItemRuntime::requiresIdentification(definition)
                ? definition.unidentifiedName : definition.name;
            CHECK(ItemRuntime::displayName(item, definition) == expected);
        }
        ++checked;
    }
    CHECK(checked >= 60);
    for (uint32_t id : {202u, 1018u, 229u, 239u})
    {
        const auto *definition = data().itemTable.get(id);
        REQUIRE(definition);
        CHECK_FALSE(ItemRuntime::requiresIdentification(*definition));
        InventoryItem item; item.objectDescriptionId = id; item.identified = false;
        CHECK(ItemRuntime::displayName(item, *definition) == definition->name);
    }
}
TEST_CASE("Inventory hotfix genuine unknown items stay unknown and reagent compatibility ignores only irrelevant flags")
{
    ItemDefinition unknown;
    unknown.name = "Hidden effect"; unknown.unidentifiedName = "Layered Potion";
    unknown.identifyRepairDifficulty = 5;
    InventoryItem item; item.identified = false;
    CHECK(ItemRuntime::displayName(item, unknown) == "Layered Potion");
    item.identified = true;
    CHECK(ItemRuntime::displayName(item, unknown) == "Hidden effect");
    InventoryItem a; a.objectDescriptionId = 202; a.quantity = 4;
    InventoryItem b = a; b.identified = false; b.quantity = 3;
    CHECK(ReagentStacks::compatible(a, b, &data().itemTable));
    Character c;
    REQUIRE(c.addInventoryItem(a, &data().itemTable));
    REQUIRE(c.addInventoryItem(b, &data().itemTable));
    REQUIRE(c.inventory.size() == 1);
    CHECK(c.inventory[0].quantity == 7);
    b.stolen = true;
    CHECK_FALSE(ReagentStacks::compatible(a, b, &data().itemTable));
    b.stolen = false; b.standardEnchantPower = 5;
    CHECK_FALSE(ReagentStacks::compatible(a, b, &data().itemTable));
}
TEST_CASE("Inventory hotfix old saves retain raw flags quantity power and effects without migration")
{
    std::string error;
    auto save = loadGameDataFromPath(std::filesystem::path(OPENYAMM_SOURCE_DIR)
        / "tests/fixtures/minotaur-1.14-v132.oysav", error);
    REQUIRE_MESSAGE(save, error);
    auto &items = save->party.members[0].inventory; items.clear();
    for (uint32_t id : {202u, 1018u, 229u, 239u})
    {
        InventoryItem item; item.objectDescriptionId = id; item.identified = false;
        item.quantity = id == 202 ? 4 : 1; item.standardEnchantPower = 17;
        items.push_back(item);
    }
    const auto path = std::filesystem::temp_directory_path() / "minotaur-names-hotfix.oysav";
    REQUIRE_MESSAGE(saveGameDataToPath(path, *save, error), error);
    const auto loaded = loadGameDataFromPath(path, error);
    REQUIRE_MESSAGE(loaded, error);
    CHECK(loaded->party.members[0].inventory == items);
    for (const auto &item : loaded->party.members[0].inventory)
        CHECK(ItemRuntime::displayName(item, *data().itemTable.get(item.objectDescriptionId))
            == data().itemTable.get(item.objectDescriptionId)->name);
    std::filesystem::remove(path);
}
