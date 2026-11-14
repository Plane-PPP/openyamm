#include "game/content/DungeonRun.h"

#include "game/content/RulesetDefinition.h"
#include "game/party/SkillData.h"
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

bool parseRunRules(const YAML::Node &rulesNode, DungeonRunDefinition &definition, std::string &errorMessage)
{
    DungeonRunRules rules = {};

    if (const YAML::Node node = rulesNode["route"])
    {
        rules.route = node.as<std::vector<int>>();
    }

    if (rules.route.empty() || rules.route.size() > definition.dungeons.size())
    {
        errorMessage = "run.yml rules need a route no longer than the list of dungeons";
        return false;
    }

    if (const YAML::Node node = rulesNode["clear"])
    {
        rules.clearShare = std::clamp(node["share"].as<double>(rules.clearShare), 0.01, 1.0);
        rules.guardianHealthScale = std::max(0.1f, node["guardianHealthScale"].as<float>(rules.guardianHealthScale));
    }

    if (const YAML::Node node = rulesNode["levels"])
    {
        rules.levelUps = node["perStop"].as<std::vector<int>>(std::vector<int>());
        rules.levelsPerLevelUp = std::max(1, node["levelsEach"].as<int>(rules.levelsPerLevelUp));
    }

    rules.levelUps.resize(rules.route.size(), 0);

    if (const YAML::Node node = rulesNode["start"])
    {
        rules.startFood = std::max(0, node["food"].as<int>(0));
        rules.startGold = std::max(0, node["gold"].as<int>(0));
    }

    rules.promotionAfterStop = rulesNode["promotionAfterStop"].as<int>(0);

    if (const YAML::Node node = rulesNode["books"])
    {
        rules.booksNewSchool = std::max(0, node["newSchool"].as<int>(rules.booksNewSchool));
        rules.booksRaisedSchool = std::max(0, node["raisedSchool"].as<int>(rules.booksRaisedSchool));
    }

    if (const YAML::Node node = rulesNode["skills"])
    {
        for (const std::string &skill : node.as<std::vector<std::string>>())
        {
            rules.skills.push_back(canonicalSkillName(skill));
        }
    }

    for (const YAML::Node &node : rulesNode["schools"])
    {
        DungeonRunSchool school = {};
        school.skill = canonicalSkillName(node["skill"].as<std::string>());
        school.firstSpell = node["firstSpell"].as<uint32_t>();
        school.combatSpell = node["combatSpell"].as<uint32_t>(0);

        if (school.skill.empty() || school.firstSpell == 0)
        {
            errorMessage = "every run.yml school needs a skill and its firstSpell";
            return false;
        }

        rules.schools.push_back(std::move(school));
    }

    for (const YAML::Node &node : rulesNode["shops"])
    {
        DungeonRunShop shop = {};
        shop.afterStop = node["afterStop"].as<int>(0);
        shop.name = node["name"].as<std::string>("");
        shop.houseId = node["house"].as<uint32_t>(0);

        if (shop.afterStop <= 0 || shop.name.empty() || shop.houseId == 0)
        {
            errorMessage = "every run.yml shop needs afterStop, a name and a house";
            return false;
        }

        rules.shops.push_back(std::move(shop));
    }

    std::unordered_set<std::string> seenPerks;

    for (const YAML::Node &node : rulesNode["perks"])
    {
        DungeonRunPerk perk = {};
        perk.id = node["id"].as<std::string>();
        perk.name = node["name"].as<std::string>(perk.id);
        perk.stat = node["stat"].as<std::string>();
        perk.amount = node["amount"].as<int>(0);
        perk.maxLevel = std::max(1, node["maxLevel"].as<int>(1));
        perk.requiresSkill = canonicalSkillName(node["requiresSkill"].as<std::string>(""));

        if (perk.id.empty() || perk.stat.empty() || !seenPerks.insert(perk.id).second)
        {
            errorMessage = "run.yml perk '" + perk.id + "' needs an id of its own and a stat";
            return false;
        }

        rules.perks.push_back(std::move(perk));
    }

    definition.rules = std::move(rules);
    return true;
}
}

int DungeonRunRules::totalLevelUps() const
{
    return std::accumulate(levelUps.begin(), levelUps.end(), 0);
}

int DungeonRunRules::levelUpsBefore(size_t stop) const
{
    return std::accumulate(levelUps.begin(), levelUps.begin() + std::min(stop, levelUps.size()), 0);
}

const DungeonRunSchool *DungeonRunRules::school(const std::string &skill) const
{
    for (const DungeonRunSchool &candidate : schools)
    {
        if (candidate.skill == skill)
        {
            return &candidate;
        }
    }

    return nullptr;
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

            if (dungeonNode["tier"])
            {
                stop.tier = dungeonNode["tier"].as<int>();
            }

            if (dungeonNode["groupScale"])
            {
                stop.groupScale = std::clamp(dungeonNode["groupScale"].as<float>(), 0.1f, 4.0f);
            }

            definition.dungeons.push_back(std::move(stop));
        }

        const YAML::Node firstNode = root["first"];

        if (firstNode)
        {
            const std::string firstMap = firstNode.IsScalar() ? lowerCaseCopy(firstNode.as<std::string>()) : "";

            for (size_t index = 0; index < definition.dungeons.size(); ++index)
            {
                if (lowerCaseCopy(definition.dungeons[index].mapFileName) == firstMap)
                {
                    definition.firstDungeon = index;
                }
            }

            if (!definition.firstDungeon.has_value())
            {
                errorMessage = "run.yml first must name one of its dungeons";
                return std::nullopt;
            }
        }

        if (const YAML::Node rulesNode = root["rules"])
        {
            if (!rulesNode.IsMap() || !parseRunRules(rulesNode, definition, errorMessage))
            {
                if (errorMessage.empty())
                {
                    errorMessage = "run.yml rules must be a map";
                }

                return std::nullopt;
            }
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

std::vector<size_t> dungeonRunOrder(const DungeonRunDefinition &run, uint64_t rootSeed)
{
    std::vector<size_t> order = dungeonRunOrder(run.dungeons.size(), rootSeed);

    if (run.rules.has_value())
    {
        std::vector<size_t> route;

        for (int tier : run.rules->route)
        {
            auto pick = std::find_if(
                order.begin(),
                order.end(),
                [&run, tier](size_t index)
                {
                    return run.dungeons[index].tier == tier;
                });

            if (pick == order.end())
            {
                pick = order.begin();
            }

            route.push_back(*pick);
            order.erase(pick);
        }

        return route;
    }

    if (run.firstDungeon.has_value() && *run.firstDungeon < order.size())
    {
        // Taking one index out leaves the others in a uniformly random order.
        std::erase(order, *run.firstDungeon);
        order.insert(order.begin(), *run.firstDungeon);
    }

    return order;
}

const DungeonRunStop *dungeonRunStop(const DungeonRunDefinition &run, uint64_t rootSeed, size_t entered)
{
    const std::vector<size_t> order = dungeonRunOrder(run, rootSeed);

    if (entered >= order.size())
    {
        return nullptr;
    }

    return &run.dungeons[order[entered]];
}

std::string dungeonRunEndText(bool defeated, size_t completedDungeons)
{
    return std::string(defeated ? "Run over." : "Run complete!")
        + " Dungeons cleared: " + std::to_string(completedDungeons) + ".";
}
}
