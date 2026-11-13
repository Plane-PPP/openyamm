#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace OpenYAMM::Game
{
class CharacterDollTable;
struct CharacterDollEntry;

// The doll images a package may replace: the base row's body (the head is painted on it) and hands
// (character_data columns bod, LHd, LHu, LHo, RHb, RHd, RHu). The package files are doll/body.png and
// doll/hand_<name>.png, each the size of the base row's image.
enum class CustomDollPart : uint8_t
{
    Body,
    LeftHandClosed,
    LeftHandHold,
    LeftHandOpen,
    RightHandFingers,
    RightHandOpen,
    RightHandHold,
    Count,
};

// Party portraits the player imports from a folder (devdocs/CUSTOM_PORTRAITS.md). A portrait package is a folder with
// portrait.yml and the 56 face frames 01.png ... 56.png. The import copies it into the user data:
// portraits/<id>.yml and portraits/icons_x2/user_<id>-01.png ... -56.png, which AssetFileSystem::setUserIconDirectory
// serves as ordinary icons. A portrait borrows the doll, race, sex and voice of an existing character_data row (its
// base character); only the face is its own, so a save keeps working when the portrait is gone.
struct CustomPortrait
{
    // 1-32 lower-case letters, digits or '_': the file names and the save's portraitTextureName derive from it.
    std::string id;
    // Shown to the player; the id when the package names none.
    std::string name;
    // The character_data row whose doll, race, sex and default voice the portrait uses.
    uint32_t baseCharacterId = 0;
    // The doll images the portrait brings; the base row's own are used for the rest.
    std::vector<CustomDollPart> dollParts;
};

inline constexpr size_t CustomPortraitFrameCount = 56;
// Every frame at the x2 scale of the original 59x79 party portraits.
inline constexpr int CustomPortraitFrameWidth = 118;
inline constexpr int CustomPortraitFrameHeight = 158;
inline constexpr std::string_view CustomPortraitManifestName = "portrait.yml";
// The user data folder (the working directory, like "saves") that holds the imported portraits.
inline constexpr std::string_view CustomPortraitsFolderName = "portraits";
inline constexpr std::string_view CustomPortraitTexturePrefix = "user_";

bool validCustomPortraitId(std::string_view id);
// The portrait's face texture names without the frame number: user_<id>- (frames user_<id>-01 ... -56).
std::string customPortraitTexturePrefix(std::string_view id);
// Whether a party member's portraitTextureName names an imported portrait.
bool isCustomPortraitTextureName(std::string_view textureName);
// Where the imported frames live: <portraits folder>/icons_x2.
std::filesystem::path customPortraitIconDirectory(const std::filesystem::path &portraitsFolder);
// "body", "left_closed", "left_hold", "left_open", "right_fingers", "right_open", "right_hold".
std::string_view customDollPartName(CustomDollPart part);
// The base row's image of a doll part (bodyAsset, leftHandClosedAsset ...).
const std::string &characterDollPartAsset(const CharacterDollEntry &entry, CustomDollPart part);
// An installed doll part's texture name: user_<id>_body, user_<id>_hand_left_closed ... (no frame digits at the end).
std::string customDollPartTextureName(std::string_view id, CustomDollPart part);
// The doll a member with this portraitTextureName shows: the base row with the imported portrait's body and hands
// where installed() finds their textures, the base row's own elsewhere (and for every built-in portrait).
CharacterDollEntry customPortraitDollEntry(
    const CharacterDollEntry &base,
    std::string_view portraitTextureName,
    const std::function<bool(const std::string &textureName)> &installed);

// The imported portraits of the portraits folder, by name. A manifest whose id does not match its file name or whose
// frames are not all present is skipped with a log line.
std::vector<CustomPortrait> loadCustomPortraits(const std::filesystem::path &portraitsFolder);

struct CustomPortraitImportResult
{
    bool success = false;
    // The portrait replaced an earlier import of the same id.
    bool replaced = false;
    CustomPortrait portrait;
    // Why the import failed, for the player.
    std::string error;
};

// Checks a portrait package and installs it into the portraits folder. Nothing is written unless the whole package is
// valid: a known base character, all 56 frames as 118x158 8-bit RGBA PNG files that decode, and each doll image an
// x2 8-bit RGBA PNG twice the logical size of the base row's image it replaces. Files are read by their
// fixed names only and written under names derived from the checked id, so a package cannot reach outside its folder
// or the portraits folder. Re-importing an id replaces its frames and manifest.
// The logical size (the original 1x pixels) of a base row's doll image, or nothing when it cannot be read.
using DollAssetSizeLookup = std::function<std::optional<std::pair<int, int>>(const std::string &assetName)>;

CustomPortraitImportResult importCustomPortrait(
    const std::filesystem::path &packageFolder,
    const std::filesystem::path &portraitsFolder,
    const CharacterDollTable &characterDollTable,
    const DollAssetSizeLookup &dollAssetSize);
}
