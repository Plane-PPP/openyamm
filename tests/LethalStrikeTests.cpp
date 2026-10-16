#include <doctest/doctest.h>

#include "engine/TextTable.h"
#include "game/ui/UiLayoutManager.h"
#include "game/tables/CharacterInspectTable.h"
#include "game/app/GameSettings.h"
#include "game/debug/GameplayDebugTrace.h"
#include "game/gameplay/GameMechanics.h"
#include "game/gameplay/GameplayActionController.h"
#include "game/gameplay/LethalStrikeMonsters.h"
#include "game/gameplay/MasteryTeacherDialog.h"
#include "game/maps/SaveGame.h"
#include "game/ui/GameplayUiController.h"
#include "tests/RegressionGameData.h"
#include "tests/HouseDialogueTestHarness.h"
#include "tests/PartySpellTestHarness.h"

#include <cmath>
#include <fstream>
#include <iterator>
#include <limits>
#include <array>
#include <tuple>

using namespace OpenYAMM::Game;

namespace
{
const OpenYAMM::Tests::RegressionGameData &data()
{
    REQUIRE_MESSAGE(OpenYAMM::Tests::regressionGameDataLoaded(), OpenYAMM::Tests::regressionGameDataFailure());
    return OpenYAMM::Tests::regressionGameData();
}

Character minotaur(const std::string &className = "MinotaurLord")
{
    Character c = OpenYAMM::Tests::makeSpellRegressionPartyMember("Lethal Tester", className, "PC01-01", 1);
    c.level = 50;
    c.skills["LethalStrike"] = {"LethalStrike", 10, SkillMastery::Grandmaster};
    c.skills["Perception"] = {"Perception", 10, SkillMastery::Normal};
    return c;
}

class LethalWorld : public OpenYAMM::Tests::PartySpellTestWorldRuntime
{
public:
    int hp = 10000;
    int maximum = 10000;
    int monster = 1;
    int level = 50;
    int deaths = 0;
    int ordinaryApplications = 0;
    int damage = 0;
    bool rejectDeath = false;
    int ordinaryDamageOverride = -1;
    bool hasTarget = true;
    float targetDistance = 100;
    std::optional<GameplayPartyAttackActorFacts> partyAttackActorFacts(size_t index, bool) const override
    {
        if (!hasTarget) return std::nullopt;
        return GameplayPartyAttackActorFacts{.actorIndex = index, .monsterId = int16_t(monster),
            .displayName = "Target", .position = {targetDistance, 0, 0}, .currentHp = hp, .maxHp = maximum,
            .effectiveLevel = level, .isDead = hp <= 0, .hostileToParty = true};
    }
    bool applyPartyAttackMeleeDamage(size_t, int value, const GameplayWorldPoint &) override
    {
        ++ordinaryApplications;
        damage = value;
        hp = std::max(0, hp - (ordinaryDamageOverride >= 0 ? ordinaryDamageOverride : value));
        return true;
    }
    bool spawnPartyAttackProjectile(const GameplayPartyAttackProjectileRequest &) override { return true; }
    bool isPartyAttackMeleeBModelTarget(size_t) const override { return true; }
    bool applyPartyAttackMeleeBModelDamage(size_t, int) override { return true; }
    bool castPartyAttackSpell(const GameplayPartyAttackSpellRequest &) override { return true; }
    bool killPartyAttackActor(size_t, const GameplayWorldPoint &) override
    {
        if (hp <= 0 || rejectDeath) return false;
        hp = 0;
        ++deaths;
        return true;
    }
};

uint32_t weapon(const std::string &skill, const std::string &equip)
{
    for (const ItemDefinition &item : data().itemTable.entries())
        if (canonicalSkillName(item.skillGroup) == skill && (equip.empty() || item.equipStat == equip)
            && item.itemId > 0 && item.itemId < 500) return item.itemId;
    FAIL("missing weapon");
    return 0;
}
}

