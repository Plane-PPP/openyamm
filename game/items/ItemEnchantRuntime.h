#pragma once

#include "game/party/CharacterState.h"
#include "game/items/ItemEnchantTables.h"
#include "game/party/Party.h"

#include <cstdint>
#include <optional>
#include <random>
#include <string>

namespace OpenYAMM::Game
{
struct Character;
class ItemTable;
struct ItemDefinition;
struct ItemContentEffect;
enum class CharacterAttackMode;
struct ElementalDamageBonuses;

enum class ItemEnchantCategory : uint8_t
{
    None = 0,
    OneHandedWeapon,
    TwoHandedWeapon,
    Missile,
    Armor,
    Shield,
    Helm,
    Belt,
    Cloak,
    Gauntlets,
    Boots,
    Ring,
    Amulet,
};

class ItemEnchantRuntime
{
public:
    static bool isEnchantable(const ItemDefinition &itemDefinition);
    static bool canHaveStandardEnchant(const ItemDefinition &itemDefinition);
    static bool canHaveSpecialEnchant(const ItemDefinition &itemDefinition);
    static ItemEnchantCategory categoryForItem(const ItemDefinition &itemDefinition);
    static int standardEnchantChance(int treasureLevel);
    static int specialEnchantChance(const ItemDefinition &itemDefinition, int treasureLevel);
    static bool isSpecialRarityAllowed(char rarityLevel, int treasureLevel);
    // worldId: the world the item rolls in; a world's own enchantments roll only there.
    static std::optional<uint16_t> chooseStandardEnchantId(
        const ItemDefinition &itemDefinition,
        const StandardItemEnchantTable &table,
        std::mt19937 &rng,
        const std::string &worldId = {});
    static std::optional<uint16_t> chooseSpecialEnchantId(
        const ItemDefinition &itemDefinition,
        const SpecialItemEnchantTable &table,
        int treasureLevel,
        std::mt19937 &rng,
        const std::string &worldId = {});
    static int generateStandardEnchantPower(StandardItemEnchantKind kind, int treasureLevel, std::mt19937 &rng);
    static std::string displayName(
        const InventoryItem &item,
        const ItemDefinition &itemDefinition,
        const StandardItemEnchantTable *pStandardTable,
        const SpecialItemEnchantTable *pSpecialTable);
    static int itemValue(
        const InventoryItem &item,
        const ItemDefinition &itemDefinition,
        const StandardItemEnchantTable *pStandardTable,
        const SpecialItemEnchantTable *pSpecialTable);
    static int itemInspectValue(
        const InventoryItem &item,
        const ItemDefinition &itemDefinition,
        const StandardItemEnchantTable *pStandardTable,
        const SpecialItemEnchantTable *pSpecialTable);
    // Standard MMMerge's value: base value, +100 per standard enchant point, the special's price modifier.
    static int standardItemValue(
        const InventoryItem &item,
        const ItemDefinition &itemDefinition,
        const StandardItemEnchantTable *pStandardTable,
        const SpecialItemEnchantTable *pSpecialTable,
        int minimumValue);
    // Adds what one special enchantment of `kind` gives `member` in Standard (stats, skills, immunities, flags).
    static void applySpecialEnchantBonus(Character &member, SpecialItemEnchantKind kind);
    // Adds one standard enchantment's bonus of `amount` to `member`.
    static void applyStandardEnchantBonus(Character &member, StandardItemEnchantKind kind, int amount);
    static std::string buildEnchantDescription(
        const InventoryItem &item,
        const StandardItemEnchantTable *pStandardTable,
        const SpecialItemEnchantTable *pSpecialTable);
    static void applyEquippedEnchantEffects(
        const ItemDefinition &itemDefinition,
        const EquippedItemRuntimeState &runtimeState,
        const StandardItemEnchantTable *pStandardTable,
        const SpecialItemEnchantTable *pSpecialTable,
        Character &member);
    static void applyContentEffect(const ItemContentEffect &effect, Character &member);
    static int characterAttackDamageMultiplierAgainstMonster(
        const Character &character,
        CharacterAttackMode attackMode,
        const ItemTable *pItemTable,
        const SpecialItemEnchantTable *pSpecialTable,
        uint32_t monsterKindFlags = 0);
    static int elementalDamageBonus(
        const InventoryItem &item,
        const SpecialItemEnchantTable *pSpecialTable);
    static ElementalDamageBonuses characterAttackElementalDamageBonuses(
        const Character &character,
        CharacterAttackMode attackMode,
        const ItemTable *pItemTable,
        const SpecialItemEnchantTable *pSpecialTable);
};
}
