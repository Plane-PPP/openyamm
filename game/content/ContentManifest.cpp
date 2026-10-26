#include "game/content/ContentManifest.h"

#include "game/maps/MapIdentity.h"

#include <yaml-cpp/yaml.h>

#include <cctype>
#include <cmath>
#include <exception>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_set>

namespace OpenYAMM::Game
{
namespace
{
std::string scalarStringOrDefault(const YAML::Node &node, const char *pKey, const std::string &defaultValue)
{
    const YAML::Node child = node[pKey];

    if (!child || !child.IsScalar())
    {
        return defaultValue;
    }

    return child.as<std::string>();
}

uint32_t scalarUint32OrDefault(const YAML::Node &node, const char *pKey, uint32_t defaultValue)
{
    const YAML::Node child = node[pKey];
    return child && child.IsScalar() ? child.as<uint32_t>() : defaultValue;
}

std::string normalizedToken(std::string value)
{
    for (char &character : value)
    {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }

    return value;
}

bool isSafePackageRelativePath(const std::string &path)
{
    if (path.empty())
    {
        return false;
    }

    const std::filesystem::path parsedPath(path);

    if (parsedPath.is_absolute())
    {
        return false;
    }

    for (const std::filesystem::path &component : parsedPath)
    {
        if (component == "..")
        {
            return false;
        }
    }

    return true;
}

std::optional<TableContributionMode> parseContributionMode(const std::string &value)
{
    const std::string normalized = normalizedToken(value);

    if (normalized == "append")
    {
        return TableContributionMode::Append;
    }

    if (normalized == "override")
    {
        return TableContributionMode::Override;
    }

    if (normalized == "distribution")
    {
        return TableContributionMode::Distribution;
    }

    return std::nullopt;
}

WorldManifest buildDefaultWorldManifest(const std::string &worldId)
{
    WorldManifest manifest = {};
    manifest.id = normalizeWorldId(worldId);
    manifest.name = manifest.id == "mm8" ? "MM8" : manifest.id;
    manifest.sourceGame = manifest.id;
    manifest.start.mapFileName = "out01.odm";
    return manifest;
}

bool parseManifestRoot(const YAML::Node &root, WorldManifest &manifest, std::string &errorMessage)
{
    if (!root || !root.IsMap())
    {
        errorMessage = "world.yml root must be a map";
        return false;
    }

    manifest.id = normalizeWorldId(scalarStringOrDefault(root, "id", manifest.id));
    manifest.name = scalarStringOrDefault(root, "name", manifest.name);
    manifest.sourceGame = scalarStringOrDefault(root, "sourceGame", manifest.sourceGame);
    manifest.contentSchemaVersion = scalarUint32OrDefault(root, "contentSchemaVersion", 1);

    if (manifest.contentSchemaVersion == 0)
    {
        errorMessage = "world.yml contentSchemaVersion must be non-zero";
        return false;
    }

    const YAML::Node startNode = root["start"];

    if (startNode && startNode.IsMap())
    {
        manifest.start.mapFileName = scalarStringOrDefault(startNode, "map", manifest.start.mapFileName);
        manifest.start.introMovie = scalarStringOrDefault(startNode, "introMovie", manifest.start.introMovie);
    }

    const YAML::Node qbitsNode = root["qbits"];
    if (qbitsNode && qbitsNode.IsMap())
    {
        manifest.qbits.begin = scalarUint32OrDefault(qbitsNode, "begin", 0);
        manifest.qbits.end = scalarUint32OrDefault(qbitsNode, "end", 0);
        manifest.qbits.mapping = scalarStringOrDefault(qbitsNode, "mapping", {});
        manifest.qbits.declared = manifest.qbits.begin != 0 || manifest.qbits.end != 0;

        if (!manifest.qbits.declared || manifest.qbits.begin > manifest.qbits.end)
        {
            errorMessage = "world.yml qbits range must declare an ordered, non-zero begin and end";
            return false;
        }
    }

    const YAML::Node partyMovementNode = root["partyMovement"];
    if (partyMovementNode)
    {
        if (!partyMovementNode.IsMap())
        {
            errorMessage = "world.yml partyMovement must declare eyeHeight, collisionRadius, and collisionHeight";
            return false;
        }

        const YAML::Node eyeHeightNode = partyMovementNode["eyeHeight"];
        const YAML::Node collisionRadiusNode = partyMovementNode["collisionRadius"];
        const YAML::Node collisionHeightNode = partyMovementNode["collisionHeight"];
        const YAML::Node maxStepHeightNode = partyMovementNode["maxStepHeight"];

        if (!eyeHeightNode || !eyeHeightNode.IsScalar()
            || !collisionRadiusNode || !collisionRadiusNode.IsScalar()
            || !collisionHeightNode || !collisionHeightNode.IsScalar())
        {
            errorMessage = "world.yml partyMovement must declare eyeHeight, collisionRadius, and collisionHeight";
            return false;
        }

        manifest.partyMovement.eyeHeight = eyeHeightNode.as<float>();
        manifest.partyMovement.collisionRadius = collisionRadiusNode.as<float>();
        manifest.partyMovement.collisionHeight = collisionHeightNode.as<float>();
        if (maxStepHeightNode)
        {
            if (!maxStepHeightNode.IsScalar())
            {
                errorMessage = "world.yml partyMovement maxStepHeight must be scalar";
                return false;
            }

            manifest.partyMovement.maxStepHeight = maxStepHeightNode.as<float>();
        }
        manifest.partyMovement.declared = true;

        if (!std::isfinite(manifest.partyMovement.eyeHeight)
            || !std::isfinite(manifest.partyMovement.collisionRadius)
            || !std::isfinite(manifest.partyMovement.collisionHeight)
            || !std::isfinite(manifest.partyMovement.maxStepHeight)
            || manifest.partyMovement.eyeHeight <= 0.0f
            || manifest.partyMovement.collisionRadius <= 0.0f
            || manifest.partyMovement.collisionHeight <= 0.0f
            || manifest.partyMovement.maxStepHeight <= 0.0f
            || manifest.partyMovement.eyeHeight > manifest.partyMovement.collisionHeight)
        {
            errorMessage =
                "world.yml partyMovement dimensions must be finite and positive, "
                "with eyeHeight at most collisionHeight";
            return false;
        }
    }

    const YAML::Node actorAwarenessNode = root["actorAwareness"];
    if (actorAwarenessNode)
    {
        if (!actorAwarenessNode.IsMap())
        {
            errorMessage = "world.yml actorAwareness must declare partyEngagementRange";
            return false;
        }

        const YAML::Node partyEngagementRangeNode = actorAwarenessNode["partyEngagementRange"];
        if (!partyEngagementRangeNode || !partyEngagementRangeNode.IsScalar())
        {
            errorMessage = "world.yml actorAwareness must declare partyEngagementRange";
            return false;
        }

        manifest.actorAwareness.partyEngagementRange = partyEngagementRangeNode.as<float>();
        manifest.actorAwareness.declared = true;

        if (!std::isfinite(manifest.actorAwareness.partyEngagementRange)
            || manifest.actorAwareness.partyEngagementRange <= 0.0f)
        {
            errorMessage = "world.yml actorAwareness partyEngagementRange must be finite and positive";
            return false;
        }
    }

    const YAML::Node dependenciesNode = root["dependencies"];
    if (dependenciesNode)
    {
        if (!dependenciesNode.IsSequence())
        {
            errorMessage = "world.yml dependencies must be a sequence";
            return false;
        }

        std::unordered_set<std::string> dependencyIds;

        for (const YAML::Node &dependencyNode : dependenciesNode)
        {
            if (!dependencyNode.IsScalar())
            {
                errorMessage = "world.yml dependencies entries must be scalar package ids";
                return false;
            }

            const std::string dependencyId = normalizeWorldId(dependencyNode.as<std::string>());

            if (dependencyId.empty() || dependencyId == manifest.id || !dependencyIds.insert(dependencyId).second)
            {
                errorMessage = "world.yml dependencies contain an empty, self, or duplicate package id";
                return false;
            }

            manifest.dependencies.push_back(dependencyId);
        }
    }

    if (!parseContentIdRanges(root["idRanges"], "world.yml", manifest.idRanges, errorMessage))
    {
        return false;
    }

    const YAML::Node contributionsNode = root["tableContributions"];
    if (contributionsNode)
    {
        if (!contributionsNode.IsSequence())
        {
            errorMessage = "world.yml tableContributions must be a sequence";
            return false;
        }

        std::unordered_set<std::string> contributionKeys;

        for (const YAML::Node &contributionNode : contributionsNode)
        {
            if (!contributionNode.IsMap())
            {
                errorMessage = "world.yml tableContributions entries must be maps";
                return false;
            }

            TableContributionDefinition contribution = {};
            contribution.table = normalizedToken(scalarStringOrDefault(contributionNode, "table", {}));
            contribution.idDomain = normalizedToken(
                scalarStringOrDefault(contributionNode, "idDomain", contribution.table));
            contribution.path = scalarStringOrDefault(contributionNode, "path", {});
            contribution.sourceIdColumn = scalarUint32OrDefault(contributionNode, "sourceIdColumn", 0);
            contribution.canonicalIdColumn = scalarUint32OrDefault(contributionNode, "canonicalIdColumn", 0);
            const std::optional<TableContributionMode> mode =
                parseContributionMode(scalarStringOrDefault(contributionNode, "mode", {}));

            if (contribution.table.empty() || contribution.idDomain.empty() || !mode
                || !isSafePackageRelativePath(contribution.path))
            {
                errorMessage = "world.yml tableContributions contain an invalid table, mode, or relative path";
                return false;
            }

            contribution.mode = *mode;

            if (contribution.mode == TableContributionMode::Append
                && contribution.table == "items"
                && contribution.sourceIdColumn == contribution.canonicalIdColumn)
            {
                errorMessage = "world.yml item contribution requires distinct source and canonical id columns";
                return false;
            }

            const std::string contributionKey = contribution.table + "\n" + contribution.path;

            if (!contributionKeys.insert(contributionKey).second)
            {
                errorMessage = "world.yml tableContributions contain a duplicate table/path entry";
                return false;
            }

            if (contribution.mode == TableContributionMode::Append
                && manifest.findIdRange(contribution.idDomain) == nullptr)
            {
                errorMessage = "world.yml append contribution requires a declared id range for its table";
                return false;
            }

            manifest.tableContributions.push_back(std::move(contribution));
        }
    }

    manifest.loadedFromFile = true;
    return true;
}

}

const ContentIdRangeDefinition *WorldManifest::findIdRange(const std::string &domain) const
{
    const std::string normalizedDomain = normalizedToken(domain);

    for (const ContentIdRangeDefinition &range : idRanges)
    {
        if (range.domain == normalizedDomain)
        {
            return &range;
        }
    }

    return nullptr;
}

std::optional<WorldManifest> loadWorldManifest(
    const Engine::AssetFileSystem &assetFileSystem,
    const std::string &virtualPath,
    const std::string &fallbackWorldId,
    std::string &errorMessage)
{
    errorMessage.clear();
    const std::optional<std::string> manifestText = assetFileSystem.readTextFile(virtualPath);

    if (!manifestText)
    {
        errorMessage = "could not read " + virtualPath;
        return std::nullopt;
    }

    WorldManifest manifest = buildDefaultWorldManifest(fallbackWorldId);

    try
    {
        const YAML::Node root = YAML::Load(*manifestText);

        if (!parseManifestRoot(root, manifest, errorMessage))
        {
            return std::nullopt;
        }
    }
    catch (const std::exception &exception)
    {
        errorMessage = "could not parse " + virtualPath + ": " + exception.what();
        return std::nullopt;
    }

    return manifest;
}

WorldManifest loadActiveWorldManifestOrDefault(
    const Engine::AssetFileSystem &assetFileSystem,
    const std::string &worldId,
    std::string &errorMessage)
{
    WorldManifest manifest = buildDefaultWorldManifest(worldId);
    const std::optional<std::string> manifestText = assetFileSystem.readTextFile("world.yml");

    if (!manifestText)
    {
        return manifest;
    }

    try
    {
        const YAML::Node root = YAML::Load(*manifestText);
        parseManifestRoot(root, manifest, errorMessage);
    }
    catch (const std::exception &exception)
    {
        errorMessage = std::string("could not parse world.yml: ") + exception.what();
    }

    return manifest;
}


bool parseContentIdRanges(
    const YAML::Node &node,
    const std::string &owner,
    std::vector<ContentIdRangeDefinition> &ranges,
    std::string &errorMessage)
{
    if (!node)
    {
        return true;
    }

    if (!node.IsMap())
    {
        errorMessage = owner + " idRanges must be a map";
        return false;
    }

    std::unordered_set<std::string> domains;

    for (YAML::const_iterator iterator = node.begin(); iterator != node.end(); ++iterator)
    {
        if (!iterator->first.IsScalar() || !iterator->second.IsMap())
        {
            errorMessage = owner + " idRanges entries must map a domain to a range";
            return false;
        }

        ContentIdRangeDefinition range = {};
        range.domain = normalizedToken(iterator->first.as<std::string>());
        range.begin = scalarUint32OrDefault(iterator->second, "begin", 0);
        range.end = scalarUint32OrDefault(iterator->second, "end", 0);

        if (range.domain.empty() || range.begin == 0 || range.begin > range.end || !domains.insert(range.domain).second)
        {
            errorMessage = owner + " idRanges contain an invalid or duplicate domain range";
            return false;
        }

        ranges.push_back(std::move(range));
    }

    return true;
}

std::optional<std::string> overlappingContentIdRange(
    const std::vector<ContentIdRangeDefinition> &left,
    const std::vector<ContentIdRangeDefinition> &right)
{
    for (const ContentIdRangeDefinition &first : left)
    {
        for (const ContentIdRangeDefinition &second : right)
        {
            if (first.domain == second.domain && first.begin <= second.end && second.begin <= first.end)
            {
                return first.domain;
            }
        }
    }

    return std::nullopt;
}
}