TEST_CASE("Lethal Strike v0.4 fractional formula complete parameter grid")
{
    Character c = minotaur();
    const double a[] = {0.60, 0.90, 1.10, 1.30};
    const double b[] = {0.0, 0.56, 0.64, 0.72};
    for (int rank = 1; rank <= 4; ++rank)
        for (uint32_t s : {1, 5, 10, 15, 20})
            for (uint32_t v : {0, 1, 5, 10, 15, 20})
            {
                c.skills["LethalStrike"] = {"LethalStrike", s, static_cast<SkillMastery>(rank)};
                c.skills["Perception"] = {"Perception", v, SkillMastery::Normal};
                const double expected = a[rank - 1] * s + b[rank - 1] * v;
                CHECK(GameMechanics::lethalStrikeBaseChance(c) == doctest::Approx(expected));
                // Item bonuses and Identify Monster have no effect.
                c.itemSkillBonuses["LethalStrike"] = 40;
                c.itemSkillBonuses["Perception"] = 40;
                c.skills["IdentifyMonster"] = {"IdentifyMonster", 63, SkillMastery::Grandmaster};
                CHECK(GameMechanics::lethalStrikeBaseChance(c) == doctest::Approx(expected));
                for (int delta : {-10000, -60, -40, -20, 0, 20, 24, 60})
                {
                    const int ml = 50 - delta;
                    const double factor = delta >= 0 ? std::min(1.4, 1.0 + delta / 60.0)
                        : 30.0 / (30.0 + 1.5 * std::abs(delta));
                    CHECK(GameMechanics::lethalStrikeChance(c, ml, false, 1, 100)
                        == doctest::Approx(std::min(30.0, expected * factor)));
                    CHECK(GameMechanics::lethalStrikeChance(c, ml, true, 59, 100)
                        == GameMechanics::lethalStrikeChance(c, ml, false, 1, 100));
                }
            }
    c.skills["LethalStrike"] = {"LethalStrike", 1, SkillMastery::Normal};
    CHECK(GameMechanics::lethalStrikeChance(c, 50, false, 1, 100) == doctest::Approx(0.6));
    CHECK(GameMechanics::lethalStrikeChance(c, 70, false, 1, 100) == doctest::Approx(0.3));
    c.levelModifier = 20;
    CHECK(GameMechanics::lethalStrikeChance(c, 70, false, 1, 100) == doctest::Approx(0.6));
    c.skills.clear();
    CHECK(GameMechanics::lethalStrikeBaseChance(c) == 0);
    CHECK(GameMechanics::lethalStrikeBaseChanceText(c) == "Base Lethal Strike Chance: 0.00%");
    c = minotaur();
    c.skills["LethalStrike"].mastery = SkillMastery::Expert;
    CHECK(GameMechanics::lethalStrikeBaseChanceText(c) == "Base Lethal Strike Chance: 14.60%");
    CHECK(GameMechanics::lethalStrikeBaseChanceText(c, true) == "Базовый шанс смертельного удара: 14,60%");
    c.skills["LethalStrike"].level = 63;
    CHECK(GameMechanics::lethalStrikeBaseChance(c) > 30);
    CHECK(GameMechanics::lethalStrikeChance(c, 50, false, 1, 100) == 30);
    c.className = "Knight";
    CHECK(GameMechanics::lethalStrikeBaseChance(c) == 0);
}

TEST_CASE("Lethal Strike GIANT strict sixty percent and approved template registry")
{
    for (int hp : {0, 60, 61, 100}) CHECK_FALSE(GameMechanics::lethalStrikeHealthEligible(true, hp, 100));
    for (int hp : {1, 59}) CHECK(GameMechanics::lethalStrikeHealthEligible(true, hp, 100));
    CHECK(GameMechanics::lethalStrikeHealthEligible(false, 100, 100));
    CHECK_FALSE(GameMechanics::lethalStrikeHealthEligible(true, 1, 0));
    CHECK_FALSE(GameMechanics::lethalStrikeHealthEligible(true, std::numeric_limits<int>::max(),
        std::numeric_limits<int>::max()));
    for (int first : {28, 31, 58, 61, 76, 79, 91, 106})
        for (int id = first; id < first + 3; ++id) CHECK_FALSE(lethalStrikeGiantMonster(id));
    for (int first : {70, 112, 130, 136, 157, 163, 190, 220})
        for (int id = first; id < first + 3; ++id) CHECK(lethalStrikeGiantMonster(id));
    CHECK_FALSE(lethalStrikeKnownMonster(700));
    CHECK_FALSE(lethalStrikeKnownMonster(9999));
    CHECK_FALSE(lethalStrikeGiantMonster(9999));
    CHECK(lethalStrikeKnownMonster(906));
}

