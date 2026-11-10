#include "game/party/CustomPortraits.h"

#include "engine/ImageAssetLoader.h"
#include "engine/Localization.h"
#include "game/tables/CharacterDollTable.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <system_error>

namespace OpenYAMM::Game
{
namespace
{
constexpr int ManifestSchema = 1;
constexpr size_t MaximumIdLength = 32;
constexpr size_t MaximumNameLength = 48;
constexpr uintmax_t MaximumManifestBytes = 64 * 1024;
// A 118x158 RGBA PNG is at most about 75 KB uncompressed; anything far larger is not a portrait frame.
constexpr uintmax_t MaximumFrameBytes = 1024 * 1024;
constexpr std::string_view ImportSuffix = ".import";
constexpr std::array<uint8_t, 8> PngSignature = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};

std::string frameFileName(size_t frame)
{
    char buffer[8] = {};
    std::snprintf(buffer, sizeof(buffer), "%02zu.png", frame);
    return buffer;
}

std::string installedFrameFileName(std::string_view id, size_t frame)
{
    char buffer[8] = {};
    std::snprintf(buffer, sizeof(buffer), "%02zu", frame);
    return customPortraitTexturePrefix(id) + buffer + ".png";
}

// A regular file, not a link, below the size limit.
std::optional<std::vector<uint8_t>> readPackageFile(const std::filesystem::path &path, uintmax_t maximumBytes)
{
    std::error_code error;
    const std::filesystem::file_status status = std::filesystem::symlink_status(path, error);

    if (error || !std::filesystem::is_regular_file(status))
    {
        return std::nullopt;
    }

    const uintmax_t size = std::filesystem::file_size(path, error);

    if (error || size == 0 || size > maximumBytes)
    {
        return std::nullopt;
    }

    std::ifstream input(path, std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    return input.good() || input.eof() ? std::optional<std::vector<uint8_t>>(std::move(bytes)) : std::nullopt;
}

uint32_t readBigEndian32(const std::vector<uint8_t> &bytes, size_t offset)
{
    return (uint32_t(bytes[offset]) << 24) | (uint32_t(bytes[offset + 1]) << 16) | (uint32_t(bytes[offset + 2]) << 8)
        | uint32_t(bytes[offset + 3]);
}

// Empty when the bytes are a frame the HUD can show: a 118x158, 8-bit RGBA PNG that decodes.
std::string checkFrame(const std::vector<uint8_t> &bytes, const std::string &fileName)
{
    const bool png = bytes.size() > 33 && std::equal(PngSignature.begin(), PngSignature.end(), bytes.begin())
        && std::equal(bytes.begin() + 12, bytes.begin() + 16, std::string_view("IHDR").begin());

    if (!png)
    {
        return Engine::tr("{file} is not a PNG image.", {{"file", fileName}});
    }

    const uint32_t width = readBigEndian32(bytes, 16);
    const uint32_t height = readBigEndian32(bytes, 20);

    if (width != uint32_t(CustomPortraitFrameWidth) || height != uint32_t(CustomPortraitFrameHeight))
    {
        return Engine::tr("{file} is {width}x{height}; portrait frames are 118x158.",
                          {{"file", fileName}, {"width", uint64_t(width)}, {"height", uint64_t(height)}});
    }

    // Bit depth 8, colour type 6 (RGBA): the transparent background the portrait frame needs.
    if (bytes[24] != 8 || bytes[25] != 6)
    {
        return Engine::tr("{file} must be an 8-bit RGBA PNG with a transparent background.", {{"file", fileName}});
    }

    if (!Engine::decodeImagePixelsBgra(bytes, fileName))
    {
        return Engine::tr("{file} is damaged and cannot be read.", {{"file", fileName}});
    }

    return {};
}

std::string trimmedText(std::string text)
{
    text.erase(std::remove_if(text.begin(), text.end(), [](unsigned char character)
    {
        return character < 0x20 || character == 0x7f;
    }), text.end());
    const size_t first = text.find_first_not_of(' ');
    const size_t last = text.find_last_not_of(' ');
    return first == std::string::npos ? std::string() : text.substr(first, last - first + 1);
}

// The portrait a manifest describes, or the reason it does not describe one.
std::optional<CustomPortrait> parseManifest(const std::string &text, std::string &error)
{
    YAML::Node manifest;

    try
    {
        manifest = YAML::Load(text);
    }
    catch (const YAML::Exception &)
    {
        error = Engine::tr("portrait.yml is not valid YAML.");
        return std::nullopt;
    }

    if (!manifest.IsMap())
    {
        error = Engine::tr("portrait.yml is not valid YAML.");
        return std::nullopt;
    }

    CustomPortrait portrait;

    try
    {
        if (!manifest["schema"] || manifest["schema"].as<int>() != ManifestSchema)
        {
            error = Engine::tr("portrait.yml must have schema: 1.");
            return std::nullopt;
        }

        portrait.id = manifest["id"] ? manifest["id"].as<std::string>() : std::string();
        portrait.name = manifest["name"] ? trimmedText(manifest["name"].as<std::string>()) : std::string();

        if (!manifest["base_character"] || manifest["base_character"].as<int64_t>() <= 0
            || manifest["base_character"].as<int64_t>() > int64_t(UINT32_MAX))
        {
            error = Engine::tr("portrait.yml needs base_character: the character_data id whose doll, race and voice "
                               "the portrait uses.");
            return std::nullopt;
        }

        portrait.baseCharacterId = manifest["base_character"].as<uint32_t>();
    }
    catch (const YAML::Exception &)
    {
        error = Engine::tr("portrait.yml has a field of the wrong type.");
        return std::nullopt;
    }

    if (!validCustomPortraitId(portrait.id))
    {
        error = Engine::tr("The portrait id must be 1-32 lower-case letters, digits or '_'.");
        return std::nullopt;
    }

    if (portrait.name.size() > MaximumNameLength)
    {
        portrait.name.resize(MaximumNameLength);
    }

    if (portrait.name.empty())
    {
        portrait.name = portrait.id;
    }

    return portrait;
}

std::string manifestText(const CustomPortrait &portrait)
{
    YAML::Emitter emitter;
    emitter << YAML::BeginMap << YAML::Key << "schema" << YAML::Value << ManifestSchema << YAML::Key << "id"
            << YAML::Value << portrait.id << YAML::Key << "name" << YAML::Value << portrait.name << YAML::Key
            << "base_character" << YAML::Value << portrait.baseCharacterId << YAML::EndMap;
    return std::string(emitter.c_str()) + "\n";
}

bool writeFile(const std::filesystem::path &path, const std::vector<uint8_t> &bytes)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char *>(bytes.data()), std::streamsize(bytes.size()));
    output.close();
    return !output.fail();
}

