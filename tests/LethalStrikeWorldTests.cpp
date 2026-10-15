#include <doctest/doctest.h>

#include "game/indoor/IndoorWorldRuntime.h"
#include "game/outdoor/OutdoorWorldRuntime.h"
#include "game/gameplay/GameplayActorService.h"
#include "game/party/Party.h"
#include "game/tables/ChestTable.h"
#include "game/tables/MonsterProjectileTable.h"
#include "game/tables/ObjectTable.h"
#include "game/tables/SpellTable.h"
#include "game/tables/ItemTable.h"
#include "game/items/ItemEnchantTables.h"
#include "game/items/ItemRuntime.h"
#include "engine/TextTable.h"
#include "tests/RegressionGameData.h"

#include <fstream>
#include <iterator>

using namespace OpenYAMM::Game;

namespace
{
struct Fixture
{
    MonsterTable monsters;
    MonsterProjectileTable projectiles;
    ObjectTable objects;
    SpellTable spells;
    ItemTable items;
    ChestTable chests;
    StandardItemEnchantTable standard;
    SpecialItemEnchantTable special;
    Party party;
    GameplayActorService actors;
    MapStatsEntry map;
    std::optional<MapDeltaData> delta = MapDeltaData{};
    std::optional<EventRuntimeState> events = EventRuntimeState{};
    IndoorMapData indoor;

    Fixture()
    {
        std::vector<std::string> row(38);
        row[0] = "1"; row[1] = "Target"; row[2] = "Target A";
        row[3] = "10"; row[4] = "100"; row[5] = "0"; row[6] = "100";
        row[7] = "0"; row[10] = "Stationary"; row[11] = "Normal";
        row[13] = "0"; row[14] = "100"; row[17] = "Phys"; row[18] = "1d6";
        REQUIRE(monsters.loadStatsFromRows({row}));
        REQUIRE(OpenYAMM::Tests::regressionGameDataLoaded());
        items = OpenYAMM::Tests::regressionGameData().itemTable;
        std::ifstream input(std::filesystem::path(OPENYAMM_SOURCE_DIR) / "assets_dev/engine/data_tables/object_list.txt");
        REQUIRE(input.good());
        const std::string objectText{std::istreambuf_iterator<char>(input), {}};
        const auto parsed = OpenYAMM::Engine::TextTable::parseTabSeparated(objectText);
        REQUIRE(parsed);
        std::vector<std::vector<std::string>> objectRows;
        for (size_t i = 0; i < parsed->getRowCount(); ++i) objectRows.push_back(parsed->getRow(i));
        REQUIRE(objects.loadRows(objectRows));
        REQUIRE(monsters.loadDeathDropsFromRows({
            {"MonsterID", "MonsterName", "ItemID", "ItemName", "ChancePercent", "Description"},
            {"1", "Target", "201", "Wolf's Eye", "100", "Deterministic world drop"}}));

        map.id = 1;
        map.fileName = "out01.odm";
        map.worldId = "mm8";
        PartySeed seed;
        Character c;
        c.name = "Tester";
        c.className = "Minotaur";
        c.health = c.maxHealth = 100;
        c.might = c.intellect = c.endurance = c.personality = c.luck = c.accuracy = c.speed = 14;
        seed.members.push_back(c);
        party.seed(seed);
        MapDeltaActor actor;
        actor.monsterInfoId = 1;
        actor.monsterId = 1;
        actor.hp = 100;
        actor.height = 160;
        actor.radius = 40;
        delta->actors.push_back(actor);
    }
};
}

TEST_CASE("Lethal Strike real outdoor terminal transition grants XP and hooks once")
{
    Fixture f;
    OutdoorWorldRuntime world;
    world.initialize(f.map, f.monsters, f.projectiles, f.objects, f.spells, f.items, &f.party, nullptr,
        f.standard, f.special, &f.chests, std::nullopt, f.delta, std::nullopt, f.events);
    world.setMonsterKilledHooksEnabled(true);
    REQUIRE(world.partyAttackActorFacts(0, false));
    REQUIRE(world.partyAttackActorFacts(0, false)->currentHp > 0);
    REQUIRE(world.debugSetActorHealth(0, 59));
    const uint64_t before = f.party.member(0)->experience;
    REQUIRE(world.killPartyAttackActor(0, {}));
    CHECK(world.partyAttackActorFacts(0, false)->currentHp == 0);
    CHECK(f.party.member(0)->experience > before);
    const uint64_t after = f.party.member(0)->experience;
    const auto snapshot = world.snapshot();
    REQUIRE(snapshot.mapActors.size() == 1);
    CHECK(snapshot.mapActors[0].aiState == OutdoorWorldRuntime::ActorAiState::Dying);
    CHECK(world.drainMonsterKilledEvents().size() == 1);
    const auto feedback = world.drainCombatFeedbackEvents();
    REQUIRE(feedback.size() == 1);
    CHECK(feedback[0].damage == 0);
    CHECK(feedback[0].killed);
    CHECK(feedback[0].experience > 0);
    CHECK_FALSE(world.killPartyAttackActor(0, {}));
    CHECK(f.party.member(0)->experience == after);
    CHECK(world.drainMonsterKilledEvents().empty());
    CHECK(world.drainCombatFeedbackEvents().empty());
    REQUIRE(snapshot.worldItems.size() == 1);
    CHECK(world.snapshot().worldItems.size() == snapshot.worldItems.size());
}

TEST_CASE("Lethal Strike real indoor terminal transition grants XP once and keeps corpse state")
{
    Fixture f;
    f.map.fileName = "d05.blv";
    IndoorWorldRuntime world;
    world.initialize(f.map, f.monsters, f.objects, f.items, f.chests, &f.party, nullptr,
        &f.delta, &f.events, &f.actors, nullptr, &f.indoor);
    REQUIRE(world.partyAttackActorFacts(0, false));
    REQUIRE(world.debugSetActorHealth(0, 59));
    const uint64_t before = f.party.member(0)->experience;
    REQUIRE(world.killPartyAttackActor(0, {}));
    CHECK(f.delta->actors[0].hp == 0);
    CHECK(f.party.member(0)->experience > before);
    const uint64_t after = f.party.member(0)->experience;
    const auto drops = f.delta->spriteObjects.size();
    const auto feedback = world.drainCombatFeedbackEvents();
    REQUIRE(feedback.size() == 1);
    CHECK(feedback[0].killed);
    CHECK(feedback[0].damage == 0);
    CHECK(feedback[0].experience > 0);
    CHECK_FALSE(world.killPartyAttackActor(0, {}));
    CHECK(f.party.member(0)->experience == after);
    CHECK(world.drainCombatFeedbackEvents().empty());
    REQUIRE(drops == 1);
    CHECK(f.delta->spriteObjects.size() == drops);
}