TEST_CASE("Lethal Strike innate migration hiring promotion and save persistence")
{
    CHECK(allCanonicalSkillNames()[39] == "Throwing");
    CHECK(allCanonicalSkillNames()[40] == "LethalStrike");
    Party party;
    party.setClassSkillTable(&data().classSkillTable);
    Character c = minotaur("Minotaur");
    c.skills.erase("LethalStrike");
    c.skillPoints = 90;
    PartySeed seed;
    seed.members.push_back(c);
    party.seed(seed);
    REQUIRE(party.member(0)->hasSkill("LethalStrike"));
    CHECK(party.member(0)->findSkill("LethalStrike")->level == 1);
    CHECK(party.member(0)->findSkill("LethalStrike")->mastery == SkillMastery::Normal);
    CHECK(party.member(0)->skillPoints == 90);
    CHECK(party.member(0)->hasSkill("Perception"));
    REQUIRE(party.increaseActiveMemberSkillLevel("LethalStrike"));
    CHECK(party.member(0)->skillPoints == 88);
    REQUIRE(party.setMemberClassName(0, "MinotaurLord"));
    CHECK(party.member(0)->findSkill("LethalStrike")->level == 2);
    CHECK(party.member(0)->findSkill("LethalStrike")->mastery == SkillMastery::Normal);
    party.member(0)->skills["LethalStrike"] = {"LethalStrike", 15, SkillMastery::Grandmaster};
    const auto snapshot = party.snapshot();
    for (int n = 0; n < 3; ++n)
    {
        party.restoreSnapshot(snapshot);
        CHECK(party.member(0)->findSkill("LethalStrike")->level == 15);
        CHECK(party.member(0)->findSkill("LethalStrike")->mastery == SkillMastery::Grandmaster);
        CHECK(party.member(0)->skills.size() == snapshot.members[0].skills.size());
    }
    for (uint32_t rosterId : {5, 13, 21, 29})
    {
        const RosterEntry *pRoster = data().rosterTable.get(rosterId);
        REQUIRE(pRoster != nullptr);
        Party hired;
        REQUIRE(hired.recruitRosterMember(*pRoster));
        REQUIRE(hired.member(0)->hasSkill("LethalStrike"));
        CHECK(hired.member(0)->findSkill("LethalStrike")->level == 1);
    }
    Party::Snapshot old = snapshot;
    old.members[0].skills.erase("LethalStrike");
    party.restoreSnapshot(old);
    CHECK(party.member(0)->findSkill("LethalStrike")->level == 1);
    const auto path = std::filesystem::temp_directory_path() / "openyamm-lethal-save.oysav";
    GameSaveData save;
    save.mapFileName = "out01.odm";
    save.party = snapshot;
    std::string error;
    REQUIRE_MESSAGE(saveGameDataToPath(path, save, error), error);
    const auto loaded = loadGameDataFromPath(path, error);
    REQUIRE_MESSAGE(loaded, error);
    party.restoreSnapshot(loaded->party);
    CHECK(party.member(0)->findSkill("LethalStrike")->level == 15);
    const auto legacy = loadGameDataFromPath(
        std::filesystem::path(OPENYAMM_SOURCE_DIR) / "tests/fixtures/minotaur-v84.oysav", error);
    REQUIRE_MESSAGE(legacy, error);
    CHECK_FALSE(legacy->party.members[0].hasSkill("LethalStrike"));
    party.restoreSnapshot(legacy->party);
    REQUIRE(party.member(0)->hasSkill("LethalStrike"));
    CHECK(party.member(0)->findSkill("LethalStrike")->level == 1);
    CHECK(party.member(0)->hasSkill("Perception"));
    save.outdoorWorld.mapActors.emplace_back();
    save.outdoorWorld.mapActors[0].effectiveLevel = 87;
    REQUIRE(saveGameDataToPath(path, save, error));
    const auto cachedLevel = loadGameDataFromPath(path, error);
    REQUIRE(cachedLevel);
    CHECK(cachedLevel->outdoorWorld.mapActors[0].effectiveLevel == 87);
    std::filesystem::remove(path);
    for (const std::string cls : {"Minotaur", "MinotaurLord", "Knight", "Necromancer", "Lich"})
    {
        const SkillMastery cap = data().classSkillTable.getEffectiveCap(cls, 3, "LethalStrike");
        CHECK(cap == (cls == "Minotaur" ? SkillMastery::Master
            : cls == "MinotaurLord" ? SkillMastery::Grandmaster : SkillMastery::None));
    }
}

