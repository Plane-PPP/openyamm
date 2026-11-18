#include "engine/AssetFileSystem.h"
#include "engine/AssetScaleTier.h"
#include "game/content/DungeonRun.h"
#include "game/content/RulesetDefinition.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

using OpenYAMM::Game::DungeonRunDefinition;
using OpenYAMM::Game::DungeonRunStop;

namespace
{
constexpr const char *RunYaml =
    "dungeons:\n"
    "  - map: 6d01.blv\n"
    "    entrance: {x: 601, y: 6871, z: 177, yaw: 1400}\n"
    "  - map: 6d02.blv\n"
    "    entrance: {x: 16406, y: -19669, z: 865}\n"
    "  - map: 6t3.blv\n"
    "    entrance: {x: 2817, y: -4748, z: -639, yaw: 512}\n";

std::string readSourceFile(const std::filesystem::path &relativePath)
{
    std::ifstream input(std::filesystem::path(OPENYAMM_SOURCE_DIR) / relativePath);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

DungeonRunDefinition installedRun()
{
    std::string error;
    const std::optional<DungeonRunDefinition> run = OpenYAMM::Game::parseDungeonRunDefinition(
        readSourceFile("assets_dev/engine/rulesets/roguelike/run.yml"), error);
    REQUIRE_MESSAGE(run.has_value(), error);
    return *run;
}
}

TEST_CASE("dungeon runs parse their dungeons and entrances")
{
    std::string error;
    const std::optional<DungeonRunDefinition> run = OpenYAMM::Game::parseDungeonRunDefinition(RunYaml, error);
    REQUIRE_MESSAGE(run.has_value(), error);
    REQUIRE_EQ(run->dungeons.size(), 3u);
    CHECK_EQ(run->dungeons[0].mapFileName, "6d01.blv");
    CHECK_EQ(run->dungeons[0].x, 601);
    CHECK_EQ(run->dungeons[0].y, 6871);
    CHECK_EQ(run->dungeons[0].z, 177);
    CHECK_EQ(run->dungeons[0].yawUnits, 1400);
    // The yaw is optional.
    CHECK_EQ(run->dungeons[1].y, -19669);
    CHECK_EQ(run->dungeons[1].yawUnits, 0);
    CHECK_EQ(run->dungeons[2].z, -639);
}

TEST_CASE("dungeon runs reject lists a run cannot play")
{
    std::string error;
    CHECK_FALSE(OpenYAMM::Game::parseDungeonRunDefinition("name: nothing\n", error).has_value());
    CHECK_FALSE(error.empty());
    CHECK_FALSE(OpenYAMM::Game::parseDungeonRunDefinition("dungeons: []\n", error).has_value());
    // No entrance: the original BLV maps have no party start to fall back on.
    CHECK_FALSE(OpenYAMM::Game::parseDungeonRunDefinition("dungeons:\n  - map: 6d01.blv\n", error).has_value());
    CHECK_FALSE(OpenYAMM::Game::parseDungeonRunDefinition(
        "dungeons:\n  - map: 6d01.blv\n    entrance: {x: 1, y: 2}\n", error).has_value());
    // A dungeon listed twice would repeat within a run.
    CHECK_FALSE(OpenYAMM::Game::parseDungeonRunDefinition(
        "dungeons:\n"
        "  - map: 6d01.blv\n    entrance: {x: 1, y: 2, z: 3}\n"
        "  - map: 6D01.BLV\n    entrance: {x: 1, y: 2, z: 3}\n",
        error).has_value());
    CHECK_FALSE(OpenYAMM::Game::parseDungeonRunDefinition("dungeons: [", error).has_value());
    CHECK_FALSE(error.empty());
}

TEST_CASE("a dungeon run visits every dungeon once, in an order fixed by the seed")
{
    for (const uint64_t seed : {uint64_t(0), uint64_t(1), uint64_t(0x5eed20261024ull), ~uint64_t(0)})
    {
        CAPTURE(seed);
        const std::vector<size_t> order = OpenYAMM::Game::dungeonRunOrder(8, seed);
        REQUIRE_EQ(order.size(), 8u);
        CHECK_EQ(std::set<size_t>(order.begin(), order.end()).size(), 8u);
        CHECK(*std::max_element(order.begin(), order.end()) == 7u);
        // A loaded save has the same seed and so the same order.
        CHECK(order == OpenYAMM::Game::dungeonRunOrder(8, seed));
    }

    // Different seeds give different orders (eight dungeons have 40320; a hundred seeds reach many of them).
    std::set<std::vector<size_t>> orders;

    for (uint64_t seed = 1; seed <= 100; ++seed)
    {
        orders.insert(OpenYAMM::Game::dungeonRunOrder(8, seed));
    }

    CHECK(orders.size() > 90);

    // Every dungeon comes first for some seed.
    std::set<size_t> firstDungeons;

    for (uint64_t seed = 1; seed <= 200; ++seed)
    {
        firstDungeons.insert(OpenYAMM::Game::dungeonRunOrder(8, seed).front());
    }

    CHECK_EQ(firstDungeons.size(), 8u);
    CHECK(OpenYAMM::Game::dungeonRunOrder(0, 7).empty());
    CHECK_EQ(OpenYAMM::Game::dungeonRunOrder(1, 7), std::vector<size_t>{0});
}

TEST_CASE("a dungeon run hands out its stops by the count of dungeons entered and then ends")
{
    std::string error;
    const std::optional<DungeonRunDefinition> run = OpenYAMM::Game::parseDungeonRunDefinition(RunYaml, error);
    REQUIRE_MESSAGE(run.has_value(), error);
    constexpr uint64_t Seed = 20261111;
    std::set<std::string> maps;

    for (size_t entered = 0; entered < run->dungeons.size(); ++entered)
    {
        const DungeonRunStop *pStop = OpenYAMM::Game::dungeonRunStop(*run, Seed, entered);
        REQUIRE(pStop != nullptr);
        maps.insert(pStop->mapFileName);
    }

    CHECK_EQ(maps.size(), run->dungeons.size());
    CHECK(OpenYAMM::Game::dungeonRunStop(*run, Seed, run->dungeons.size()) == nullptr);
    CHECK(OpenYAMM::Game::dungeonRunStop(*run, Seed, run->dungeons.size() + 5) == nullptr);
}

TEST_CASE("a dungeon run with a first dungeon always starts there and shuffles the others")
{
    std::string error;
    const std::optional<DungeonRunDefinition> run =
        OpenYAMM::Game::parseDungeonRunDefinition(std::string("first: 6D02.blv\n") + RunYaml, error);
    REQUIRE_MESSAGE(run.has_value(), error);
    REQUIRE(run->firstDungeon.has_value());
    CHECK_EQ(*run->firstDungeon, 1u);

    // Without the key any dungeon may come first.
    const std::optional<DungeonRunDefinition> freeRun = OpenYAMM::Game::parseDungeonRunDefinition(RunYaml, error);
    REQUIRE_MESSAGE(freeRun.has_value(), error);
    CHECK_FALSE(freeRun->firstDungeon.has_value());

    std::set<std::vector<size_t>> orders;
    std::set<size_t> freeFirstDungeons;

    for (uint64_t seed = 1; seed <= 100; ++seed)
    {
        CAPTURE(seed);
        const std::vector<size_t> order = OpenYAMM::Game::dungeonRunOrder(*run, seed);
        REQUIRE_EQ(order.size(), 3u);
        CHECK_EQ(order.front(), 1u);
        CHECK_EQ(std::set<size_t>(order.begin(), order.end()).size(), 3u);
        CHECK(order == OpenYAMM::Game::dungeonRunOrder(*run, seed));
        CHECK_EQ(OpenYAMM::Game::dungeonRunStop(*run, seed, 0)->mapFileName, "6d02.blv");
        orders.insert(order);
        freeFirstDungeons.insert(OpenYAMM::Game::dungeonRunOrder(*freeRun, seed).front());
    }

    // The two other dungeons come in both orders.
    CHECK_EQ(orders.size(), 2u);
    CHECK_EQ(freeFirstDungeons.size(), 3u);

    // The first dungeon must be one of the list.
    CHECK_FALSE(OpenYAMM::Game::parseDungeonRunDefinition(std::string("first: 6d99.blv\n") + RunYaml, error)
        .has_value());
    CHECK_FALSE(error.empty());
}

TEST_CASE("a dungeon run's end line counts the dungeons left behind")
{
    CHECK_EQ(OpenYAMM::Game::dungeonRunEndText(true, 0), "Run over. Dungeons cleared: 0.");
    CHECK_EQ(OpenYAMM::Game::dungeonRunEndText(true, 3), "Run over. Dungeons cleared: 3.");
    CHECK_EQ(OpenYAMM::Game::dungeonRunEndText(false, 8), "Run complete! Dungeons cleared: 8.");
}

TEST_CASE("only a ruleset package with a run.yml is a dungeon run")
{
    const std::filesystem::path source = OPENYAMM_SOURCE_DIR;
    OpenYAMM::Engine::AssetFileSystem assetFileSystem;
    REQUIRE(assetFileSystem.initialize(source, source / "assets_dev", OpenYAMM::Engine::AssetScaleTier::X1));
    std::string error;

    // Standard and MAW play as before.
    CHECK_FALSE(OpenYAMM::Game::loadDungeonRunDefinition(assetFileSystem, "mmerge", error).has_value());
    CHECK(error.empty());
    CHECK_FALSE(OpenYAMM::Game::loadDungeonRunDefinition(assetFileSystem, "maw", error).has_value());
    CHECK(error.empty());
    CHECK_FALSE(OpenYAMM::Game::loadDungeonRunDefinition(assetFileSystem, "", error).has_value());
    CHECK(error.empty());

    const std::optional<DungeonRunDefinition> run =
        OpenYAMM::Game::loadDungeonRunDefinition(assetFileSystem, "roguelike", error);
    REQUIRE_MESSAGE(run.has_value(), error);
    CHECK_EQ(run->dungeons.size(), installedRun().dungeons.size());

    // The package is an ordinary ruleset the new-game screen lists.
    std::vector<OpenYAMM::Game::RulesetDefinition> rulesets;
    REQUIRE_MESSAGE(OpenYAMM::Game::loadAvailableRulesets(assetFileSystem, rulesets, error), error);
    CHECK(std::any_of(rulesets.begin(), rulesets.end(),
        [](const OpenYAMM::Game::RulesetDefinition &ruleset) { return ruleset.id == "roguelike"; }));
}

TEST_CASE("the installed run's dungeons are MM6-MM9 maps entered where their worlds' events lead")
{
    const DungeonRunDefinition run = installedRun();
    // The pool of 2026-11-17 (docs/research/roguelike-dungeon-pool-mm6-mm9.md): every world, every stage.
    CHECK(run.dungeons.size() >= 40);
    REQUIRE(run.rules.has_value());
    CHECK(run.rules->route == std::vector<int>{1, 2, 3, 4});

    for (const int stage : run.rules->route)
    {
        CAPTURE(stage);
        CHECK(std::count_if(run.dungeons.begin(), run.dungeons.end(),
                  [stage](const DungeonRunStop &stop) { return OpenYAMM::Game::dungeonRunStopAtStage(stop, stage); })
            >= 3);
    }

    // Every evt.MoveToMap(x, y, z, yaw, ..., "map") of MM6-MM8, and every MM9 transition's arrival.
    const std::filesystem::path worldsRoot = std::filesystem::path(OPENYAMM_SOURCE_DIR) / "assets_dev/worlds";
    const std::regex moveToMap(
        R"re(evt\.MoveToMap\((-?\d+), (-?\d+), (-?\d+), (-?\d+),[^"\n]*"([^"]+)"\))re");
    std::set<std::tuple<std::string, int32_t, int32_t, int32_t, int32_t>> entrances;

    for (const char *pWorld : {"mm6", "mm7", "mm8"})
    {
        for (const std::filesystem::directory_entry &entry :
            std::filesystem::directory_iterator(worldsRoot / pWorld / "events/maps"))
        {
            if (entry.path().extension() != ".lua")
            {
                continue;
            }

            std::ifstream input(entry.path());
            const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};

            for (std::sregex_iterator match(text.begin(), text.end(), moveToMap); match != std::sregex_iterator();
                 ++match)
            {
                entrances.emplace(
                    (*match)[5].str(),
                    std::stoi((*match)[1].str()),
                    std::stoi((*match)[2].str()),
                    std::stoi((*match)[3].str()),
                    std::stoi((*match)[4].str()));
            }
        }
    }

    {
        // CanonicalId, SourceMap, SourceMapFile, ... DestinationMapFile (13), ArrivalX/Y/Z (16-18), FacingYawUnits (20).
        std::ifstream input(worldsRoot / "mm9/data_tables/map_transitions.txt");
        std::string line;
        std::getline(input, line);

        while (std::getline(input, line))
        {
            std::vector<std::string> columns;
            std::stringstream stream(line);
            std::string column;

            while (std::getline(stream, column, '\t'))
            {
                columns.push_back(column);
            }

            if (columns.size() > 20)
            {
                entrances.emplace(columns[13], std::stoi(columns[16]), std::stoi(columns[17]),
                    std::stoi(columns[18]), columns[20].empty() ? 0 : std::stoi(columns[20]));
            }
        }
    }

    REQUIRE_FALSE(entrances.empty());

    for (const DungeonRunStop &stop : run.dungeons)
    {
        CAPTURE(stop.mapFileName);
        const std::string stem = std::filesystem::path(stop.mapFileName).stem().string();
        // The map belongs to one world, which has its scene and its events.
        int worlds = 0;

        for (const char *pWorld : {"mm6", "mm7", "mm8", "mm9"})
        {
            if (std::filesystem::exists(worldsRoot / pWorld / "maps" / (stem + ".scene.yml")))
            {
                ++worlds;
                CHECK(std::filesystem::exists(worldsRoot / pWorld / "events/maps" / (stem + ".lua")));
            }
        }

        CHECK_EQ(worlds, 1);
        CHECK(entrances.contains({stop.mapFileName, stop.x, stop.y, stop.z, stop.yawUnits}));
    }
}

