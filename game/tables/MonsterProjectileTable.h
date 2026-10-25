#pragma once

#include "game/tables/MonsterShields.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace OpenYAMM::Game
{
struct MonsterProjectileEntry
{
    std::string token;
    std::string normalizedToken;
    int objectId = 0;
    int impactObjectId = 0;
    // What a hit is to an energy shield; unset follows the attack's damage type (projectileShieldTier). Only a world's
    // missiles (monster_projectiles.yml) set it.
    std::optional<ShieldAttackTier> shieldTier;
};

class MonsterProjectileTable
{
public:
    bool loadFromRows(const std::vector<std::vector<std::string>> &rows);
    // Appends a world's missiles: worlds/<world>/monster_projectiles.yml (schema openyamm.monsterProjectiles.v1),
    //   missiles: {<token>: {object: <id>, impact_object: <id or 0>, tier: <shield tier>}}
    // Each object id must lie in the world's objects range [objectBegin, objectEnd]; a token may not repeat one the
    // table already has (MM's tokens stay as they are).
    bool loadWorldYaml(const std::string &text, const std::string &packageId, uint32_t objectBegin,
        uint32_t objectEnd, std::string &error);
    const MonsterProjectileEntry *findByToken(const std::string &token) const;
    const std::vector<MonsterProjectileEntry> &entries() const
    {
        return m_entries;
    }

private:
    std::vector<MonsterProjectileEntry> m_entries;
    std::unordered_map<std::string, size_t> m_entryIndexByNormalizedToken;
};
}
