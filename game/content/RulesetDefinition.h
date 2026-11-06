#pragma once

#include "engine/AssetFileSystem.h"
#include "game/content/ContentManifest.h"
#include "game/content/RulesetOptions.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace OpenYAMM::Game
{
// What a ruleset offers at new game. A list with one entry is a fixed choice; empty lists mean the ruleset has no such
// option (Standard).
struct RulesetChoices
{
    std::vector<RulesetDifficulty> difficulties;
    RulesetDifficulty defaultDifficulty = RulesetDifficulty::Maw;
    std::vector<RulesetProgression> progressions;
    RulesetProgression defaultProgression = RulesetProgression::Free;
    std::vector<RulesetChallengeMode> challengeModes;
    bool randomizer = false;
    bool preferences = false;
};

// What the new-game Rules panel shows for a ruleset (`rulesPanel` in ruleset.yml); display text only.
struct RulesetPanelText
{
    struct Difficulty
    {
        // One cell per difficulty column.
        std::vector<std::string> values;
        std::string description;
        std::string warning;
    };

    std::string title;
    std::string subtitle;
    // The line beside the ruleset switch's Rules button and the continent screen's footer note.
    std::string summary;
    std::vector<std::string> difficultyColumns;
    std::map<RulesetDifficulty, Difficulty> difficulties;
    std::map<RulesetProgression, std::string> progressions;
    std::map<RulesetChallengeMode, std::string> challengeModes;
    std::string randomizer;
};

// One engine table the ruleset changes. `table` is the path below engine/data_tables/ ("class_skills.txt",
// "english/potion.txt"); `path` is inside the ruleset package.
// - Override: a tab-separated patch with a "key<TAB>column<TAB>value" header and one changed cell per line. The key
//   selects the single base row whose keyColumn cell matches (keyColumn "row": the row's zero-based number in the
//   file, where a number past the end adds rows); the column is a header name from headerRow or "#<index>". An
//   optional `append` file adds whole rows in the table's own layout after the overrides, and an optional
//   `appendColumns` file adds columns: its first row names them, each other row fills them for the base row of its
//   key.
// - Replace: the patch file replaces the whole table.
struct RulesetTableContribution
{
    std::string table;
    TableContributionMode mode = TableContributionMode::Override;
    std::string path;
    std::string appendPath;
    std::string appendColumnsPath;
    size_t keyColumn = 0;
    size_t headerRow = 0;
};

// One card of the new-game start choice (`newGame.starts` in ruleset.yml). A ruleset with start cards shows them on
// the continent screen instead of the three continents; choosing one opens party creation for the
// `new_game_continents` entry of its key in character_selection.yml (the engine's, or the ruleset's replacement).
struct RulesetStartCard
{
    std::string continentKey;
    std::string title;
    // The small line above the title, where a continent card names its game.
    std::string subtitle;
    // The line below the title ("Begin in ..."), also shown on the party creation screen.
    std::string startText;
    // A picture name resolved like a menu texture (a Data/icons name or a hud_x2/ path); empty for none.
    std::string illustration;
};

// At most this many start cards fit the continent screen.
inline constexpr size_t MaxRulesetStartCards = 3;

// A ruleset package, engine/rulesets/<id>/ruleset.yml (devdocs/maw/HANDOFF.md §3.2). The Standard ruleset has no
// package.
struct RulesetDefinition
{
    std::string id = StandardRulesetId;
    std::string name = "Standard MMMerge";
    std::string description;
    std::vector<std::string> credits;
    // A web page explaining the ruleset to players (`guide_url`), linked from its Rules panel and its journal; empty
    // for none.
    std::string guideUrl;
    uint32_t contentSchemaVersion = 1;
    // The ids its tables add, by domain, like a world's (world.yml idRanges); they may not overlap a mounted world's.
    std::vector<ContentIdRangeDefinition> idRanges;
    RulesetChoices choices;
    RulesetPanelText panel;
    std::vector<RulesetTableContribution> tables;
    // Skills the ruleset shows under another name (`skillNames`: canonical skill name -> display name).
    std::map<std::string, std::string> skillNames;
    // The ruleset's own new-game starts; empty keeps the three continents.
    std::vector<RulesetStartCard> startCards;

    bool isStandard() const;
    // Whether the new game offers any rules choice for the Rules panel.
    bool hasRulesChoices() const;
    RulesetOptions defaultOptions() const;
    // Whether these options are among the choices and agree with the chosen challenge mode (preferences are always
    // allowed).
    bool offers(const RulesetOptions &options) const;
    // The contributions to an engine table, in the order they apply (a generated patch, then hand-written ones).
    std::vector<const RulesetTableContribution *> findTables(const std::string &engineTableVirtualPath) const;
};

RulesetDefinition standardRulesetDefinition();
// The folder holding the ruleset packages.
inline constexpr const char *RulesetsRoot = "engine/rulesets";
// "engine/rulesets/<id>/", the package root of a ruleset. The rulesets live in the engine package so every released
// updater accepts their packs (devdocs/maw/AUDIT_20261024_PLAN.md, D15).
std::string rulesetPackageRoot(const std::string &rulesetId);
std::optional<RulesetDefinition> parseRulesetDefinition(
    const std::string &yamlText,
    const std::string &expectedId,
    std::string &errorMessage);
// The Standard definition for "mmerge"; otherwise the mounted package, or nullopt when it is missing or invalid.
std::optional<RulesetDefinition> loadRulesetDefinition(
    const Engine::AssetFileSystem &assetFileSystem,
    const std::string &rulesetId,
    std::string &errorMessage);
// Standard first, then every valid mounted ruleset package by id. False, with the reasons in errorMessage, when a
// package was left out because it is invalid; `rulesets` still holds the valid ones.
bool loadAvailableRulesets(
    const Engine::AssetFileSystem &assetFileSystem,
    std::vector<RulesetDefinition> &rulesets,
    std::string &errorMessage);
}