TEST_CASE("Lethal Strike teachers load real dialog topics retain services and enforce promotion")
{
    Party party;
    party.setClassSkillTable(&data().classSkillTable);
    PartySeed seed;
    Character c = minotaur("Minotaur");
    c.skills["LethalStrike"] = {"LethalStrike", 10, SkillMastery::Normal};
    seed.members.push_back(c);
    seed.gold = 100000;
    party.seed(seed);
    for (uint32_t i = 0; i < 3; ++i)
    {
        const NpcEntry *pNpc = data().npcDialogTable.getNpc(std::array<uint32_t, 3>{184, 131, 117}[i]);
        REQUIRE(pNpc != nullptr);
        CHECK(pNpc->houseId > 0);
        CHECK(std::find(pNpc->topicIds.begin(), pNpc->topicIds.end(), std::array<uint32_t, 3>{387, 319, 311}[i]) != pNpc->topicIds.end());
        CHECK(std::find(pNpc->topicIds.begin(), pNpc->topicIds.end(), 29000 + i) != pNpc->topicIds.end());
        const auto topics = data().npcDialogTable.getTopicsForNpc(pNpc->id);
        CHECK(std::any_of(topics.begin(), topics.end(), [i](const auto &topic)
        {
            return topic.id == 29000 + i && !topic.topic.empty();
        }));
        OpenYAMM::Tests::HouseDialogueTestHarness harness(data());
        const auto &dialog = harness.openNpcDialogue(pNpc->id, pNpc->houseId);
        CHECK(std::any_of(dialog.actions.begin(), dialog.actions.end(), [i](const auto &action)
        {
            return action.id == 29000 + i && action.kind == EventDialogActionKind::MasteryTeacherOffer;
        }));
        REQUIRE(harness.party().setMemberClassName(harness.party().activeMemberIndex(), "MinotaurLord"));
        auto *member = harness.party().activeMember();
        REQUIRE(member);
        member->skills["LethalStrike"] = {"LethalStrike", 10, static_cast<SkillMastery>(i + 1)};
        harness.party().addGold(100000);
        const auto &offer = harness.openMasteryTeacherOffer(pNpc->id,
            std::array<std::string, 3>{"Expert Lethal Strike", "Master Lethal Strike", "Grandmaster Lethal Strike"}[i],
            pNpc->houseId);
        REQUIRE_FALSE(offer.actions.empty());
        CHECK(offer.actions.front().label.find("Lethal Strike") != std::string::npos);
        const auto goldBefore = harness.party().gold();
        harness.executeAndPresent(0);
        CHECK(goldBefore - harness.party().gold() == std::array<int, 3>{2000, 5000, 8000}[i]);
        CHECK(member->skills.at("LethalStrike").mastery == static_cast<SkillMastery>(i + 2));
        auto evaluation = evaluateMasteryTeacherTopic(29000 + i, party, data().classSkillTable,
            data().npcDialogTable, &data().mergedTeacherTopicTable);
        REQUIRE(evaluation);
        CHECK(evaluation->approved == (i < 2));
        if (i < 2)
        {
            std::string message;
            REQUIRE(applyMasteryTeacherTopic(29000 + i, party, data().classSkillTable,
                data().npcDialogTable, &data().mergedTeacherTopicTable, message));
        }
    }
    REQUIRE(party.setMemberClassName(0, "MinotaurLord"));
    auto evaluation = evaluateMasteryTeacherTopic(29002, party, data().classSkillTable,
        data().npcDialogTable, &data().mergedTeacherTopicTable);
    REQUIRE(evaluation);
    CHECK(evaluation->approved);
}

