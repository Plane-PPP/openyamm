#include "game/rules/PotionRules.h"

#include "game/items/InventoryItemMixingRuntime.h"
#include "game/party/Party.h"
#include "game/tables/ItemTable.h"

#include <algorithm>

namespace OpenYAMM::Game
{
int PotionRules::reagentPower(const ItemDefinition &reagentDefinition, const InventoryItem &, SkillMastery, uint32_t) const
{
    return InventoryItemMixingRuntime::reagentPower(reagentDefinition);
}

double PotionRules::estimatedPool(PotionPool, int) const
{
    return 0;
}

uint32_t PotionRules::mixingAlchemyLevel(const Character &alchemist) const
{
    const CharacterSkill *pSkill = alchemist.findSkill("Alchemy");
    return pSkill != nullptr ? pSkill->level : 0;
}

int PotionRules::catalystMixPower(int targetPower, int heldPower, bool bothCatalysts, SkillMastery, uint32_t) const
{
    return bothCatalysts ? std::max(targetPower, heldPower) : heldPower;
}

std::optional<std::string> PotionRules::noBenefitText() const
{
    return std::nullopt;
}

const PotionRules &standardPotionRules()
{
    static const PotionRules Rules;
    return Rules;
}
}
