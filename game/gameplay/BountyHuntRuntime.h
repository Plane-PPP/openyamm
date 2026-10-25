#pragma once

#include "game/rules/ActorRulesetFacts.h"
#include "game/tables/MonsterTable.h"
#include "game/tables/MergedBaseTables.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace OpenYAMM::Game
{
struct BountyHuntEntry
{
    uint32_t month = 0;
    int16_t monsterId = 0;
    bool done = false;
    bool claimed = false;
    // The affix of the boss the rules spawned (MAW), which the bounty texts name; 0 for a plain target.
    uint8_t bossAffix = 0;
};

struct BountyHuntClaimResult
{
    bool claimed = false;
    uint32_t goldReward = 0;
    int32_t reputationDelta = 0;
    uint32_t bountyTotalDelta = 0;
};

struct BountyHuntInteractionResult
{
    bool succeeded = false;
    std::vector<std::string> messages;
};

class IGameplayWorldRuntime;
class NpcDialogTable;
class Party;

uint32_t bountyHuntRewardForMonster(const MonsterTable::MonsterStatsEntry &stats);
bool bountyHuntMonsterAllowed(
    const MonsterTable::MonsterStatsEntry &stats,
    const MergedBolsterMonsterTable *pBolsterMonsterTable,
    uint32_t maximumLevel);
std::vector<int16_t> collectBountyHuntMonsterIds(
    const MonsterTable &monsterTable,
    const MergedBolsterMonsterTable *pBolsterMonsterTable,
    uint32_t maximumLevel);
std::optional<int16_t> chooseBountyHuntMonsterId(
    const MonsterTable &monsterTable,
    const MergedBolsterMonsterTable *pBolsterMonsterTable,
    uint32_t maximumLevel,
    uint32_t seed);
bool bountyHuntEntryExpired(const BountyHuntEntry &entry, uint32_t currentMonth);
bool markBountyHuntMonsterKilled(BountyHuntEntry &entry, int16_t monsterId, uint32_t currentMonth);
BountyHuntClaimResult claimBountyHuntReward(
    BountyHuntEntry &entry,
    const MonsterTable &monsterTable,
    uint32_t currentMonth,
    bool awardGold);
bool markRuntimeBountyHuntMonsterKilled(IGameplayWorldRuntime &worldRuntime, int16_t monsterId);
// The rules may require more of the kill than the monster's id (MAW: the boss) and that a party member struck the
// killing blow (MAW: any other death of the target drops the bounty).
bool markRuntimeBountyHuntMonsterKilled(
    IGameplayWorldRuntime &worldRuntime,
    int16_t monsterId,
    const MonsterTable *pMonsterTable,
    const ActorRulesetFacts &facts = {},
    bool killedByParty = true);
BountyHuntInteractionResult performBountyHuntInteraction(
    IGameplayWorldRuntime &worldRuntime,
    Party *pParty,
    bool awardGold,
    const NpcDialogTable *pNpcDialogTable = nullptr);
std::string bountyHuntTargetText(
    const MonsterTable::MonsterStatsEntry &stats,
    const NpcDialogTable *pNpcDialogTable = nullptr);
std::string bountyHuntRewardText(
    const MonsterTable::MonsterStatsEntry &stats,
    const NpcDialogTable *pNpcDialogTable = nullptr);
std::string bountyHuntClaimedText(const NpcDialogTable *pNpcDialogTable = nullptr);
void applyBountyHuntClaimResult(
    IGameplayWorldRuntime &worldRuntime,
    Party *pParty,
    const BountyHuntClaimResult &result);
}
