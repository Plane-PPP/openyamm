#include "game/content/DungeonRun.h"

#include "game/content/RulesetDefinition.h"
#include "game/rules/RandomStreams.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cctype>
#include <exception>
#include <numeric>
#include <unordered_set>
#include <utility>

namespace OpenYAMM::Game
{
namespace
{
std::string lowerCaseCopy(const std::string &text)
{
    std::string result = text;
    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return result;
}

bool readEntranceCoordinate(const YAML::Node &entranceNode, const char *pKey, int32_t &value)
{
    const YAML::Node node = entranceNode[pKey];

    if (!node || !node.IsScalar())
    {
        return false;
    }

    value = node.as<int32_t>();
    return true;
}
}

std::string dungeonRunDefinitionPath(const std::string &rulesetId)
{
    return rulesetPackageRoot(rulesetId) + "run.yml";
}

std::optional<DungeonRunDefinition> parseDungeonRunDefinition(const std::string &yamlText, std::string &errorMessage)
{
    errorMessage.clear();
    DungeonRunDefinition definition = {};

    try
    {
        const YAML::Node root = YAML::Load(yamlText);

        if (!root || !root.IsMap())
        {
            errorMessage = "run.yml root must be a map";
            return std::nullopt;
        }

        const YAML::Node dungeonsNode = root["dungeons"];

        if (!dungeonsNode || !dungeonsNode.IsSequence() || dungeonsNode.size() == 0)
        {
            errorMessage = "run.yml needs a list of dungeons";
            return std::nullopt;
        }

        std::unordered_set<std::string> seenMaps;

        for (const YAML::Node &dungeonNode : dungeonsNode)
        {
            if (!dungeonNode.IsMap() || !dungeonNode["map"] || !dungeonNode["map"].IsScalar())
            {
                errorMessage = "every run.yml dungeon needs a map";
                return std::nullopt;
            }

            DungeonRunStop stop = {};
            stop.mapFileName = dungeonNode["map"].as<std::string>();

            if (stop.mapFileName.empty() || !seenMaps.insert(lowerCaseCopy(stop.mapFileName)).second)
            {
                errorMessage = "run.yml dungeon map '" + stop.mapFileName + "' is empty or listed twice";
                return std::nullopt;
            }

            const YAML::Node entranceNode = dungeonNode["entrance"];

            if (!entranceNode
                || !entranceNode.IsMap()
                || !readEntranceCoordinate(entranceNode, "x", stop.x)
                || !readEntranceCoordinate(entranceNode, "y", stop.y)
                || !readEntranceCoordinate(entranceNode, "z", stop.z))
            {
                errorMessage = "run.yml dungeon '" + stop.mapFileName + "' needs an entrance with x, y and z";
                return std::nullopt;
            }

            if (entranceNode["yaw"])
            {
                stop.yawUnits = entranceNode["yaw"].as<int32_t>();
            }

            definition.dungeons.push_back(std::move(stop));
        }
    }
    catch (const std::exception &exception)
    {
        errorMessage = std::string("run.yml could not be parsed: ") + exception.what();
        return std::nullopt;
    }

    return definition;
}

std::optional<DungeonRunDefinition> loadDungeonRunDefinition(
    const Engine::AssetFileSystem &assetFileSystem,
    const std::string &rulesetId,
    std::string &errorMessage)
{
    errorMessage.clear();

    if (rulesetId.empty() || rulesetId == StandardRulesetId)
    {
        return std::nullopt;
    }

    const std::string path = dungeonRunDefinitionPath(rulesetId);

    if (!assetFileSystem.exists(path))
    {
        return std::nullopt;
    }

    const std::optional<std::string> text = assetFileSystem.readTextFile(path);

    if (!text)
    {
        errorMessage = path + " could not be read";
        return std::nullopt;
    }

    std::optional<DungeonRunDefinition> definition = parseDungeonRunDefinition(*text, errorMessage);

    if (!definition)
    {
        errorMessage = path + ": " + errorMessage;
    }

    return definition;
}

std::vector<size_t> dungeonRunOrder(size_t dungeonCount, uint64_t rootSeed)
{
    std::vector<size_t> order(dungeonCount);
    std::iota(order.begin(), order.end(), size_t(0));
    RandomStream stream(rootSeed ^ randomNameKey("dungeon_run.order"));

    // Fisher-Yates from the back: every order is equally likely.
    for (size_t remaining = dungeonCount; remaining > 1; --remaining)
    {
        const size_t pick = stream.uniform(0, static_cast<uint32_t>(remaining - 1));
        std::swap(order[remaining - 1], order[pick]);
    }

    return order;
}

const DungeonRunStop *dungeonRunStop(const DungeonRunDefinition &run, uint64_t rootSeed, size_t entered)
{
    if (entered >= run.dungeons.size())
    {
        return nullptr;
    }

    return &run.dungeons[dungeonRunOrder(run.dungeons.size(), rootSeed)[entered]];
}

std::string dungeonRunEndText(bool defeated, size_t completedDungeons)
{
    return std::string(defeated ? "Run over." : "Run complete!")
        + " Dungeons cleared: " + std::to_string(completedDungeons) + ".";
}
}