TEST_CASE("Lethal Strike one roll after a complete melee action and trace invariance")
{
    const auto path = std::filesystem::temp_directory_path() / "openyamm-lethal-action.log";
    MonsterTable physicalImmune;
    std::vector<std::string> immuneRow(38);
    immuneRow[0] = "151"; immuneRow[1] = "Fear"; immuneRow[2] = "Nightmare A";
    immuneRow[3] = "50"; immuneRow[4] = "10000"; immuneRow[5] = "0";
    immuneRow[10] = "Short"; immuneRow[11] = "Normal"; immuneRow[13] = "100";
    immuneRow[14] = "100"; immuneRow[17] = "Phys"; immuneRow[18] = "1d6"; immuneRow[37] = "Imm";
    REQUIRE(physicalImmune.loadStatsFromRows({immuneRow}));
    int successes = 0;
    int misses = 0;
    for (int setup = 0; setup < 9; ++setup)
        for (uint32_t seedValue = 0; seedValue < 80; ++seedValue)
        {
            bool referenceKill = false;
            int referenceHp = 0;
            for (bool traced : {false, true})
            {
                Party party;
                party.setItemTable(&data().itemTable);
                Character c = minotaur();
                c.skills["Axe"] = {"Axe", 10, SkillMastery::Grandmaster};
                c.skills["Mace"] = {"Mace", 10, SkillMastery::Normal};
                c.skills["LethalStrike"].level = 20;
                c.skills["Perception"].level = 20;
                if (setup != 0) c.equipment.mainHand = weapon("Axe", setup == 2 ? "Weapon2" : "Weapon");
                if (setup == 3) c.equipment.offHand = weapon("Axe", "Weapon");
                if (setup == 4)
                {
                    c.equipment.mainHand = weapon("Mace", "Weapon");
                    c.equipment.offHand = weapon("Axe", "Weapon");
                }
                PartySeed seed;
                seed.members.push_back(c);
                party.seed(seed);
                LethalWorld world;
                if (setup == 5) world.hp = 1;
                if (setup == 6 || setup == 7)
                {
                    world.monster = 70;
                    world.maximum = 100;
                    world.hp = setup == 6 ? 60 : 61;
                    world.ordinaryDamageOverride = 1;
                }
                if (setup == 8) world.monster = 151; // Physical immunity must not imply Lethal Strike immunity.
                configureGameplayCombatTrace(false, path.string(), false, false, false, traced);
                GameplayActionController::PartyAttackConfig config;
                config.pParty = &party;
                config.pWorldRuntime = &world;
                config.pItemTable = &data().itemTable;
                config.pSpellTable = &data().spellTable;
                config.pMonsterTable = setup == 8 ? &physicalImmune : &data().monsterTable;
                config.directTargetActorIndex = 0;
                config.randomSeed = seedValue;
                const auto result = GameplayActionController::executePartyAttack(config);
                REQUIRE(result.actionPerformed);
                CHECK(world.ordinaryApplications <= 1);
                CHECK(world.deaths <= 1);
                CHECK(result.lethalStrikeSucceeded == (world.deaths == 1));
                if (!result.attack.hit)
                {
                    ++misses;
                    CHECK(world.deaths == 0);
                }
                if (setup == 5 || setup == 7) CHECK(world.deaths == 0);
                if (setup == 8) CHECK(world.damage == 0);
                if (!traced)
                {
                    referenceKill = result.lethalStrikeSucceeded;
                    referenceHp = world.hp;
                    successes += world.deaths;
                }
                else
                {
                    CHECK(result.lethalStrikeSucceeded == referenceKill);
                    CHECK(world.hp == referenceHp);
                    std::ifstream f(path);
                    const std::string text{std::istreambuf_iterator<char>(f), {}};
                    CHECK(text.find("lethal_strike action_id=") != std::string::npos);
                    const size_t first = text.find("lethal_strike action_id=");
                    CHECK(text.find("lethal_strike action_id=", first + 1) == std::string::npos);
                    if (world.deaths) CHECK(text.find("success=1 death_processed=1") != std::string::npos);
                }
            }
        }
    CHECK(successes > 0);
    CHECK(misses > 0);
    configureGameplayCombatTrace(false, path.string(), true);
    std::filesystem::remove(path);
}

