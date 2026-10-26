#pragma once

#include "game/party/Party.h"
#include "game/tables/ReadableScrollTable.h"
#include "game/party/SkillData.h"
#include "game/party/SpeechIds.h"
#include "game/audio/SoundIds.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace OpenYAMM::Game
{
class ItemTable;
class MergedPotionSettingTable;
class MergedReagentSettingTable;

enum class InventoryItemUseAction : uint8_t
{
    None = 0,
    Equip,
    CastScroll,
    LearnSpell,
    LearnSkill,
    ConsumePotion,
    ReadMessageScroll,
    UseHorseshoe,
    UseGenieLamp,
    UseDeckOfFate,
    EatFoodItem,
    PlayInstrument,
    UseTempleInABottle,
    UseReagent,
    UseDimensionDoorScroll,
    UseConnectorStone,
    // An item whose use action is grantTechPoints (Hesper's Engrams): its VarA in tech points to the target.
    GrantTechPoints,
    // Personal energy shields (use actions useShieldCell, useShieldBurst; Party::chargePersonalShield and
    // burstPersonalShields).
    UseShieldCell,
    UseShieldBurst,
    // throwEmpGrenade: consumed here; the caller applies it to the world (GameplayItemService).
    ThrowEmpGrenade,
};

struct InventoryItemUseContext
{
    bool underwater = false;
    float gameMinutes = 0.0f;
    // Hostile monsters near the party (the yellow alert): drinking may then cost recovery.
    bool enemiesNear = false;
};

struct InventoryItemUseResult
{
    bool handled = false;
    bool consumed = false;
    InventoryItemUseAction action = InventoryItemUseAction::None;
    bool alreadyKnown = false;
    std::vector<std::string> learnedSkills;
    uint32_t spellId = 0;
    uint32_t spellSkillLevelOverride = 0;
    SkillMastery spellSkillMasteryOverride = SkillMastery::None;
    bool spellBypassRequiredMastery = false;
    std::string statusText;
    std::optional<SpeechId> speechId;
    std::optional<SoundId> soundId;
    std::string readableTitle;
    std::string readableBody;
    // A potion of several drinks that is not used up: the uses its bottle has left (its new charges).
    std::optional<uint32_t> potionUsesLeft;
};

class InventoryItemUseRuntime
{
public:
    static InventoryItemUseAction classifyItemUse(
        const InventoryItem &item,
        const ItemTable &itemTable,
        const MergedPotionSettingTable *pPotionSettingTable = nullptr,
        const MergedReagentSettingTable *pReagentSettingTable = nullptr);

    static InventoryItemUseResult useItemOnMember(
        Party &party,
        size_t targetMemberIndex,
        const InventoryItem &item,
        const ItemTable &itemTable,
        const ReadableScrollTable *pReadableScrollTable,
        const MergedPotionSettingTable *pPotionSettingTable = nullptr,
        const MergedReagentSettingTable *pReagentSettingTable = nullptr,
        const InventoryItemUseContext &context = {});
    // Whether a drink costs a ready member recovery under the rules' potion effects (MAW). Standard: no.
    static bool drinkRecoveryApplies(const MergedPotionSettingTable *pPotionSettingTable);
    // Who pays a drink's recovery: the drinker if ready, else the active member, else the first ready one.
    static std::optional<size_t> drinkRecoveryPayer(const Party &party, size_t drinkerIndex);
};
}
