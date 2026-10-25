#include "game/items/InventoryBags.h"
#include "game/items/QuickItemSlots.h"
#include "game/party/Party.h"

#include <doctest/doctest.h>

using OpenYAMM::Game::Character;
using OpenYAMM::Game::InventoryItem;
using OpenYAMM::Game::QuickItemBottleRule;
using OpenYAMM::Game::QuickItemSlot;
namespace QuickItemSlots = OpenYAMM::Game::QuickItemSlots;

namespace
{
InventoryItem bottle(uint32_t itemId, uint16_t power, uint8_t gridX)
{
    InventoryItem item;
    item.objectDescriptionId = itemId;
    item.standardEnchantPower = power;
    item.gridX = gridX;
    return item;
}
}

TEST_CASE("a quick item slot uses the weakest bottle of its kind, or the strongest when asked")
{
    // The potion radial (GAPS.md H.6): a slot holds a kind; the weakest bottle by default saves the strong ones.
    Character member;
    member.inventory = {bottle(222, 40, 0), bottle(222, 12, 1), bottle(223, 5, 2), bottle(222, 60, 3)};
    QuickItemSlot slot;
    slot.itemId = 222;

    std::optional<QuickItemSlots::Location> found = QuickItemSlots::findItem(member, slot, 1);
    REQUIRE(found.has_value());
    CHECK_EQ(found->gridX, 1);

    slot.bottleRule = QuickItemBottleRule::Strongest;
    found = QuickItemSlots::findItem(member, slot, 1);
    REQUIRE(found.has_value());
    CHECK_EQ(found->gridX, 3);

    CHECK_EQ(QuickItemSlots::countItems(member, slot, 1), 3u);
    slot.itemId = 230;
    CHECK_FALSE(QuickItemSlots::findItem(member, slot, 1).has_value());
    CHECK_EQ(QuickItemSlots::countItems(member, slot, 1), 0u);
    CHECK_FALSE(QuickItemSlots::findItem(member, QuickItemSlot{}, 1).has_value());
}

TEST_CASE("a quick item slot uses the owner's bottle first, else another member's")
{
    // The slot draws on the whole party (user, 2026-10-25): the owner's bags first, then the others' in party order.
    OpenYAMM::Game::Party party;
    OpenYAMM::Game::Party::Snapshot snapshot = party.snapshot();
    snapshot.members.assign(3, Character{});
    snapshot.members[1].inventory = {bottle(222, 40, 0)};
    snapshot.members[2].inventory = {bottle(222, 10, 4), bottle(222, 20, 5)};
    party.restoreSnapshot(snapshot);
    QuickItemSlot slot;
    slot.itemId = 222;

    std::optional<QuickItemSlots::PartyLocation> found = QuickItemSlots::findPartyItem(party, 2, slot, 1);
    REQUIRE(found.has_value());
    CHECK_EQ(found->memberIndex, 2u);
    CHECK_EQ(found->location.gridX, 4);

    found = QuickItemSlots::findPartyItem(party, 0, slot, 1);
    REQUIRE(found.has_value());
    CHECK_EQ(found->memberIndex, 1u);
    CHECK_EQ(found->location.gridX, 0);

    CHECK_EQ(QuickItemSlots::countPartyItems(party, slot, 1), 3u);
    slot.itemId = 230;
    CHECK_FALSE(QuickItemSlots::findPartyItem(party, 0, slot, 1).has_value());
}

TEST_CASE("sorting every bag sends crafting and alchemy items to the bags tagged for them")
{
    using OpenYAMM::Game::BagTag;
    using OpenYAMM::Game::Party;
    using OpenYAMM::Game::PartySeed;
    namespace InventoryBags = OpenYAMM::Game::InventoryBags;

    const auto item = [](uint32_t itemId, uint8_t gridX)
    {
        InventoryItem result;
        result.objectDescriptionId = itemId;
        result.width = 1;
        result.height = 1;
        result.gridX = gridX;
        return result;
    };
    // 1050 a crafting material, 222 a reagent, 1 a sword (the rules' bagTag, by id here).
    const auto bagTagOf = [](const InventoryItem &stored) -> std::optional<BagTag>
    {
        return stored.objectDescriptionId == 1050 ? std::optional<BagTag>(BagTag::Crafting)
            : stored.objectDescriptionId == 222   ? std::optional<BagTag>(BagTag::Alchemy)
                                                  : std::nullopt;
    };
    const auto noRank = [](const InventoryItem &) -> std::optional<int>
    {
        return std::nullopt;
    };

    PartySeed seed;
    seed.members = {Character{}};
    Party party;
    party.seed(seed);
    Character &member = *party.member(0);
    member.inventory = {item(1050, 0), item(222, 1), item(1, 2)};
    InventoryBags::toggleTag(member, 2, BagTag::Alchemy);
    InventoryBags::toggleTag(member, 3, BagTag::Crafting);

    // The open bag alone keeps everything: it has nowhere else to put an item.
    REQUIRE(InventoryBags::sortPartyBags(party, {0}, false, 4, std::nullopt, noRank, bagTagOf));
    CHECK_EQ(InventoryBags::itemsInBag(member, 0).size(), 3u);

    REQUIRE(InventoryBags::sortPartyBags(party, {0}, true, 4, std::nullopt, noRank, bagTagOf));
    REQUIRE_EQ(InventoryBags::itemsInBag(member, 0).size(), 1u);
    CHECK_EQ(InventoryBags::itemsInBag(member, 0)[0].objectDescriptionId, 1u);
    REQUIRE_EQ(InventoryBags::itemsInBag(member, 2).size(), 1u);
    CHECK_EQ(InventoryBags::itemsInBag(member, 2)[0].objectDescriptionId, 222u);
    REQUIRE_EQ(InventoryBags::itemsInBag(member, 3).size(), 1u);
    CHECK_EQ(InventoryBags::itemsInBag(member, 3)[0].objectDescriptionId, 1050u);
}