TEST_CASE("Lethal Strike success message has priority and does not vanish on ordinary hits")
{
    GameplayUiController ui;
    const std::string text = GameMechanics::lethalStrikeSuccessText("Arius");
    CHECK(text == "Arius delivers a Lethal Strike!");
    ui.setStatusBarEvent(text, 4.0f, GameplayUiController::StatusBarEventPriority::High);
    ui.setStatusBarEvent("Ordinary hit", 2.0f);
    CHECK(ui.statusBar().eventText == text);
}


TEST_CASE("Lethal Strike and Damage Resistance descriptions and Stats layout")
{
    const std::filesystem::path root = OPENYAMM_SOURCE_DIR;
    const auto read = [](const std::filesystem::path &path)
    {
        std::ifstream f(path, std::ios::binary);
        REQUIRE(f.good());
        return std::string(std::istreambuf_iterator<char>(f), {});
    };
    CharacterInspectTable inspect;
    const auto skillRows = OpenYAMM::Engine::TextTable::parseTabSeparated(
        read(root / "assets_dev/engine/data_tables/english/skill_des.txt"));
    const auto statRows = OpenYAMM::Engine::TextTable::parseTabSeparated(
        read(root / "assets_dev/engine/data_tables/english/stats.txt"));
    REQUIRE(skillRows);
    REQUIRE(statRows);
    std::vector<std::vector<std::string>> rows;
    for (size_t i = 0; i < skillRows->getRowCount(); ++i) rows.push_back(skillRows->getRow(i));
    REQUIRE(inspect.loadSkillRows(rows));
    rows.clear();
    for (size_t i = 0; i < statRows->getRowCount(); ++i) rows.push_back(statRows->getRow(i));
    REQUIRE(inspect.loadStatRows(rows));
    REQUIRE(inspect.getSkill("LethalStrike"));
    CHECK(inspect.getSkill("LethalStrike")->description.find("60%") != std::string::npos);
    CHECK(inspect.getSkill("LethalStrike")->description.find("30%") != std::string::npos);
    REQUIRE(inspect.getStat("DamageResistance"));
    CHECK(inspect.getStat("DamageResistance")->description.find("physical") != std::string::npos);
    REQUIRE(inspect.getStat("ArmorClass"));
    REQUIRE(inspect.getSkill("Axe"));
    UiLayoutManager layout;
    REQUIRE(layout.loadLayoutText("character.yml", read(root / "assets_dev/engine/ui/gameplay/character.yml")));
    const auto *ac = layout.findElement("CharacterStatArmorClassLabel");
    const auto *dr = layout.findElement("CharacterStatDamageResistanceLabel");
    const auto *condition = layout.findElement("CharacterStatConditionLabel");
    REQUIRE(ac);
    REQUIRE(dr);
    REQUIRE(condition);
    CHECK(ac->gapY + ac->height <= dr->gapY);
    CHECK(dr->gapY + dr->height <= condition->gapY);
    // Keep every label/value and hit rectangle inside the existing painted sections.
    for (const auto &[names, top, bottom] : {
        std::tuple{std::vector<std::string>{"HitPoints", "SpellPoints", "ArmorClass", "DamageResistance"}, 179.0f, 238.0f},
        std::tuple{std::vector<std::string>{"Condition", "QuickSpell"}, 244.0f, 284.5f},
        std::tuple{std::vector<std::string>{"FireResistance", "AirResistance", "WaterResistance",
            "EarthResistance", "MindResistance", "BodyResistance"}, 190.0f, 303.0f}})
    {
        float previousBottom = top;
        for (const auto &name : names)
        {
            const auto *label = layout.findElement("CharacterStat" + name + "Label");
            const auto *value = layout.findElement("CharacterStat" + name + "Value");
            REQUIRE(label);
            REQUIRE(value);
            CHECK(label->gapY >= previousBottom);
            CHECK(label->gapY + label->height <= bottom);
            CHECK(value->gapY == label->gapY);
            CHECK(value->height == label->height);
            previousBottom = label->gapY + label->height;
        }
    }
    for (uint32_t npcId : {115u, 116u})
    {
        const auto topics = data().npcDialogTable.getTopicsForNpc(npcId);
        CHECK(std::none_of(topics.begin(), topics.end(), [](const auto &topic)
        {
            return topic.id == 29000 || topic.id == 29001;
        }));
    }
    Character c = minotaur("Minotaur");
    for (const auto &[level,expected] : {std::pair{10,7}, {20,9}, {50,15}})
    {
        c.level = uint32_t(level);
        CHECK(GameMechanics::minotaurDamageResistancePercent(c) == expected);
    }
    c.className = "MinotaurLord";
    c.level = 20;
    CHECK(GameMechanics::minotaurDamageResistancePercent(c) == 16);
    c.level = 50;
    CHECK(GameMechanics::minotaurDamageResistancePercent(c) == 22);
    c.className = "Knight";
    CHECK(GameMechanics::minotaurDamageResistancePercent(c) == 0);
}