TEST_CASE("a run's route draws one dungeon of each stop's stage, never twice, and keeps pinned stops")
{
    const DungeonRunDefinition run = installedRun();
    REQUIRE(run.rules.has_value());
    std::set<std::string> firstDungeons;

    for (uint64_t seed = 1; seed <= 200; ++seed)
    {
        CAPTURE(seed);
        const std::vector<size_t> route = OpenYAMM::Game::drawDungeonRunRoute(run, seed);
        REQUIRE_EQ(route.size(), run.rules->route.size());
        CHECK_EQ(std::set<size_t>(route.begin(), route.end()).size(), route.size());

        for (size_t stop = 0; stop < route.size(); ++stop)
        {
            CHECK(OpenYAMM::Game::dungeonRunStopAtStage(run.dungeons[route[stop]], run.rules->route[stop]));
        }

        // The same seed, the same route.
        CHECK(OpenYAMM::Game::drawDungeonRunRoute(run, seed) == route);
        firstDungeons.insert(run.dungeons[route[0]].mapFileName);
    }

    // The first stop is no longer always Goblinwatch.
    CHECK(firstDungeons.size() >= 3);

    // Pinned stops (a scenario's `run new <seed> <map> ...`, a 2.4 save's route) keep their dungeons.
    const auto indexOf = [&run](const std::string &map)
    {
        return OpenYAMM::Game::dungeonRunIndexOfKey(run, OpenYAMM::Game::dungeonRunMapKey(map));
    };
    REQUIRE(indexOf("6d01.blv").has_value());
    REQUIRE(indexOf("6T3.BLV").has_value());
    const std::vector<size_t> pinned =
        OpenYAMM::Game::drawDungeonRunRoute(run, 4, {indexOf("6d01.blv"), indexOf("6t3.blv")});
    REQUIRE_EQ(pinned.size(), 4u);
    CHECK_EQ(run.dungeons[pinned[0]].mapFileName, "6d01.blv");
    CHECK_EQ(run.dungeons[pinned[1]].mapFileName, "6t3.blv");
    CHECK(OpenYAMM::Game::dungeonRunStopAtStage(run.dungeons[pinned[2]], 3));
    CHECK(OpenYAMM::Game::dungeonRunStopAtStage(run.dungeons[pinned[3]], 4));
    CHECK_EQ(std::set<size_t>(pinned.begin(), pinned.end()).size(), 4u);
}

