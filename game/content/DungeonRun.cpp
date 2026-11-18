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

// "normal", "expert", "master" or "grandmaster"; None for anything else.
SkillMastery masteryByName(const std::string &name)
{
    const std::string key = lowerCaseCopy(name);
    return key == "normal"       ? SkillMastery::Normal
        : key == "expert"        ? SkillMastery::Expert
        : key == "master"        ? SkillMastery::Master
        : key == "grandmaster"   ? SkillMastery::Grandmaster
                                 : SkillMastery::None;
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

    // Every stage of the route has a dungeon of its own: a route drawn without repeats never runs dry.
    for (size_t stop = 0; stop < rules.route.size(); ++stop)
    {
        const int stage = rules.route[stop];
        const size_t stops = size_t(std::count(rules.route.begin(), rules.route.end(), stage));
        const size_t dungeons = size_t(std::count_if(definition.dungeons.begin(), definition.dungeons.end(),
            [stage](const DungeonRunStop &dungeon) { return dungeonRunStopAtStage(dungeon, stage); }));

        if (dungeons < stops)
        {
            errorMessage = "run.yml route stage " + std::to_string(stage) + " has too few dungeons";
            return false;
        }
    }

    if (const YAML::Node node = rulesNode["clear"])
    {
        rules.clearShare = std::clamp(node["share"].as<double>(rules.clearShare), 0.01, 1.0);
        rules.guardianHealthScale = std::max(0.1f, node["guardianHealthScale"].as<float>(rules.guardianHealthScale));
        rules.uniqueGuardianHealthScale =
            std::max(0.0f, node["uniqueGuardianHealthScale"].as<float>(rules.uniqueGuardianHealthScale));
    }

    if (const YAML::Node node = rulesNode["levels"])
    {
        rules.levelUps = node["perStop"].as<std::vector<int>>(std::vector<int>());
        rules.levelsPerLevelUp = std::max(1, node["levelsEach"].as<int>(rules.levelsPerLevelUp));
        rules.cardOffers = std::clamp(node["offers"].as<int>(rules.cardOffers), 1, 5);
        rules.levelUpHealShare = std::clamp(node["heal"].as<double>(rules.levelUpHealShare), 0.0, 1.0);
    }

    if (const YAML::Node node = rulesNode["camp"])
    {
        rules.campRestFood = std::max(0, node["restFood"].as<int>(rules.campRestFood));
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

        for (const YAML::Node &stackNode : node["stock"])
        {
            DungeonRunShopStack stack = {};
            stack.itemId = stackNode["item"].as<uint32_t>(0);
            stack.count = std::max<uint32_t>(1, stackNode["count"].as<uint32_t>(1));

            if (stack.itemId == 0)
            {
                errorMessage = "run.yml shop '" + shop.name + "': every stock line needs an item";
                return false;
            }

            shop.stock.push_back(stack);
        }

        rules.shops.push_back(std::move(shop));
    }

    if (const YAML::Node rewardsNode = rulesNode["rewards"])
    {
        for (const YAML::Node &node : rewardsNode["guardians"])
        {
            DungeonRunGuardianReward reward = {};
            reward.gold = std::max(0, node["gold"].as<int>(0));
            reward.item = lowerCaseCopy(node["item"].as<std::string>(""));

            if (!reward.item.empty() && reward.item != "artifact" && reward.item != "relic")
            {
                errorMessage = "run.yml guardian reward item must be artifact or relic";
                return false;
            }

            rules.guardianRewards.push_back(std::move(reward));
        }
    }

    if (const YAML::Node iconsNode = rulesNode["icons"])
    {
        for (const auto &entry : iconsNode)
        {
            rules.skillIcons[canonicalSkillName(entry.first.as<std::string>())] = entry.second.as<std::string>();
        }
    }

    if (const YAML::Node node = rulesNode["unrestrictedCharacters"])
    {
        rules.unrestrictedCharacters = node.as<std::vector<uint32_t>>();
    }

    std::unordered_set<std::string> seenTalents;

    for (const YAML::Node &node : rulesNode["talents"])
    {
        DungeonRunTalent talent = {};
        talent.id = node["id"].as<std::string>("");
        talent.name = node["name"].as<std::string>(talent.id);
        talent.text = node["text"].as<std::string>("");
        talent.icon = node["icon"].as<std::string>("");
        talent.stat = node["stat"].as<std::string>("");
        talent.amount = node["amount"].as<int>(0);
        talent.maxRank = std::max(1, node["maxRank"].as<int>(1));
        const std::string needs = node["needs"].as<std::string>("");
        const std::string group = lowerCaseCopy(needs);
        talent.needsSkill = group == "melee" || group == "magic" ? group : canonicalSkillName(needs);
        talent.needsMastery = masteryByName(node["mastery"].as<std::string>("normal"));
        talent.values = node["values"].as<std::vector<float>>(std::vector<float>());
        talent.values2 = node["values2"].as<std::vector<float>>(std::vector<float>());
        talent.share = node["share"].as<float>(0.0f);
        talent.seconds = node["seconds"].as<float>(0.0f);
        talent.cooldown = node["cooldown"].as<float>(0.0f);
        talent.radius = node["radius"].as<float>(0.0f);
        talent.arc = node["arc"].as<float>(0.0f);
        talent.targets = std::max(0, node["targets"].as<int>(0));
        talent.spells = node["spells"].as<std::vector<uint32_t>>(std::vector<uint32_t>());

        if (talent.id.empty() || !seenTalents.insert(talent.id).second)
        {
            errorMessage = "run.yml talent '" + talent.id + "' needs an id of its own";
            return false;
        }

        const bool knownNeed = needs.empty() || group == "melee" || group == "magic"
            || skillIdForSkillName(talent.needsSkill).has_value();

        if (talent.needsMastery == SkillMastery::None || !knownNeed)
        {
            errorMessage = "run.yml talent '" + talent.id + "' needs a known skill and mastery";
            return false;
        }

        rules.talents.push_back(std::move(talent));
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

float DungeonRunTalent::value(int rank) const
{
    return rank <= 0 || values.empty() ? 0.0f : values[std::min(static_cast<size_t>(rank), values.size()) - 1];
}

float DungeonRunTalent::value2(int rank) const
{
    return rank <= 0 || values2.empty() ? 0.0f : values2[std::min(static_cast<size_t>(rank), values2.size()) - 1];
}

const DungeonRunTalent *DungeonRunRules::talent(std::string_view id) const
{
    for (const DungeonRunTalent &candidate : talents)
    {
        if (candidate.id == id)
        {
            return &candidate;
        }
    }

    return nullptr;
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
                stop.stages = {stop.tier};
            }

            if (const YAML::Node stagesNode = dungeonNode["stages"])
            {
                stop.stages = stagesNode.as<std::vector<int>>();

                if (stop.stages.empty())
                {
                    errorMessage = "run.yml dungeon '" + stop.mapFileName + "' needs at least one stage";
                    return std::nullopt;
                }

                stop.tier = stop.stages.front();
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

bool dungeonRunStopAtStage(const DungeonRunStop &stop, int stage)
{
    return stop.stages.empty() ? stop.tier == stage
                               : std::find(stop.stages.begin(), stop.stages.end(), stage) != stop.stages.end();
}

int32_t dungeonRunMapKey(std::string_view mapFileName)
{
    // FNV-1a over the lower-case name: the same on every platform and build.
    uint32_t hash = 2166136261u;

    for (const char character : mapFileName)
    {
        hash ^= static_cast<uint8_t>(std::tolower(static_cast<unsigned char>(character)));
        hash *= 16777619u;
    }

    return static_cast<int32_t>(hash & 0x7fffffffu);
}

std::optional<size_t> dungeonRunIndexOfKey(const DungeonRunDefinition &run, int32_t key)
{
    for (size_t index = 0; index < run.dungeons.size(); ++index)
    {
        if (dungeonRunMapKey(run.dungeons[index].mapFileName) == key)
        {
            return index;
        }
    }

    return std::nullopt;
}

std::vector<size_t> drawDungeonRunRoute(
    const DungeonRunDefinition &run, uint64_t rootSeed, const std::vector<std::optional<size_t>> &fixed)
{
    if (!run.rules.has_value())
    {
        return {};
    }

    std::vector<size_t> order = dungeonRunOrder(run.dungeons.size(), rootSeed);
    const std::vector<int> &stages = run.rules->route;
    std::vector<std::optional<size_t>> route(stages.size());

    // The fixed stops first, so that no drawn stop takes their dungeons.
    for (size_t stop = 0; stop < route.size() && stop < fixed.size(); ++stop)
    {
        if (fixed[stop].has_value() && *fixed[stop] < run.dungeons.size())
        {
            route[stop] = fixed[stop];
            std::erase(order, *fixed[stop]);
        }
    }

    for (size_t stop = 0; stop < route.size(); ++stop)
    {
        if (route[stop].has_value())
        {
            continue;
        }

        const int stage = stages[stop];
        auto pick = std::find_if(
            order.begin(),
            order.end(),
            [&run, stage](size_t index)
            {
                return dungeonRunStopAtStage(run.dungeons[index], stage);
            });

        // A stage with no dungeon left (a run.yml the loader let through): any dungeon, as the first runs did.
        if (pick == order.end())
        {
            pick = order.begin();
        }

        if (pick == order.end())
        {
            break;
        }

        route[stop] = *pick;
        order.erase(pick);
    }

    std::vector<size_t> drawn;

    for (const std::optional<size_t> &index : route)
    {
        if (index.has_value())
        {
            drawn.push_back(*index);
        }
    }

    return drawn;
}

std::vector<std::string> legacyDungeonRunMaps(uint64_t rootSeed)
{
    // run.yml of OpenYAMM 2.3 and 2.4 (rules v0, v1): its dungeons in their order and tiers.
    struct LegacyStop
    {
        const char *pMap;
        int tier;
    };

    static constexpr LegacyStop LegacyPool[] = {
        {"6d01.blv", 1}, {"6d02.blv", 2}, {"6t3.blv", 2}, {"6t2.blv", 2},
        {"6d07.blv", 3}, {"6d15.blv", 3}, {"6d13.blv", 3}, {"6d12.blv", 3},
    };
    static constexpr int LegacyRoute[] = {1, 2, 3, 3};

    std::vector<size_t> order = dungeonRunOrder(std::size(LegacyPool), rootSeed);
    std::vector<std::string> maps;

    for (int tier : LegacyRoute)
    {
        auto pick = std::find_if(
            order.begin(), order.end(), [tier](size_t index) { return LegacyPool[index].tier == tier; });

        if (pick == order.end())
        {
            pick = order.begin();
        }

        maps.emplace_back(LegacyPool[*pick].pMap);
        order.erase(pick);
    }

    return maps;
}

std::vector<size_t> dungeonRunOrder(const DungeonRunDefinition &run, uint64_t rootSeed)
{
    std::vector<size_t> order = dungeonRunOrder(run.dungeons.size(), rootSeed);

    if (run.rules.has_value())
    {
        return drawDungeonRunRoute(run, rootSeed);
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
