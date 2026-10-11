#include <doctest/doctest.h>

#include "game/app/GameSettings.h"
#include "game/debug/GameplayDebugTrace.h"
#include "game/gameplay/GameMechanics.h"
#include "game/gameplay/GameplayActionController.h"
#include "game/maps/SaveGame.h"
#include "game/party/SkillData.h"
#include "tests/RegressionGameData.h"
#include "tests/PartySpellTestHarness.h"

#include <cmath>
#include <chrono>
#include <fstream>
#include <iterator>

using namespace OpenYAMM::Game;

namespace
{
std::filesystem::path temporaryPath(const std::string &name)
{
    return std::filesystem::temp_directory_path()
        / (name + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
}

const OpenYAMM::Tests::RegressionGameData &data()
{
    REQUIRE_MESSAGE(OpenYAMM::Tests::regressionGameDataLoaded(), OpenYAMM::Tests::regressionGameDataFailure());
    return OpenYAMM::Tests::regressionGameData();
}

uint32_t weapon(const std::string &skill, const std::string &equip)
{
    for (const ItemDefinition &item : data().itemTable.entries())
    {
        if (canonicalSkillName(item.skillGroup) == skill && item.equipStat == equip
            && item.itemId > 0 && item.itemId < 500)
        {
            return item.itemId;
        }
    }
    FAIL("missing test weapon");
    return 0;
}

Character fighter(SkillMastery mastery = SkillMastery::Grandmaster, uint32_t level = 10)
{
    Character member;
    member.name = "Axe Tester";
    member.className = "MinotaurLord";
    member.raceId = 3;
    member.level = 50;
    member.might = member.speed = member.accuracy = member.luck = 14;
    member.skills["Axe"] = {"Axe", level, mastery};
    member.skills["Sword"] = {"Sword", 10, SkillMastery::Master};
    member.skills["Dagger"] = {"Dagger", 10, SkillMastery::Expert};
    return member;
}

CharacterAttackProfile profile(const Character &member)
{
    return GameMechanics::buildCharacterAttackProfile(member, &data().itemTable, &data().spellTable);
}

int ticks(float seconds)
{
    return int(std::lround(seconds * 60.0f));
}

class AxeTestWorld : public OpenYAMM::Tests::PartySpellTestWorldRuntime
{
public:
    int damage = 0;
    std::optional<GameplayPartyAttackActorFacts> partyAttackActorFacts(size_t index, bool) const override
    {
        return GameplayPartyAttackActorFacts{.actorIndex = index, .monsterId = 1, .displayName = "Test target",
            .position = {100, 0, 0}, .currentHp = 100000, .maxHp = 100000, .hostileToParty = true};
    }

    bool applyPartyAttackMeleeDamage(size_t, int value, const GameplayWorldPoint &) override
    {
        damage = value;
        return true;
    }
};
}

TEST_CASE("Axe Master off-hand follows weapon restrictions and save load")
{
    CHECK(data().classSkillTable.getEffectiveCap("Minotaur", 3, "Axe") == SkillMastery::Master);
    CHECK(data().classSkillTable.getEffectiveCap("MinotaurLord", 3, "Axe") == SkillMastery::Grandmaster);
    CHECK(data().classSkillTable.getEffectiveCap("MinotaurLord", 3, "Sword") == SkillMastery::Expert);
    const uint32_t axe = weapon("Axe", "Weapon");
    const uint32_t twoHanded = weapon("Axe", "Weapon2");
    for (const std::string skill : {"Axe", "Sword", "Dagger"})
    {
        for (bool axeInMain : {false, true})
        {
            Character member = fighter(SkillMastery::Master);
            const uint32_t other = weapon(skill, "Weapon");
            member.equipment.mainHand = axeInMain ? axe : other;
            const uint32_t off = axeInMain ? other : axe;
            const ItemDefinition *pOff = data().itemTable.get(off);
            REQUIRE(pOff != nullptr);
            const std::optional<CharacterEquipPlan> plan = GameMechanics::resolveCharacterEquipPlan(
                member, *pOff, &data().itemTable, nullptr, std::nullopt, true);
            REQUIRE(plan);
            CHECK(plan->targetSlot == EquipmentSlot::OffHand);
            Party party;
            party.setItemTable(&data().itemTable);
            PartySeed seed;
            seed.members.push_back(member);
            party.seed(seed);
            InventoryItem item;
            item.objectDescriptionId = off;
            std::optional<InventoryItem> replacement;
            REQUIRE(party.tryEquipItemOnMember(0, plan->targetSlot, item, plan->displacedSlot, true, replacement));
            CHECK_EQ(party.member(0)->equipment.offHand, off);
            const std::filesystem::path path = temporaryPath("axe-equip-test.oysav");
            GameSaveData save;
            save.mapFileName = "out01.odm";
            save.party = party.snapshot();
            std::string error;
            REQUIRE_MESSAGE(saveGameDataToPath(path, save, error), error);
            const std::optional<GameSaveData> restored = loadGameDataFromPath(path, error);
            REQUIRE_MESSAGE(restored, error);
            CHECK_EQ(restored->party.members[0].equipment.mainHand, member.equipment.mainHand);
            CHECK_EQ(restored->party.members[0].equipment.offHand, off);
            std::filesystem::remove(path);
        }
    }
    for (SkillMastery mastery : {SkillMastery::Normal, SkillMastery::Expert, SkillMastery::Master})
    {
        Character member = fighter(mastery);
        member.equipment.mainHand = axe;
        const std::optional<CharacterEquipPlan> plan = GameMechanics::resolveCharacterEquipPlan(
            member, *data().itemTable.get(axe), &data().itemTable, nullptr, std::nullopt, true);
        REQUIRE(plan);
        CHECK((plan->targetSlot == EquipmentSlot::OffHand) == (mastery == SkillMastery::Master));
        const std::optional<CharacterEquipPlan> twoPlan = GameMechanics::resolveCharacterEquipPlan(
            member, *data().itemTable.get(twoHanded), &data().itemTable, nullptr, std::nullopt, true);
        REQUIRE(twoPlan);
        CHECK(twoPlan->targetSlot != EquipmentSlot::OffHand);
        member.equipment.mainHand = twoHanded;
        const std::optional<CharacterEquipPlan> blocked = GameMechanics::resolveCharacterEquipPlan(
            member, *data().itemTable.get(axe), &data().itemTable, nullptr, std::nullopt, true);
        REQUIRE(blocked);
        CHECK(blocked->targetSlot != EquipmentSlot::OffHand);
    }
}

TEST_CASE("Axe GM one-handed recovery is once per melee action in either hand")
{
    const uint32_t axe = weapon("Axe", "Weapon");
    for (uint32_t level : {10u, 20u, 30u})
    {
        for (const std::string otherSkill : {"", "Axe", "Sword", "Dagger"})
        {
            for (bool axeInMain : {false, true})
            {
                if (otherSkill.empty() && !axeInMain)
                {
                    continue;
                }
                Character member = fighter(SkillMastery::Grandmaster, level);
                const uint32_t other = otherSkill.empty() ? 0 : weapon(otherSkill, "Weapon");
                member.equipment.mainHand = axeInMain ? axe : other;
                member.equipment.offHand = axeInMain ? other : axe;
                const CharacterAttackProfile gm = profile(member);
                CHECK_EQ(gm.axe.recoveryBonus, int(level));
                CHECK_EQ(gm.axe.recoveryBefore, 100 - int(level));
                CHECK_EQ(gm.axe.recoveryAfter, 100 - 2 * int(level));
                CHECK_EQ(ticks(gm.meleeRecoverySeconds), 100 - 2 * int(level));
                member.skills["Axe"].mastery = SkillMastery::Master;
                const CharacterAttackProfile master = profile(member);
                CHECK_EQ(master.axe.recoveryBonus, 0);
                CHECK_EQ(ticks(master.meleeRecoverySeconds), 100 - int(level));
                CHECK_EQ(gm.rangedRecoverySeconds, master.rangedRecoverySeconds);
            }
        }
    }
    Character member = fighter(SkillMastery::Grandmaster, 20);
    member.equipment.mainHand = axe;
    member.itemSkillBonuses["Axe"] = 3;
    CHECK_EQ(profile(member).axe.recoveryBonus, 23);
    member.equipment.mainHand = weapon("Axe", "Weapon2");
    CHECK_EQ(profile(member).axe.recoveryBonus, 0);
    member.equipment.mainHand = weapon("Sword", "Weapon");
    CHECK_EQ(profile(member).axe.recoveryBonus, 0);
    member.equipment.mainHand = axe;
    member.speed = 100;
    member.skills["Armsmaster"] = {"Armsmaster", 10, SkillMastery::Grandmaster};
    member.attackRecoveryReductionTicks = 25; // Existing Haste contribution.
    const CharacterAttackProfile fast = profile(member);
    CHECK_EQ(fast.axe.recoveryBonus, 23);
    CHECK_LT(fast.axe.recoveryAfter, 30);
    CHECK_EQ(fast.axe.recoveryFinal, 30);
    CHECK_EQ(ticks(fast.meleeRecoverySeconds), 30);
    member.skills["Axe"].mastery = SkillMastery::Master;
    CHECK_EQ(profile(member).axe.recoveryBefore, fast.axe.recoveryBefore);
}

TEST_CASE("Axe GM two-handed doubles complete physical damage only before target resistance")
{
    Character member = fighter();
    member.equipment.mainHand = weapon("Axe", "Weapon2");
    member.might = 100;
    member.skills["Armsmaster"] = {"Armsmaster", 12, SkillMastery::Grandmaster};
    member.weaponEnchantmentDamageBonus = 18; // Physical +9 and Fire +9.
    member.permanentBonuses.meleeDamage = 7;
    member.magicalBonuses.meleeDamage = 11;
    const CharacterAttackProfile attack = profile(member);
    CHECK_EQ(attack.axe.armsmasterBonus, 24);
    CHECK_EQ(attack.axe.weaponSkillBonus, 10);
    CHECK_EQ(attack.axe.enchantmentBonus, 18);
    CHECK_EQ(attack.axe.flatBonus, 18);
    CHECK_EQ(attack.meleeMinDamage, attack.axe.weaponMin + attack.axe.mightBonus + 24 + 10 + 18 + 18);
    ElementalDamageBonuses elemental;
    elemental.fire = 9;
    for (uint32_t seed = 0; seed < 128; ++seed)
    {
        std::mt19937 rolledRng(seed);
        const int rolled = std::uniform_int_distribution<int>(attack.meleeMinDamage, attack.meleeMaxDamage)(rolledRng);
        for (int resistance : {0, 50, 200})
        {
            MonsterDamageResistances defenses;
            defenses.physical = resistance;
            AxeAttackObservation observed;
            std::mt19937 rng(seed), reference(seed), unobserved(seed);
            const int expected = GameMechanics::resolveMonsterIncomingWeaponDamage(
                2 * (rolled - 9) + 9, CombatDamageType::Physical, elemental, defenses, 0, reference);
            const int actual = GameMechanics::resolveAxeMeleeWeaponDamage(member, &data().itemTable,
                rolled, CombatDamageType::Physical, elemental, defenses, 0, rng, &observed);
            CHECK_EQ(actual, expected);
            CHECK(rng == reference);
            CHECK_EQ(observed.damageMultiplier, 2);
            CHECK_EQ(observed.physicalBefore, rolled - 9);
            CHECK_EQ(observed.physicalAfter, 2 * (rolled - 9));
            CHECK_EQ(observed.elementalDamage, 9);
            CHECK_EQ(actual, observed.physicalFinal + 9);
            CHECK_EQ(GameMechanics::resolveAxeMeleeWeaponDamage(member, &data().itemTable,
                rolled, CombatDamageType::Physical, elemental, defenses, 0, unobserved), actual);
            CHECK(rng == unobserved);
        }
        member.equippedItemEffectFlags.insert("PerfectAccuracy");
        const CharacterAttackResult hit = GameMechanics::resolveCharacterAttackAgainstArmorClass(
            member, &data().itemTable, &data().spellTable, 1000, 100, rolledRng);
        REQUIRE(hit.hit);
        CHECK_FALSE(hit.halveTargetArmorClass);
    }
    for (bool twoHanded : {false, true})
    {
        for (SkillMastery mastery : {SkillMastery::Expert, SkillMastery::Master, SkillMastery::Grandmaster})
        {
            member.equipment.mainHand = weapon("Axe", twoHanded ? "Weapon2" : "Weapon");
            member.skills["Axe"].mastery = mastery;
            std::mt19937 rng(7);
            AxeAttackObservation observation;
            CHECK_EQ(GameMechanics::resolveAxeMeleeWeaponDamage(member, &data().itemTable,
                109, CombatDamageType::Physical, elemental, {}, 0, rng, &observation),
                twoHanded && mastery == SkillMastery::Grandmaster ? 209 : 109);
            CHECK_EQ(observation.damageMultiplier, twoHanded && mastery == SkillMastery::Grandmaster ? 2 : 1);
            rng.seed(7);
            CHECK_EQ(GameMechanics::resolveAxeMeleeWeaponDamage(member, &data().itemTable,
                109, CombatDamageType::Fire, elemental, {}, 0, rng, &observation), 109);
            CHECK_EQ(observation.damageMultiplier, 1);
        }
    }
}

TEST_CASE("Axe trace settings and logging remain independent of minotaur channel")
{
    GameSettings settings = GameSettings::createDefault();
    CHECK_FALSE(settings.axeDamageTrace);
    const std::filesystem::path ini = temporaryPath("axe-trace-settings.ini");
    const std::filesystem::path log = temporaryPath("axe-trace-test.log");
    for (bool enabled : {true, false})
    {
        settings.axeDamageTrace = enabled;
        settings.minotaurDamageTrace = true;
        std::string error;
        REQUIRE(saveGameSettings(ini, settings, error));
        const std::optional<GameSettings> loaded = loadGameSettings(ini, error);
        REQUIRE_MESSAGE(loaded, error);
        CHECK(loaded->axeDamageTrace == enabled);
        CHECK(loaded->minotaurDamageTrace);
        configureGameplayCombatTrace(false, log.string(), false, true, enabled);
        CHECK_FALSE(gameplayCombatTraceEnabled());
        CHECK(gameplayMinotaurDamageTraceEnabled());
        CHECK(gameplayAxeDamageTraceEnabled() == enabled);
    }
    configureGameplayCombatTrace(false, log.string(), true, false, true);
    CHECK_FALSE(gameplayMinotaurDamageTraceEnabled());
    CHECK(gameplayAxeDamageTraceEnabled());
    gameplayCombatTraceWrite("axe test");
    configureGameplayCombatTrace(false, log.string(), true);
    CHECK_FALSE(gameplayAxeDamageTraceEnabled());
    gameplayCombatTraceWrite("disabled");
    std::ifstream input(log);
    std::string line;
    REQUIRE(bool(std::getline(input, line)));
    CHECK_EQ(line, "[CombatTrace] axe test");
    CHECK_FALSE(bool(std::getline(input, line)));
    input.close();
    std::filesystem::remove(log);
    std::filesystem::remove(ini);
}

TEST_CASE("Axe traced live action preserves damage recovery and complete saved party state")
{
    const auto log = temporaryPath("axe-action-trace.log");
    const auto savePath = temporaryPath("axe-action-state.oysav");
    for (int setup : {0, 1, 2})
    {
        for (bool haste : {false, true})
        {
            for (uint32_t seed = 0; seed < 16; ++seed)
            {
                std::string referenceState;
                int referenceDamage = 0;
                float referenceRecovery = 0;
                std::optional<Party::Snapshot> initialState;
                for (bool trace : {false, true})
                {
                    Party party;
                    party.setItemTable(&data().itemTable);
                    party.setItemEnchantTables(&data().standardItemEnchantTable, &data().specialItemEnchantTable);
                    Character member = fighter();
                    member.health = member.maxHealth = 100;
                    member.endurance = member.intellect = member.personality = 14;
                    member.skills["Armsmaster"] = {"Armsmaster", 5, SkillMastery::Grandmaster};
                    member.speed = 30;
                    member.equipment.mainHand = weapon(setup == 2 ? "Sword" : "Axe",
                        setup == 1 ? "Weapon2" : "Weapon");
                    if (setup == 2)
                    {
                        member.equipment.offHand = weapon("Axe", "Weapon");
                    }
                    PartySeed partySeed;
                    partySeed.members.push_back(member);
                    party.seed(partySeed);
                    if (haste)
                    {
                        party.applyPartyBuff(PartyBuffId::Haste, 600, 0, 0, 10, SkillMastery::Master, 0);
                    }
                    if (!trace)
                    {
                        initialState = party.snapshot();
                        initialState->houseStockSeed = 123;
                    }
                    party.restoreSnapshot(*initialState);
                    party.member(0)->equippedItemEffectFlags.insert("PerfectAccuracy");
                    configureGameplayCombatTrace(false, log.string(), false, true, trace);
                    AxeTestWorld world;
                    const auto result = GameplayActionController::executePartyAttack({
                        .pWorldRuntime = &world, .pParty = &party, .pItemTable = &data().itemTable,
                        .pSpellTable = &data().spellTable, .pMonsterTable = &data().monsterTable,
                        .pSpecialItemEnchantTable = &data().specialItemEnchantTable,
                        .directTargetActorIndex = 0, .randomSeed = seed});
                    REQUIRE(result.attacked);
                    REQUIRE(result.attack.axe.damageResolved);
                    CHECK_EQ(result.attack.axe.damageMultiplier, setup == 1 ? 2 : 1);
                    CHECK_EQ(result.attack.axe.recoveryBonus, setup == 1 ? 0 : 10);
                    CHECK_EQ(result.attack.axe.physicalFinal, world.damage);
                    CHECK_EQ(result.attack.axe.physicalBefore,
                        result.attack.damage * result.attack.axe.targetMultiplier - result.attack.axe.elementalDamage);
                    CHECK_EQ(result.attack.axe.physicalAfter,
                        result.attack.axe.physicalBefore * (setup == 1 ? 2 : 1));
                    CHECK_EQ(party.member(0)->attackRecoveryReductionTicks, haste ? 25 : 0);
                    CHECK_EQ(ticks(result.attack.recoverySeconds), result.attack.axe.recoveryFinal);
                    GameSaveData save;
                    save.party = party.snapshot();
                    std::string error;
                    REQUIRE_MESSAGE(saveGameDataToPath(savePath, save, error), error);
                    std::ifstream input(savePath, std::ios::binary);
                    const std::string state{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
                    if (!trace)
                    {
                        referenceState = state;
                        referenceDamage = world.damage;
                        referenceRecovery = result.attack.recoverySeconds;
                    }
                    else
                    {
                        const auto mismatch = std::mismatch(state.begin(), state.end(), referenceState.begin(),
                            referenceState.end());
                        INFO("first differing byte: " << std::distance(state.begin(), mismatch.first));
                        INFO("sizes: " << state.size() << ", " << referenceState.size());
                        CHECK_EQ(state, referenceState);
                        CHECK_EQ(world.damage, referenceDamage);
                        CHECK_EQ(result.attack.recoverySeconds, referenceRecovery);
                        std::ifstream logged(log);
                        const std::string text{std::istreambuf_iterator<char>(logged), std::istreambuf_iterator<char>()};
                        CHECK(text.find("axe_attack member_name=\"Axe Tester\"") != std::string::npos);
                        CHECK(text.find("physical_before_gm=" + std::to_string(result.attack.axe.physicalBefore))
                            != std::string::npos);
                        CHECK(text.find("final_physical_damage=" + std::to_string(world.damage)) != std::string::npos);
                        const std::string recoveryField = "final_recovery_ticks="
                            + std::to_string(result.attack.axe.recoveryFinal);
                        CHECK(text.find(recoveryField)
                            != std::string::npos);
                        CHECK(gameplayMinotaurDamageTraceEnabled());
                    }
                }
            }
        }
    }
    configureGameplayCombatTrace(false, log.string(), true);
    std::filesystem::remove(log);
    std::filesystem::remove(savePath);
}