TEST_CASE("the route of a 2.4 run is the one that version drew for the seed")
{
    // Seed 4's 2.4 route, as the frozen v1 save prints it (tests/fixtures/minotaur-2.4-v184-roguelike-v1.txt).
    CHECK(OpenYAMM::Game::legacyDungeonRunMaps(4)
        == std::vector<std::string>{"6d01.blv", "6t3.blv", "6d12.blv", "6d07.blv"});

    for (uint64_t seed = 1; seed <= 50; ++seed)
    {
        const std::vector<std::string> maps = OpenYAMM::Game::legacyDungeonRunMaps(seed);
        REQUIRE_EQ(maps.size(), 4u);
        // 2.4's one dungeon of tier 1.
        CHECK_EQ(maps[0], "6d01.blv");
    }

    // A map key is the same whatever the case of the name, and two maps differ.
    CHECK_EQ(OpenYAMM::Game::dungeonRunMapKey("6d01.blv"), OpenYAMM::Game::dungeonRunMapKey("6D01.BLV"));
    CHECK(OpenYAMM::Game::dungeonRunMapKey("6d01.blv") != OpenYAMM::Game::dungeonRunMapKey("6d02.blv"));
    CHECK(OpenYAMM::Game::dungeonRunMapKey("6d01.blv") >= 0);
}

