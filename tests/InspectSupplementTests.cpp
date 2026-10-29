#include "doctest/doctest.h"
#include "game/gameplay/GameMechanics.h"
#include "game/rules/SkillRules.h"
#include "game/rules/maw/MawRules.h"
#include "game/ui/GameplayHudCommon.h"
#include "tests/RegressionGameData.h"
#include <cmath>

using namespace OpenYAMM::Game;
namespace
{
const OpenYAMM::Tests::RegressionGameData &data()
{
    REQUIRE_MESSAGE(OpenYAMM::Tests::regressionGameDataLoaded(), OpenYAMM::Tests::regressionGameDataFailure());
    return OpenYAMM::Tests::regressionGameData();
}
Party inspectionParty()
{
    Party party;
    party.setItemTable(&data().itemTable);
    party.setClassMultiplierTable(&data().classMultiplierTable);
    Character c;
    c.name = "Inspector"; c.className = "MinotaurLord"; c.raceId = 3; c.level = 50;
    c.might = 90; c.speed = 100; c.accuracy = 50; c.luck = 90;
    c.intellect = c.personality = c.endurance = 25;
    c.skills["Axe"] = {"Axe", 10, SkillMastery::Grandmaster};
    c.skills["Mace"] = {"Mace", 10, SkillMastery::Master};
    c.skills["PlateArmor"] = {"PlateArmor", 10, SkillMastery::Normal};
    PartySeed seed; seed.members.push_back(c); party.seed(seed);
    return party;
}
uint32_t item(const std::string &skill, const std::string &slot)
{
    for (const auto &entry : data().itemTable.entries())
        if (entry.itemId > 0 && entry.itemId < 500 && entry.equipStat == slot
            && canonicalSkillName(entry.skillGroup) == skill) return entry.itemId;
    FAIL("missing inspection fixture"); return 0;
}
}
TEST_CASE("Inspect attributes use engine thresholds and preserve original descriptions")
{
    Party party = inspectionParty();
    const auto tiers = GameMechanics::primaryStatThresholds();
    REQUIRE(tiers.size() == 29);
    for (size_t i = 0; i < tiers.size(); ++i)
    {
        CHECK(GameMechanics::primaryStatModifier(tiers[i].first) == tiers[i].second);
        if (i) CHECK(GameMechanics::primaryStatModifier(tiers[i].first - 1) == tiers[i-1].second);
    }
    SkillRules rules;
    for (const std::string stat : {"Might", "Intellect", "Personality", "Endurance", "Accuracy", "Speed", "Luck"})
    {
        const auto text = rules.statTooltip(party, *party.member(0), stat, "Original description");
        CHECK(text.find("Original description\n\n") == 0);
        CHECK(text.find("Thresholds:") != std::string::npos);
        CHECK(text.find("Next threshold:") != std::string::npos);
        CHECK(text.find("500:") != std::string::npos);
        CHECK(rules.statTooltip(party, *party.member(0), stat, "Original", true).find("Текущее значение") != std::string::npos);
    }
    CHECK(rules.statTooltip(party, *party.member(0), "Condition", "Unchanged") == "Unchanged");
    CHECK(GameMechanics::buildPrimaryStatInspectSupplement(party, *party.member(0), "Speed").find("Current value: 100") != std::string::npos);
    CHECK(GameMechanics::buildPrimaryStatInspectSupplement(party, *party.member(0), "Accuracy").find("Current value: 50") != std::string::npos);
    CHECK(GameMechanics::buildPrimaryStatInspectSupplement(party, *party.member(0), "Might").find(">75:+10") != std::string::npos);
}
TEST_CASE("Inspect long descriptions stay inside bounded viewports")
{
    for (const float height : {240.f, 480.f, 720.f, 1080.f})
    {
        const int rows = GameplayHudCommon::fitInspectBodyLines(height, 70, 15, 100);
        CHECK(rows >= 0); CHECK(rows * 15 + 70 <= height);
        CHECK(GameplayHudCommon::fitInspectBodyLines(height, 70, 15, 2) == 2);
    }
    CHECK(GameplayHudCommon::fitInspectBodyLines(20, 70, 15, 100) == 0);
}
TEST_CASE("Inspect weapon recovery equals combat for dual weapons mastery speed and armor")
{
    for (const auto mastery : {SkillMastery::Expert, SkillMastery::Master, SkillMastery::Grandmaster})
    for (const auto offhand : {uint32_t(0), item("Axe", "Weapon"), item("Mace", "Weapon")})
    for (const auto speed : {14u, 50u, 500u})
    {
        Party party = inspectionParty();
        auto *c = party.member(0);
        c->speed = speed; c->skills["Axe"].mastery = mastery;
        c->equipment.mainHand = item("Axe", "Weapon"); c->equipment.offHand = offhand;
        const int expected = int(std::lround(GameMechanics::buildCharacterAttackProfile(*c, &data().itemTable,
            &data().spellTable).meleeRecoverySeconds / ticksToRecoverySeconds(1)));
        CHECK(GameMechanics::attackRecoveryTicks(party, *c, CharacterAttackMode::Melee, &data().spellTable) == expected);
        InventoryItem axe; axe.objectDescriptionId = c->equipment.mainHand;
        CHECK(GameMechanics::buildItemRecoveryInspectSupplement(party, 0, axe, true, &data().spellTable)
            .find(": " + std::to_string(expected) + " ticks") != std::string::npos);
        if (offhand)
        {
            InventoryItem other; other.objectDescriptionId = offhand;
            CHECK(GameMechanics::buildItemRecoveryInspectSupplement(party, 0, other, true, &data().spellTable)
                .find(": " + std::to_string(expected) + " ticks") != std::string::npos);
        }
    }
}
TEST_CASE("Inspect equip preview is read only and armor penalty is isolated")
{
    Party party = inspectionParty();
    const Character before = *party.member(0);
    InventoryItem axe; axe.objectDescriptionId = item("Axe", "Weapon2");
    const auto text = GameMechanics::buildItemRecoveryInspectSupplement(party, 0, axe, false, &data().spellTable);
    CHECK(text.find("equip preview") != std::string::npos);
    CHECK(party.member(0)->equipment == before.equipment);
    CHECK(party.member(0)->inventory == before.inventory);
    InventoryItem armor; armor.objectDescriptionId = item("PlateArmor", "Armor");
    const auto *definition = data().itemTable.get(armor.objectDescriptionId);
    auto *c = party.member(0);
    const int penalty = GameMechanics::armorRecoveryPenaltyTicks(*c, *definition);
    CHECK(penalty > 0);
    c->speed = 500;
    CHECK(GameMechanics::armorRecoveryPenaltyTicks(*c, *definition) == penalty);
    c->skills["PlateArmor"].mastery = SkillMastery::Grandmaster;
    CHECK(GameMechanics::armorRecoveryPenaltyTicks(*c, *definition) == 0);
    CHECK(GameMechanics::buildItemRecoveryInspectSupplement(party, 0, armor, false, &data().spellTable).find("+0 ticks") != std::string::npos);
}
TEST_CASE("Inspect resource contributions follow class multipliers and current bonuses")
{
    Party party = inspectionParty();
    Character *c = party.member(0);
    for (const std::string className : {"Cleric", "Necromancer", "MinotaurLord", "Knight"})
    for (const std::string stat : {"Intellect", "Personality", "Endurance"})
    {
        c->className = className;
        Character actual = *c;
        actual.intellect = actual.personality = actual.endurance = 25;
        Character neutral = actual;
        if (stat == "Intellect") neutral.intellect = 13;
        if (stat == "Personality") neutral.personality = 13;
        if (stat == "Endurance") neutral.endurance = 13;
        const int expected = stat == "Endurance"
            ? GameMechanics::calculateBaseCharacterMaxHealth(actual, party.classMultiplierTable())
                - GameMechanics::calculateBaseCharacterMaxHealth(neutral, party.classMultiplierTable())
            : GameMechanics::calculateBaseCharacterMaxSpellPoints(actual, party.classMultiplierTable())
                - GameMechanics::calculateBaseCharacterMaxSpellPoints(neutral, party.classMultiplierTable());
        const std::string text = GameMechanics::buildPrimaryStatInspectSupplement(party, *c, stat);
        CHECK(text.find(std::string("zero modifier: ") + (expected >= 0 ? "+" : "") + std::to_string(expected)) != std::string::npos);
    }
    c->permanentBonuses.might = 10;
    c->magicalBonuses.might = 25;
    const auto text = GameMechanics::buildPrimaryStatInspectSupplement(party, *c, "Might");
    CHECK(text.find("Current value: 125") != std::string::npos);
    CHECK(text.find(">125:+12") != std::string::npos);
}
TEST_CASE("Inspect preview matches real equip and recovery includes haste and minimum")
{
    Party party = inspectionParty();
    InventoryItem axe; axe.objectDescriptionId = item("Axe", "Weapon2");
    auto preview = GameMechanics::buildItemRecoveryInspectSupplement(party, 0, axe, false, &data().spellTable);
    const auto plan = GameMechanics::resolveCharacterEquipPlan(*party.member(0), *data().itemTable.get(axe.objectDescriptionId),
        &data().itemTable, nullptr, std::nullopt, false);
    REQUIRE(plan);
    std::optional<InventoryItem> displaced;
    REQUIRE(party.tryEquipItemOnMember(0, plan->targetSlot, axe, plan->displacedSlot, plan->autoStoreDisplacedItem, displaced));
    const int ticks = GameMechanics::attackRecoveryTicks(party, *party.member(0), CharacterAttackMode::Melee, &data().spellTable);
    CHECK(preview.find(": " + std::to_string(ticks) + " ticks") != std::string::npos);
    party.member(0)->attackRecoveryReductionTicks = 1000;
    const int fast = GameMechanics::attackRecoveryTicks(party, *party.member(0), CharacterAttackMode::Melee, &data().spellTable);
    CHECK(fast == 30);
    CHECK(GameMechanics::buildItemRecoveryInspectSupplement(party, 0, axe, true, &data().spellTable).find(": 30 ticks") != std::string::npos);
}

