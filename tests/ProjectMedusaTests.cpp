// Project Medusa (https://github.com/Plane-PPP/project-medusa): a ruleset package with its own new-game start and
// heroine. The package itself lives in the Project Medusa repository; development links it, and the Medusa Windows
// build copies it, to assets_dev/engine/rulesets/medusa. The tests of the installed package skip without it.
#include "engine/AssetFileSystem.h"
#include "engine/AssetScaleTier.h"
#include "engine/TextTable.h"
#include "game/content/ContentTableComposer.h"
#include "game/content/RulesetDefinition.h"
#include "game/maps/SaveGame.h"

#include <doctest/doctest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <vector>

using OpenYAMM::Game::RulesetDefinition;
using OpenYAMM::Game::RulesetStartCard;

namespace
{
using Rows = std::vector<std::vector<std::string>>;

const std::filesystem::path SourceRoot = OPENYAMM_SOURCE_DIR;
const std::filesystem::path MedusaPackage = "assets_dev/engine/rulesets/medusa";

std::string readText(const std::filesystem::path &path)
{
    std::ifstream input(SourceRoot / path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

Rows readTable(const std::filesystem::path &path)
{
    const std::optional<OpenYAMM::Engine::TextTable> table =
        OpenYAMM::Engine::TextTable::parseTabSeparated(readText(path));
    REQUIRE_MESSAGE(table.has_value(), path.string());
    Rows rows;
    for (size_t index = 0; index < table->getRowCount(); ++index)
    {
        rows.push_back(table->getRow(index));
    }
    return rows;
}

const std::vector<std::string> *rowWithKey(const Rows &rows, const std::string &key, size_t keyColumn = 0)
{
    for (const std::vector<std::string> &row : rows)
    {
        if (row.size() > keyColumn && row[keyColumn] == key)
        {
            return &row;
        }
    }
    return nullptr;
}

std::filesystem::path makeTemporaryRoot(const std::string &name)
{
    const uint64_t ticks = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() / ("openyamm_" + name + "_" + std::to_string(ticks));
}

void writeTextFile(const std::filesystem::path &path, const std::string &contents)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << contents;
}

std::optional<RulesetDefinition> parse(const std::string &yaml, std::string &error)
{
    return OpenYAMM::Game::parseRulesetDefinition(yaml, "medusa", error);
}

bool medusaInstalled()
{
    return std::filesystem::exists(SourceRoot / MedusaPackage / "ruleset.yml");
}

// Width and height from a PNG header.
std::optional<std::pair<uint32_t, uint32_t>> pngSize(const std::filesystem::path &path)
{
    std::ifstream input(path, std::ios::binary);
    std::vector<unsigned char> header(24);
    if (!input.read(reinterpret_cast<char *>(header.data()), std::streamsize(header.size()))
        || header[1] != 'P' || header[2] != 'N' || header[3] != 'G')
    {
        return std::nullopt;
    }
    const auto big = [&header](size_t offset)
    {
        return uint32_t(header[offset]) << 24 | uint32_t(header[offset + 1]) << 16 | uint32_t(header[offset + 2]) << 8
            | uint32_t(header[offset + 3]);
    };
    return std::make_pair(big(16), big(20));
}
}

TEST_CASE("ruleset start cards parse from newGame.starts")
{
    std::string error;
    const std::optional<RulesetDefinition> plain = parse("id: medusa\nname: Project Medusa\n", error);
    REQUIRE_MESSAGE(plain.has_value(), error);
    CHECK(plain->startCards.empty());
    CHECK_FALSE(plain->hasRulesChoices());

    const std::optional<RulesetDefinition> medusa = parse(
        "id: medusa\nname: Project Medusa\n"
        "newGame:\n"
        "  starts:\n"
        "    - {continent: antagarich, title: Mount Nighon, subtitle: Prologue, start: Begin in Mount Nighon,"
        " illustration: out10}\n"
        "    - {continent: jadame, title: Second}\n",
        error);
    REQUIRE_MESSAGE(medusa.has_value(), error);
    REQUIRE_EQ(medusa->startCards.size(), 2u);
    const RulesetStartCard &first = medusa->startCards.front();
    CHECK_EQ(first.continentKey, "antagarich");
    CHECK_EQ(first.title, "Mount Nighon");
    CHECK_EQ(first.subtitle, "Prologue");
    CHECK_EQ(first.startText, "Begin in Mount Nighon");
    CHECK_EQ(first.illustration, "out10");
    CHECK(medusa->startCards.back().subtitle.empty());
    CHECK(medusa->startCards.back().illustration.empty());
}

TEST_CASE("ruleset start cards reject what the continent screen cannot show")
{
    const auto rejects = [](const std::string &newGame)
    {
        std::string error;
        const bool parsed = parse("id: medusa\nname: Project Medusa\n" + newGame, error).has_value();
        CHECK_FALSE(error.empty());
        return !parsed;
    };
    CHECK(rejects("newGame: [antagarich]\n"));
    CHECK(rejects("newGame:\n  starts: []\n"));
    CHECK(rejects("newGame:\n  starts:\n    - {title: No continent}\n"));
    CHECK(rejects("newGame:\n  starts:\n    - {continent: antagarich}\n"));
    CHECK(rejects("newGame:\n  starts:\n    - {continent: a, title: A}\n    - {continent: a, title: B}\n"));
    CHECK(rejects("newGame:\n  starts:\n    - {continent: a, title: A}\n    - {continent: b, title: B}\n"
                  "    - {continent: c, title: C}\n    - {continent: d, title: D}\n"));
}

TEST_CASE("rulesets with options keep their Rules panel")
{
    std::string error;
    const std::optional<RulesetDefinition> withOptions =
        parse("id: medusa\nname: Project Medusa\noptions:\n  preferences: true\n", error);
    REQUIRE_MESSAGE(withOptions.has_value(), error);
    CHECK(withOptions->hasRulesChoices());
    CHECK_FALSE(OpenYAMM::Game::standardRulesetDefinition().hasRulesChoices());
}

TEST_CASE("a broken ruleset package leaves the valid ones listed")
{
    const std::filesystem::path temporaryRoot = makeTemporaryRoot("broken_ruleset");
    const std::filesystem::path assetRoot = temporaryRoot / "assets_dev";
    writeTextFile(assetRoot / "engine/rulesets/aaa/ruleset.yml", "id: aaa\nname: Broken\nnewGame:\n  starts: []\n");
    writeTextFile(assetRoot / "engine/rulesets/maw/ruleset.yml", "id: maw\nname: MAW\n");
    writeTextFile(assetRoot / "engine/rulesets/medusa/ruleset.yml", "id: medusa\nname: Project Medusa\n");
    {
        OpenYAMM::Engine::AssetFileSystem assetFileSystem;
        REQUIRE(assetFileSystem.initialize(temporaryRoot, assetRoot, OpenYAMM::Engine::AssetScaleTier::X1, "mm8"));
        std::vector<RulesetDefinition> rulesets;
        std::string error;
        CHECK_FALSE(OpenYAMM::Game::loadAvailableRulesets(assetFileSystem, rulesets, error));
        CHECK(error.find("rulesets/aaa/") != std::string::npos);
        REQUIRE_EQ(rulesets.size(), 3u);
        CHECK(rulesets[0].isStandard());
        // Tabs follow id order: Standard MMMerge | MAW | Project Medusa.
        CHECK_EQ(rulesets[1].id, "maw");
        CHECK_EQ(rulesets[2].id, "medusa");
    }
    std::filesystem::remove_all(temporaryRoot);
}

TEST_CASE("saves keep the Project Medusa ruleset and its heroine's portrait")
{
    OpenYAMM::Game::GameSaveData save;
    save.mapFileName = "out10.odm";
    save.saveName = "medusa check";
    save.ruleset.id = "medusa";
    OpenYAMM::Game::Character heroine;
    heroine.name = "Medusa";
    heroine.characterDataId = 79;
    heroine.portraitPictureId = 78;
    heroine.portraitTextureName = "med01-01";
    heroine.voiceId = 1;
    save.party.members.push_back(heroine);

    const std::filesystem::path path = makeTemporaryRoot("medusa_save").replace_extension(".oysav");
    std::string error;
    REQUIRE_MESSAGE(OpenYAMM::Game::saveGameDataToPath(path, save, error), error);
    const std::optional<OpenYAMM::Game::GameSaveData> restored = OpenYAMM::Game::loadGameDataFromPath(path, error);
    std::filesystem::remove(path);
    REQUIRE_MESSAGE(restored.has_value(), error);
    CHECK_EQ(restored->ruleset.id, "medusa");
    CHECK_EQ(restored->mapFileName, "out10.odm");
    REQUIRE_EQ(restored->party.members.size(), 1u);
    CHECK_EQ(restored->party.members[0].characterDataId, 79u);
    CHECK_EQ(restored->party.members[0].portraitPictureId, 78u);
    CHECK_EQ(restored->party.members[0].portraitTextureName, "med01-01");
}

TEST_CASE("Project Medusa: the installed package starts in Mount Nighon with its heroine")
{
    if (!medusaInstalled())
    {
        MESSAGE("Project Medusa package not installed in assets_dev/engine/rulesets/medusa; skipped");
        return;
    }

    std::string error;
    const std::optional<RulesetDefinition> medusa = parse(readText(MedusaPackage / "ruleset.yml"), error);
    REQUIRE_MESSAGE(medusa.has_value(), error);
    CHECK_EQ(medusa->name, "Project Medusa");
    // Standard rules: no MAW options, so no Rules panel.
    CHECK_FALSE(medusa->hasRulesChoices());
    REQUIRE_EQ(medusa->startCards.size(), 1u);
    const RulesetStartCard &card = medusa->startCards.front();
    CHECK_EQ(card.title, "Mount Nighon");
    // The card opens the engine's own Antagarich party creation (character_selection.yml is not replaced).
    CHECK_EQ(card.continentKey, "antagarich");
    CHECK(medusa->findTables("engine/data_tables/character_selection.yml").empty());
    CHECK(readText("assets_dev/engine/data_tables/character_selection.yml").find("key: antagarich")
          != std::string::npos);

    std::map<std::string, Rows> composed;
    for (const OpenYAMM::Game::RulesetTableContribution &table : medusa->tables)
    {
        CAPTURE(table.table);
        REQUIRE(table.mode == OpenYAMM::Game::TableContributionMode::Override);
        if (!composed.contains(table.table))
        {
            composed[table.table] = readTable(std::filesystem::path("assets_dev/engine/data_tables") / table.table);
        }
        Rows &rows = composed[table.table];
        REQUIRE_MESSAGE(OpenYAMM::Game::applyTableCellOverrides(rows, readTable(MedusaPackage / table.path),
                                                                table.keyColumn, table.headerRow, error),
                        error);
        if (!table.appendPath.empty())
        {
            REQUIRE_MESSAGE(OpenYAMM::Game::appendTableRows(rows, readTable(MedusaPackage / table.appendPath),
                                                            table.keyColumn, table.headerRow, error),
                            error);
        }
    }

    // Antagarich (row 2) starts, and a defeated party wakes, at the original Town Portal landing in Mount Nighon.
    const std::vector<std::string> *pAntagarich = rowWithKey(composed["continent_settings.txt"], "2");
    REQUIRE(pAntagarich != nullptr);
    REQUIRE_GE(pAntagarich->size(), 21u);
    const Rows townPortals = readTable("assets_dev/engine/data_tables/town_portal_switch.txt");
    const std::vector<std::string> *pNighonPortal = nullptr;
    for (const std::vector<std::string> &row : townPortals)
    {
        if (row.size() > 13 && row[13] == "Nighon")
        {
            pNighonPortal = &row;
        }
    }
    REQUIRE(pNighonPortal != nullptr);
    // The table outlives the row pointer into it (a temporary here left it dangling: Windows overwrote the rows).
    const Rows maps = readTable("assets_dev/engine/data_tables/map_stats.txt");
    const std::vector<std::string> *pMountNighon = rowWithKey(maps, (*pNighonPortal)[1]);
    REQUIRE(pMountNighon != nullptr);
    CHECK_EQ(pMountNighon->at(1), "Mount Nighon");
    CHECK_EQ(pMountNighon->at(2), "out10.odm");
    CHECK_EQ(pMountNighon->back(), "mm7");
    for (size_t first : {size_t(11), size_t(16)})
    {
        CHECK_EQ(pAntagarich->at(first), "out10.odm");
        CHECK_EQ(pAntagarich->at(first + 1), (*pNighonPortal)[2]);
        CHECK_EQ(pAntagarich->at(first + 2), (*pNighonPortal)[3]);
        CHECK_EQ(pAntagarich->at(first + 3), (*pNighonPortal)[4]);
        CHECK_EQ(pAntagarich->at(first + 4), (*pNighonPortal)[5]);
    }
    // The other continents keep their starts.
    CHECK_EQ(rowWithKey(composed["continent_settings.txt"], "1")->at(11), "out01.odm");
    CHECK_EQ(rowWithKey(composed["continent_settings.txt"], "3")->at(11), "oute3.odm");

    // The heroine: portrait 79, a human woman on the existing female doll, offered at the start.
    const Rows &characters = composed["character_data.txt"];
    const std::vector<std::string> *pHeroine = rowWithKey(characters, "79");
    const std::vector<std::string> *pDoll = rowWithKey(characters, "2");
    REQUIRE(pHeroine != nullptr);
    REQUIRE(pDoll != nullptr);
    CHECK_EQ(pHeroine->at(1), "1");
    CHECK_EQ(pHeroine->at(4), "1");
    CHECK_EQ(pHeroine->at(5), "x");
    CHECK_EQ(pHeroine->at(19), "med01-");
    CHECK_EQ(pHeroine->at(23), "0");
    for (size_t column = 6; column <= 18; ++column)
    {
        CAPTURE(column);
        CHECK_EQ(pHeroine->at(column), pDoll->at(column));
    }
    CHECK(rowWithKey(readTable("assets_dev/engine/data_tables/character_data.txt"), "79") == nullptr);

    // All 56 face frames, at the restored size, in the package's own icons.
    for (int frame = 1; frame <= 56; ++frame)
    {
        const std::string name = std::string("med01-") + (frame < 10 ? "0" : "") + std::to_string(frame) + ".png";
        CAPTURE(name);
        const std::optional<std::pair<uint32_t, uint32_t>> size = pngSize(SourceRoot / MedusaPackage / "icons_x2" / name);
        REQUIRE(size.has_value());
        CHECK_EQ(size->first, 118u);
        CHECK_EQ(size->second, 158u);
    }
}
