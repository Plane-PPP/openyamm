#pragma once

#include "engine/AssetFileSystem.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace YAML
{
class Node;
}

namespace OpenYAMM::Game
{
struct WorldStartDefinition
{
    std::string mapFileName;
    std::string introMovie;
};

struct WorldQBitDefinition
{
    uint32_t begin = 0;
    uint32_t end = 0;
    std::string mapping;
    bool declared = false;
};

struct WorldPartyMovementDefinition
{
    float eyeHeight = 0.0f;
    float collisionRadius = 0.0f;
    float collisionHeight = 0.0f;
    float maxStepHeight = 128.0f;
    bool declared = false;
};

struct WorldActorAwarenessDefinition
{
    float partyEngagementRange = 0.0f;
    bool declared = false;
};

enum class TableContributionMode
{
    Append,
    Override,
    Distribution,
    // Whole-table replacement; ruleset packages only.
    Replace,
};

struct ContentIdRangeDefinition
{
    std::string domain;
    uint32_t begin = 0;
    uint32_t end = 0;
};

// Parses a manifest's idRanges map (domain: {begin, end}) into ranges; `owner` names the file in errors.
bool parseContentIdRanges(
    const YAML::Node &node,
    const std::string &owner,
    std::vector<ContentIdRangeDefinition> &ranges,
    std::string &errorMessage);
// The first domain whose ranges in the two lists overlap, if any.
std::optional<std::string> overlappingContentIdRange(
    const std::vector<ContentIdRangeDefinition> &left,
    const std::vector<ContentIdRangeDefinition> &right);

struct TableContributionDefinition
{
    std::string table;
    std::string idDomain;
    TableContributionMode mode = TableContributionMode::Append;
    std::string path;
    size_t sourceIdColumn = 0;
    size_t canonicalIdColumn = 0;
};

struct WorldManifest
{
    std::string id = "mm8";
    std::string name = "MM8";
    std::string sourceGame = "mm8";
    WorldStartDefinition start;
    WorldQBitDefinition qbits;
    WorldPartyMovementDefinition partyMovement;
    WorldActorAwarenessDefinition actorAwareness;
    uint32_t contentSchemaVersion = 1;
    std::vector<std::string> dependencies;
    std::vector<ContentIdRangeDefinition> idRanges;
    std::vector<TableContributionDefinition> tableContributions;
    bool loadedFromFile = false;

    const ContentIdRangeDefinition *findIdRange(const std::string &domain) const;
};

WorldManifest loadActiveWorldManifestOrDefault(
    const Engine::AssetFileSystem &assetFileSystem,
    const std::string &worldId,
    std::string &errorMessage
);

std::optional<WorldManifest> loadWorldManifest(
    const Engine::AssetFileSystem &assetFileSystem,
    const std::string &virtualPath,
    const std::string &fallbackWorldId,
    std::string &errorMessage
);
}