TEST_CASE("Inspect MAW preserves its own stat text and uses its combat recovery override")
{
    MawRules rules(RulesetOptions{}, MawMapLevels{});
    Party party = inspectionParty();
    party.setRules(&rules.gameRules());
    party.member(0)->equipment.mainHand = item("Axe", "Weapon");
    const auto expected = rules.gameRules().combat.characterAttackRecoveryTicks(*party.member(0),
        CharacterAttackMode::Melee, data().itemTable, nullptr, nullptr);
    REQUIRE(expected);
    CHECK(GameMechanics::attackRecoveryTicks(party, *party.member(0), CharacterAttackMode::Melee,
        &data().spellTable) == *expected);
    const auto description = rules.gameRules().skills.statTooltip(party, *party.member(0), "Might", "Original");
    CHECK(description.find("Thresholds: value -> effect") == std::string::npos);
}

TEST_CASE("Inspect Android touch scrolling retains sub line motion and starts without jumping")
{
    float previous = 0, remainder = 0; bool dragging = false; int scroll = 0;
    GameplayHudCommon::dragInspectText(true, 100, 15, previous, remainder, dragging, scroll);
    CHECK(scroll == 0);
    GameplayHudCommon::dragInspectText(true, 90, 15, previous, remainder, dragging, scroll);
    CHECK(scroll == 0);
    GameplayHudCommon::dragInspectText(true, 80, 15, previous, remainder, dragging, scroll);
    CHECK(scroll == 1);
    GameplayHudCommon::dragInspectText(false, 0, 15, previous, remainder, dragging, scroll);
    GameplayHudCommon::dragInspectText(true, 200, 15, previous, remainder, dragging, scroll);
    CHECK(scroll == 1);
}
TEST_CASE("Inspect every armor mastery penalty matches its isolated combat contribution")
{
    for (const std::string skill : {"LeatherArmor", "ChainArmor", "PlateArmor", "Shield"})
    for (const auto mastery : {SkillMastery::None, SkillMastery::Normal, SkillMastery::Expert,
                              SkillMastery::Master, SkillMastery::Grandmaster})
    {
        Party party = inspectionParty();
        auto *c = party.member(0);
        c->speed = 14; c->skills["Axe"].mastery = SkillMastery::Normal;
        c->equipment.mainHand = item("Axe", "Weapon");
        c->skills[skill] = {skill, 10, mastery};
        const auto definition = data().itemTable.get(item(skill, skill == "Shield" ? "Shield" : "Armor"));
        REQUIRE(definition);
        const int before = GameMechanics::attackRecoveryTicks(party, *c, CharacterAttackMode::Melee, &data().spellTable);
        if (skill == "Shield") c->equipment.offHand = definition->itemId;
        else c->equipment.armor = definition->itemId;
        const int after = GameMechanics::attackRecoveryTicks(party, *c, CharacterAttackMode::Melee, &data().spellTable);
        CHECK(after - before == GameMechanics::armorRecoveryPenaltyTicks(*c, *definition));
    }
}