TEST_CASE("Lethal Strike excludes shooting spells world objects and untargeted swings")
{
    for (int mode = 0; mode < 4; ++mode)
    {
        Party party;
        party.setItemTable(&data().itemTable);
        Character c = minotaur();
        c.skills["LethalStrike"].level = 63;
        c.skills["Perception"].level = 63;
        c.skills["Bow"] = {"Bow", 10, SkillMastery::Grandmaster};
        LethalWorld world;
        if (mode == 0)
        {
            c.equipment.bow = weapon("Bow", "");
            world.targetDistance = 1000;
        }
        if (mode == 1)
        {
            c.attackSpellName = "Fire Bolt";
            c.skills["FireMagic"] = {"FireMagic", 10, SkillMastery::Master};
            c.knownSpellIds.insert(2);
        }
        if (mode >= 2) world.hasTarget = false;
        PartySeed seed;
        seed.members.push_back(c);
        party.seed(seed);
        if (mode == 1)
        {
            world.bindParty(&party);
            GameplayRuntimeActorState actor;
            actor.preciseX = 100;
            actor.height = 160;
            actor.hostileToParty = true;
            const size_t index = world.addActor(actor);
            party.member(0)->spellPoints = party.member(0)->maxSpellPoints = 100;
            PartySpellCastRequest request;
            request.casterMemberIndex = 0;
            request.spellId = 2;
            request.targetActorIndex = index;
            const auto cast = PartySpellSystem::castSpell(party, world, data().spellTable, request);
            REQUIRE(cast.succeeded());
            CHECK(world.deaths == 0);
            CHECK(world.ordinaryApplications == 0);
            continue;
        }
        GameplayActionController::PartyAttackConfig config;
        config.pParty = &party;
        config.pWorldRuntime = &world;
        config.pItemTable = &data().itemTable;
        config.pSpellTable = &data().spellTable;
        config.pMonsterTable = &data().monsterTable;
        config.arrowProjectileObjectId = 1;
        config.directTargetActorIndex = 0;
        config.randomSeed = 1;
        if (mode == 2)
        {
            config.directTargetBModelIndex = 0;
            config.directWorldTargetDistance = 100;
        }
        const auto result = GameplayActionController::executePartyAttack(config);
        CHECK_FALSE(result.lethalStrikeSucceeded);
        CHECK(world.deaths == 0);
        if (mode == 0) CHECK(result.attack.mode == CharacterAttackMode::Bow);
    }
}

TEST_CASE("Lethal Strike trace settings persist independently")
{
    const auto path = std::filesystem::temp_directory_path() / "openyamm-lethal-settings.ini";
    GameSettings settings = GameSettings::createDefault();
    CHECK_FALSE(settings.lethalStrikeTrace);
    CHECK_FALSE(settings.minotaurRussianText);
    settings.lethalStrikeTrace = true;
    settings.minotaurRussianText = true;
    settings.axeDamageTrace = true;
    settings.minotaurDamageTrace = true;
    std::string error;
    REQUIRE(saveGameSettings(path, settings, error));
    const auto restored = loadGameSettings(path, error);
    REQUIRE(restored);
    CHECK(restored->lethalStrikeTrace);
    CHECK(restored->minotaurRussianText);
    CHECK(restored->axeDamageTrace);
    CHECK(restored->minotaurDamageTrace);
    std::filesystem::remove(path);
}