TEST_CASE("run.yml stages: a map may stand at several, and a route stage needs its dungeons")
{
    std::string error;
    const std::string dungeons =
        "dungeons:\n"
        "  - {map: a.blv, stages: [1, 2], entrance: {x: 1, y: 2, z: 3}}\n"
        "  - {map: b.blv, tier: 2, entrance: {x: 1, y: 2, z: 3}}\n";
    const std::optional<DungeonRunDefinition> run =
        OpenYAMM::Game::parseDungeonRunDefinition(dungeons + "rules:\n  route: [1, 2]\n", error);
    REQUIRE_MESSAGE(run.has_value(), error);
    CHECK(run->dungeons[0].stages == std::vector<int>{1, 2});
    CHECK_EQ(run->dungeons[0].tier, 1);
    CHECK(run->dungeons[1].stages == std::vector<int>{2});
    CHECK(OpenYAMM::Game::dungeonRunStopAtStage(run->dungeons[0], 2));
    CHECK_FALSE(OpenYAMM::Game::dungeonRunStopAtStage(run->dungeons[1], 1));

    // Stage 3 has no dungeon; stage 2 twice has its two maps.
    CHECK_FALSE(OpenYAMM::Game::parseDungeonRunDefinition(dungeons + "rules:\n  route: [1, 3]\n", error).has_value());
    CHECK_FALSE(error.empty());
    CHECK(OpenYAMM::Game::parseDungeonRunDefinition(dungeons + "rules:\n  route: [2, 2]\n", error).has_value());
}
