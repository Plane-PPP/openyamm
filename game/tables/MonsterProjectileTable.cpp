#include "game/tables/MonsterProjectileTable.h"

#include <yaml-cpp/yaml.h>

#include <cctype>

namespace OpenYAMM::Game
{
namespace
{
std::string normalizeToken(std::string value)
{
    for (char &character : value)
    {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }

    return value;
}

bool isNumericString(const std::string &value)
{
    if (value.empty())
    {
        return false;
    }

    for (char character : value)
    {
        if (!std::isdigit(static_cast<unsigned char>(character)) && character != '-')
        {
            return false;
        }
    }

    return true;
}
}

bool MonsterProjectileTable::loadFromRows(const std::vector<std::vector<std::string>> &rows)
{
    m_entries.clear();
    m_entryIndexByNormalizedToken.clear();

    for (const std::vector<std::string> &row : rows)
    {
        if (row.size() < 2 || row[0].empty() || !isNumericString(row[1]))
        {
            continue;
        }

        MonsterProjectileEntry entry = {};
        entry.token = row[0];
        entry.normalizedToken = normalizeToken(entry.token);
        entry.objectId = std::stoi(row[1]);
        entry.impactObjectId = row.size() > 2 && isNumericString(row[2]) ? std::stoi(row[2]) : 0;
        m_entryIndexByNormalizedToken[entry.normalizedToken] = m_entries.size();
        m_entries.push_back(std::move(entry));
    }

    return !m_entries.empty();
}

bool MonsterProjectileTable::loadWorldYaml(const std::string &text, const std::string &packageId,
    uint32_t objectBegin, uint32_t objectEnd, std::string &error)
{
    const std::string where = "worlds/" + packageId + "/monster_projectiles.yml: ";
    try
    {
        const YAML::Node root = YAML::Load(text);
        if (!root.IsMap() || root["schema"].as<std::string>("") != "openyamm.monsterProjectiles.v1"
            || !root["missiles"] || !root["missiles"].IsMap())
        {
            error = where + "expected schema openyamm.monsterProjectiles.v1 with a missiles map";
            return false;
        }

        // Parsed whole before any is added: a broken file adds nothing.
        std::vector<MonsterProjectileEntry> added;
        std::unordered_map<std::string, size_t> addedIndex;

        for (const auto &item : root["missiles"])
        {
            MonsterProjectileEntry entry = {};
            entry.token = item.first.as<std::string>();
            entry.normalizedToken = normalizeToken(entry.token);
            const YAML::Node &node = item.second;
            const std::string context = where + entry.token + ": ";

            if (entry.token.empty() || isNumericString(entry.token) || entry.normalizedToken == "0"
                || m_entryIndexByNormalizedToken.contains(entry.normalizedToken)
                || addedIndex.contains(entry.normalizedToken))
            {
                error = context + "a missile token must be a new name";
                return false;
            }

            for (const auto &field : node)
            {
                const std::string key = field.first.as<std::string>();
                if (key != "object" && key != "impact_object" && key != "tier")
                {
                    error = context + "unknown key " + key;
                    return false;
                }
            }

            entry.objectId = node["object"].as<int>(0);
            entry.impactObjectId = node["impact_object"].as<int>(0);

            if (entry.objectId < int64_t(objectBegin) || entry.objectId > int64_t(objectEnd))
            {
                error = context + "object " + std::to_string(entry.objectId) + " is outside the world's objects range";
                return false;
            }

            if (entry.impactObjectId < 0)
            {
                error = context + "impact_object must be an object id or 0";
                return false;
            }

            if (const YAML::Node tier = node["tier"])
            {
                entry.shieldTier = parseShieldAttackTier(tier.as<std::string>());
                if (!entry.shieldTier)
                {
                    error = context + "tier must be physical, classic_blaster, kreegan_energy or spell";
                    return false;
                }
            }

            addedIndex[entry.normalizedToken] = added.size();
            added.push_back(std::move(entry));
        }

        for (MonsterProjectileEntry &entry : added)
        {
            m_entryIndexByNormalizedToken[entry.normalizedToken] = m_entries.size();
            m_entries.push_back(std::move(entry));
        }
    }
    catch (const YAML::Exception &exception)
    {
        error = where + exception.what();
        return false;
    }

    return true;
}

const MonsterProjectileEntry *MonsterProjectileTable::findByToken(const std::string &token) const
{
    const auto entryIt = m_entryIndexByNormalizedToken.find(normalizeToken(token));

    if (entryIt == m_entryIndexByNormalizedToken.end())
    {
        return nullptr;
    }

    return &m_entries[entryIt->second];
}
}