void removeFiles(const std::vector<std::filesystem::path> &paths)
{
    for (const std::filesystem::path &path : paths)
    {
        std::error_code error;
        std::filesystem::remove(path, error);
    }
}
}

bool validCustomPortraitId(std::string_view id)
{
    return !id.empty() && id.size() <= MaximumIdLength && std::all_of(id.begin(), id.end(), [](char character)
    {
        return (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '_';
    });
}

std::string customPortraitTexturePrefix(std::string_view id)
{
    return std::string(CustomPortraitTexturePrefix) + std::string(id) + "-";
}

bool isCustomPortraitTextureName(std::string_view textureName)
{
    return textureName.size() > CustomPortraitTexturePrefix.size()
        && std::equal(CustomPortraitTexturePrefix.begin(), CustomPortraitTexturePrefix.end(), textureName.begin(),
                      [](char left, char right)
                      {
                          return left == char(std::tolower(static_cast<unsigned char>(right)));
                      });
}

std::filesystem::path customPortraitIconDirectory(const std::filesystem::path &portraitsFolder)
{
    return portraitsFolder / "icons_x2";
}

std::vector<CustomPortrait> loadCustomPortraits(const std::filesystem::path &portraitsFolder)
{
    std::vector<CustomPortrait> portraits;
    std::error_code error;

    if (!std::filesystem::is_directory(portraitsFolder, error))
    {
        return portraits;
    }

    const std::filesystem::path iconDirectory = customPortraitIconDirectory(portraitsFolder);

    for (std::filesystem::directory_iterator entry(portraitsFolder, error), end; !error && entry != end;
         entry.increment(error))
    {
        std::error_code typeError;

        if (!entry->is_regular_file(typeError) || entry->path().extension() != ".yml")
        {
            continue;
        }

        const std::optional<std::vector<uint8_t>> bytes = readPackageFile(entry->path(), MaximumManifestBytes);
        std::string problem;
        std::optional<CustomPortrait> portrait =
            bytes ? parseManifest(std::string(bytes->begin(), bytes->end()), problem) : std::nullopt;

        if (portrait && portrait->id != entry->path().stem().string())
        {
            portrait.reset();
            problem = "its id does not match the file name";
        }

        for (size_t frame = 1; portrait && frame <= CustomPortraitFrameCount; ++frame)
        {
            if (!std::filesystem::is_regular_file(iconDirectory / installedFrameFileName(portrait->id, frame),
                                                  typeError))
            {
                portrait.reset();
                problem = "frame " + std::to_string(frame) + " is missing";
            }
        }

        if (!portrait)
        {
            std::cout << "Custom portraits: ignoring " << entry->path().filename().string() << ": " << problem << '\n';
            continue;
        }

        portraits.push_back(std::move(*portrait));
    }

    std::sort(portraits.begin(), portraits.end(), [](const CustomPortrait &left, const CustomPortrait &right)
    {
        return left.name != right.name ? left.name < right.name : left.id < right.id;
    });
    return portraits;
}

CustomPortraitImportResult importCustomPortrait(
    const std::filesystem::path &packageFolder,
    const std::filesystem::path &portraitsFolder,
    const CharacterDollTable &characterDollTable)
{
    CustomPortraitImportResult result;
    const std::optional<std::vector<uint8_t>> manifestBytes =
        readPackageFile(packageFolder / CustomPortraitManifestName, MaximumManifestBytes);

    if (!manifestBytes)
    {
        result.error = Engine::tr("The folder has no portrait.yml. Pick the folder of a portrait package.");
        return result;
    }

    std::optional<CustomPortrait> portrait =
        parseManifest(std::string(manifestBytes->begin(), manifestBytes->end()), result.error);

    if (!portrait)
    {
        return result;
    }

    const CharacterDollEntry *pBase = characterDollTable.getCharacter(portrait->baseCharacterId);

    if (pBase == nullptr || pBase->raceId < 0)
    {
        result.error = Engine::tr("base_character {id} is not a character portrait of this game.",
                                  {{"id", uint64_t(portrait->baseCharacterId)}});
        return result;
    }

    std::vector<std::vector<uint8_t>> frames;
    frames.reserve(CustomPortraitFrameCount);

    for (size_t frame = 1; frame <= CustomPortraitFrameCount; ++frame)
    {
        const std::string fileName = frameFileName(frame);
        std::optional<std::vector<uint8_t>> bytes = readPackageFile(packageFolder / fileName, MaximumFrameBytes);

        if (!bytes)
        {
            result.error = Engine::tr("{file} is missing or too large: a package holds the 56 frames 01.png to "
                                      "56.png.", {{"file", fileName}});
            return result;
        }

        result.error = checkFrame(*bytes, fileName);

        if (!result.error.empty())
        {
            return result;
        }

        frames.push_back(std::move(*bytes));
    }

    // Everything is checked: write temporary files first, then replace the old ones, so a failed write leaves the
    // installed portraits as they were.
    std::error_code error;
    const std::filesystem::path iconDirectory = customPortraitIconDirectory(portraitsFolder);
    std::filesystem::create_directories(iconDirectory, error);
    const std::filesystem::path manifestPath = portraitsFolder / (portrait->id + ".yml");
    result.replaced = std::filesystem::exists(manifestPath, error);
    std::vector<std::filesystem::path> temporaryFiles;
    std::vector<std::filesystem::path> finalFiles;

    for (size_t frame = 1; frame <= CustomPortraitFrameCount; ++frame)
    {
        finalFiles.push_back(iconDirectory / installedFrameFileName(portrait->id, frame));
        temporaryFiles.push_back(finalFiles.back().string() + std::string(ImportSuffix));

        if (!writeFile(temporaryFiles.back(), frames[frame - 1]))
        {
            removeFiles(temporaryFiles);
            result.error = Engine::tr("The portrait could not be written to the portraits folder.");
            return result;
        }
    }

    const std::string manifest = manifestText(*portrait);
    finalFiles.push_back(manifestPath);
    temporaryFiles.push_back(manifestPath.string() + std::string(ImportSuffix));

    if (!writeFile(temporaryFiles.back(), std::vector<uint8_t>(manifest.begin(), manifest.end())))
    {
        removeFiles(temporaryFiles);
        result.error = Engine::tr("The portrait could not be written to the portraits folder.");
        return result;
    }

    // The manifest goes last: a portrait without it is not offered.
    for (size_t index = 0; index < finalFiles.size(); ++index)
    {
        std::filesystem::rename(temporaryFiles[index], finalFiles[index], error);

        if (error)
        {
            removeFiles(std::vector<std::filesystem::path>(temporaryFiles.begin() + index, temporaryFiles.end()));
            result.error = Engine::tr("The portrait could not be written to the portraits folder.");
            return result;
        }
    }

    result.success = true;
    result.portrait = std::move(*portrait);
    return result;
}
}
