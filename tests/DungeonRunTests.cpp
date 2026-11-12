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

TEST_CASE("the installed run's dungeons are MM6 maps entered where their outdoor events lead")
{
    const DungeonRunDefinition run = installedRun();
    // The skeleton's pool: five to eight dungeons.
    CHECK(run.dungeons.size() >= 5);
    CHECK(run.dungeons.size() <= 8);

    // Every evt.MoveToMap(x, y, z, yaw, ..., "map") of MM6's outdoor maps.
    const std::filesystem::path mapsRoot = std::filesystem::path(OPENYAMM_SOURCE_DIR) / "assets_dev/worlds/mm6";
    const std::regex moveToMap(R"re(evt\.MoveToMap\((-?\d+), (-?\d+), (-?\d+), (-?\d+),[^"\n]*"([^"]+)"\))re");
    std::set<std::tuple<std::string, int32_t, int32_t, int32_t, int32_t>> entrances;

    for (const std::filesystem::directory_entry &entry :
        std::filesystem::directory_iterator(mapsRoot / "events/maps"))
    {
        const std::string name = entry.path().filename().string();

        if (!name.starts_with("out") || entry.path().extension() != ".lua")
        {
            continue;
        }

        std::ifstream input(entry.path());
        const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};

        for (std::sregex_iterator match(text.begin(), text.end(), moveToMap); match != std::sregex_iterator(); ++match)
        {
            entrances.emplace(
                (*match)[5].str(),
                std::stoi((*match)[1].str()),
                std::stoi((*match)[2].str()),
                std::stoi((*match)[3].str()),
                std::stoi((*match)[4].str()));
        }
    }

    REQUIRE_FALSE(entrances.empty());

    for (const DungeonRunStop &stop : run.dungeons)
    {
        CAPTURE(stop.mapFileName);
        const std::string stem = std::filesystem::path(stop.mapFileName).stem().string();
        CHECK(std::filesystem::exists(mapsRoot / "maps" / (stem + ".scene.yml")));
        CHECK(std::filesystem::exists(mapsRoot / "events/maps" / (stem + ".lua")));
        CHECK(entrances.contains({stop.mapFileName, stop.x, stop.y, stop.z, stop.yawUnits}));
    }
}
