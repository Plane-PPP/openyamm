#include "game/party/CustomPortraits.h"
#include "game/tables/CharacterDollTable.h"

#include <doctest/doctest.h>

#include <SDL3/SDL.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace
{
using namespace OpenYAMM::Game;

std::filesystem::path makePortraitTestRoot()
{
    const std::filesystem::path root = std::filesystem::temp_directory_path()
        / ("openyamm_custom_portraits_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    return root;
}

// A 118x158 frame: transparent with an opaque oval, so the image has the alpha a portrait has.
bool writeFrame(const std::filesystem::path &path, int width, int height, SDL_PixelFormat format)
{
    SDL_Surface *pSurface = SDL_CreateSurface(width, height, format);

    if (pSurface == nullptr)
    {
        return false;
    }

    SDL_FillSurfaceRect(pSurface, nullptr, SDL_MapSurfaceRGBA(pSurface, 0, 0, 0, 0));
    const SDL_Rect face = {width / 4, height / 4, width / 2, height / 2};
    SDL_FillSurfaceRect(pSurface, &face, SDL_MapSurfaceRGBA(pSurface, 180, 196, 162, 255));
    const bool saved = SDL_SavePNG(pSurface, path.string().c_str());
    SDL_DestroySurface(pSurface);
    return saved;
}

std::string frameName(size_t frame)
{
    char buffer[8] = {};
    std::snprintf(buffer, sizeof(buffer), "%02zu.png", frame);
    return buffer;
}

void writePackage(const std::filesystem::path &folder, const std::string &manifest)
{
    std::filesystem::create_directories(folder);
    std::ofstream(folder / "portrait.yml") << manifest;

    for (size_t frame = 1; frame <= CustomPortraitFrameCount; ++frame)
    {
        REQUIRE(writeFrame(folder / frameName(frame), CustomPortraitFrameWidth, CustomPortraitFrameHeight,
                           SDL_PIXELFORMAT_RGBA32));
    }
}

std::vector<std::string> characterRow(const std::string &id, const std::string &raceId)
{
    std::vector<std::string> row(24);
    row[0] = id;
    row[1] = "1";
    row[3] = "1";
    row[5] = "x";
    row[23] = raceId;
    return row;
}

CharacterDollTable makeDolls()
{
    CharacterDollTable dolls;
    // Row 2 is a playable portrait; row 3 belongs to no race (an NPC-only face).
    REQUIRE(dolls.loadCharacterRows({{}, characterRow("2", "0"), characterRow("3", "-1")}));
    return dolls;
}

std::string manifest(const std::string &id, const std::string &baseCharacter = "2", const std::string &name = "Medusa")
{
    return "schema: 1\nid: " + id + "\nname: " + name + "\nbase_character: " + baseCharacter + "\n";
}

bool portraitsFolderEmpty(const std::filesystem::path &portraitsFolder)
{
    std::error_code error;

    if (!std::filesystem::exists(portraitsFolder, error))
    {
        return true;
    }

    for (const std::filesystem::directory_entry &entry : std::filesystem::recursive_directory_iterator(portraitsFolder))
    {
        if (entry.is_regular_file())
        {
            return false;
        }
    }

    return true;
}
}

TEST_CASE("Custom portraits: names, ids and the texture prefix")
{
    CHECK(validCustomPortraitId("medusa"));
    CHECK(validCustomPortraitId("half_medusa_2"));
    CHECK_FALSE(validCustomPortraitId(""));
    CHECK_FALSE(validCustomPortraitId("Medusa"));
    CHECK_FALSE(validCustomPortraitId("../medusa"));
    CHECK_FALSE(validCustomPortraitId("me dusa"));
    CHECK_FALSE(validCustomPortraitId(std::string(33, 'a')));
    CHECK_EQ(customPortraitTexturePrefix("medusa"), "user_medusa-");
    CHECK(isCustomPortraitTextureName("user_medusa-01"));
    CHECK(isCustomPortraitTextureName("USER_medusa-01"));
    CHECK_FALSE(isCustomPortraitTextureName("pc02-01"));
    CHECK_FALSE(isCustomPortraitTextureName("user_"));
}

TEST_CASE("Custom portraits: a complete package imports, lists and can be imported again")
{
    const std::filesystem::path root = makePortraitTestRoot();
    const std::filesystem::path package = root / "medusa package";
    const std::filesystem::path portraits = root / "user" / "portraits";
    const CharacterDollTable dolls = makeDolls();
    writePackage(package, manifest("medusa"));

    const CustomPortraitImportResult first = importCustomPortrait(package, portraits, dolls);
    REQUIRE_MESSAGE(first.success, first.error);
    CHECK_FALSE(first.replaced);
    CHECK_EQ(first.portrait.id, "medusa");
    CHECK_EQ(first.portrait.name, "Medusa");
    CHECK_EQ(first.portrait.baseCharacterId, 2u);

    for (size_t frame = 1; frame <= CustomPortraitFrameCount; ++frame)
    {
        CAPTURE(frame);
        CHECK(std::filesystem::is_regular_file(customPortraitIconDirectory(portraits)
                                               / ("user_medusa-" + frameName(frame))));
    }

    std::vector<CustomPortrait> listed = loadCustomPortraits(portraits);
    REQUIRE_EQ(listed.size(), 1u);
    CHECK_EQ(listed[0].id, "medusa");
    CHECK_EQ(listed[0].baseCharacterId, 2u);

    // The same id again replaces it: still one portrait, with the new name.
    std::ofstream(package / "portrait.yml") << manifest("medusa", "2", "Medusa v2");
    const CustomPortraitImportResult second = importCustomPortrait(package, portraits, dolls);
    REQUIRE_MESSAGE(second.success, second.error);
    CHECK(second.replaced);
    listed = loadCustomPortraits(portraits);
    REQUIRE_EQ(listed.size(), 1u);
    CHECK_EQ(listed[0].name, "Medusa v2");

    // No temporary file is left behind.
    for (const auto &entry : std::filesystem::recursive_directory_iterator(portraits))
    {
        CHECK(entry.path().extension() != ".import");
    }

    std::filesystem::remove_all(root);
}

TEST_CASE("Custom portraits: a broken package is refused with its reason and writes nothing")
{
    const std::filesystem::path root = makePortraitTestRoot();
    const std::filesystem::path portraits = root / "portraits";
    const CharacterDollTable dolls = makeDolls();

    const auto refused = [&](const std::filesystem::path &package, const std::string &expectedText)
    {
        const CustomPortraitImportResult result = importCustomPortrait(package, portraits, dolls);
        CHECK_FALSE(result.success);
        CHECK_MESSAGE(result.error.find(expectedText) != std::string::npos, result.error);
        CHECK(portraitsFolderEmpty(portraits));
    };

    SUBCASE("no manifest")
    {
        const std::filesystem::path package = root / "empty";
        std::filesystem::create_directories(package);
        refused(package, "portrait.yml");
    }

    SUBCASE("a path in the id")
    {
        const std::filesystem::path package = root / "escape";
        writePackage(package, manifest("../../evil"));
        refused(package, "id");
    }

    SUBCASE("an unknown base character")
    {
        const std::filesystem::path package = root / "unknown";
        writePackage(package, manifest("medusa", "99"));
        refused(package, "base_character 99");
    }

    SUBCASE("a base character of no race")
    {
        const std::filesystem::path package = root / "npc";
        writePackage(package, manifest("medusa", "3"));
        refused(package, "base_character 3");
    }

    SUBCASE("a missing frame")
    {
        const std::filesystem::path package = root / "missing";
        writePackage(package, manifest("medusa"));
        std::filesystem::remove(package / "37.png");
        refused(package, "37.png");
    }

    SUBCASE("a frame of the wrong size")
    {
        const std::filesystem::path package = root / "small";
        writePackage(package, manifest("medusa"));
        REQUIRE(writeFrame(package / "12.png", 59, 79, SDL_PIXELFORMAT_RGBA32));
        refused(package, "12.png is 59x79");
    }

    SUBCASE("a frame without transparency")
    {
        const std::filesystem::path package = root / "opaque";
        writePackage(package, manifest("medusa"));
        // SDL saves every surface as RGBA, so mark the header as RGB (colour type 2) the way an editor would write
        // an opaque image; the format check comes before decoding.
        std::fstream frame(package / "05.png", std::ios::binary | std::ios::in | std::ios::out);
        frame.seekp(25);
        frame.put(char(2));
        frame.close();
        refused(package, "05.png must be an 8-bit RGBA PNG");
    }

    SUBCASE("a damaged frame")
    {
        const std::filesystem::path package = root / "damaged";
        writePackage(package, manifest("medusa"));
        std::ifstream input(package / "56.png", std::ios::binary);
        std::string bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        input.close();
        std::ofstream(package / "56.png", std::ios::binary | std::ios::trunc) << bytes.substr(0, 60);
        refused(package, "56.png is damaged");
    }

    SUBCASE("not YAML")
    {
        const std::filesystem::path package = root / "yaml";
        writePackage(package, "schema: [1\n");
        refused(package, "portrait.yml");
    }

    std::filesystem::remove_all(root);
}

TEST_CASE("Custom portraits: an installed portrait that lost frames or its name is not offered")
{
    const std::filesystem::path root = makePortraitTestRoot();
    const std::filesystem::path package = root / "package";
    const std::filesystem::path portraits = root / "portraits";
    const CharacterDollTable dolls = makeDolls();
    writePackage(package, manifest("medusa"));
    REQUIRE(importCustomPortrait(package, portraits, dolls).success);
    REQUIRE_EQ(loadCustomPortraits(portraits).size(), 1u);

    std::filesystem::remove(customPortraitIconDirectory(portraits) / "user_medusa-20.png");
    CHECK(loadCustomPortraits(portraits).empty());

    REQUIRE(importCustomPortrait(package, portraits, dolls).success);
    std::filesystem::rename(portraits / "medusa.yml", portraits / "gorgon.yml");
    CHECK(loadCustomPortraits(portraits).empty());

    std::filesystem::remove_all(root);
}
