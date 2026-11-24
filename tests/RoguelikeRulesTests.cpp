#include "doctest/doctest.h"

#include "game/content/DungeonRun.h"
#include "game/gameplay/GameMechanics.h"
#include "game/party/Party.h"
#include "game/maps/SaveGame.h"
#include "game/rules/roguelike/RoguelikeRules.h"
#include "game/tables/ClassSkillTable.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

using OpenYAMM::Game::Character;
using OpenYAMM::Game::ClassSkillTable;
using OpenYAMM::Game::DungeonRunDefinition;
using OpenYAMM::Game::DungeonRunRules;
using OpenYAMM::Game::DungeonRunStop;
using OpenYAMM::Game::MapKillProgress;
using OpenYAMM::Game::RoguelikeRunState;
using OpenYAMM::Game::RunCard;
using OpenYAMM::Game::SkillMastery;

namespace
{
DungeonRunDefinition installedRulesRun()
{
    std::ifstream input(
        std::filesystem::path(OPENYAMM_SOURCE_DIR) / "assets_dev/engine/rulesets/roguelike/run.yml");
    const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    std::string error;
    const std::optional<DungeonRunDefinition> run = OpenYAMM::Game::parseDungeonRunDefinition(text, error);
    REQUIRE_MESSAGE(run.has_value(), error);
    REQUIRE(run->rules.has_value());
    return *run;
}

ClassSkillTable testClassSkills()
{
    ClassSkillTable table;
    REQUIRE(table.loadCapsFromRows({
        {"", "Knight", "Sorcerer"},
        {"Sword", "G", "-"},
        {"Bow", "E", "B"},
        {"Bodybuilding", "M", "-"},
        {"FireMagic", "-", "G"},
        {"AirMagic", "-", "M"},
    }));
    return table;
}

Character testHero(const std::string &className)
{
    Character hero = {};
    hero.name = "Test";
    hero.className = className;
    hero.level = 1;
    return hero;
}

// Every number of a hero's permanent bonuses, in one row to compare.
std::vector<int> bonusNumbers(const OpenYAMM::Game::CharacterStatBonuses &bonuses)
{
    const OpenYAMM::Game::CharacterResistanceSet &resistances = bonuses.resistances;
    return {bonuses.might, bonuses.intellect, bonuses.personality, bonuses.endurance, bonuses.speed,
        bonuses.accuracy, bonuses.luck, bonuses.maxHealth, bonuses.maxSpellPoints, bonuses.armorClass,
        bonuses.meleeAttack, bonuses.rangedAttack, bonuses.meleeDamage, bonuses.rangedDamage, resistances.fire,
        resistances.air, resistances.water, resistances.earth, resistances.mind, resistances.body,
        resistances.spirit, resistances.light, resistances.dark};
}

MapKillProgress progressOf(double slain, double counted)
{
    MapKillProgress progress;
    progress.slainHealth = slain;
    progress.countedHealth = counted;
    return progress;
}
}

TEST_CASE("the installed run's rules: a route of four by stage, ten level-ups and cards to deal")
{
    const DungeonRunDefinition run = installedRulesRun();
    const DungeonRunRules &rules = *run.rules;

    CHECK_EQ(rules.route, std::vector<int>{1, 2, 3, 4});
    CHECK_EQ(rules.levelUps.size(), rules.route.size());
    CHECK_EQ(rules.totalLevelUps(), 10);
    CHECK_EQ(rules.levelsPerLevelUp, 4);
    CHECK_EQ(rules.clearShare, doctest::Approx(0.5));
    CHECK_EQ(rules.guardianHealthScale, doctest::Approx(4.0));
    // A boss of its own (its kind alone on the map) is fortified twice, a kind of many four times (the user,
    // 2026-11-17).
    CHECK_EQ(rules.uniqueGuardianHealthScale, doctest::Approx(2.0));
    CHECK_EQ(OpenYAMM::Game::runGuardianHealthScale(rules, 1), doctest::Approx(2.0));
    CHECK_EQ(OpenYAMM::Game::runGuardianHealthScale(rules, 2), doctest::Approx(4.0));
    CHECK_EQ(OpenYAMM::Game::runGuardianHealthScale(rules, 11), doctest::Approx(4.0));
    CHECK(rules.skills.size() >= 10);
    CHECK_EQ(rules.cardOffers, 4);
    CHECK_EQ(rules.levelUpHealShare, doctest::Approx(0.25));
    // The start's food pays for one rest in the camp.
    CHECK(rules.campRestFood > 0);
    CHECK(rules.startFood >= rules.campRestFood);
    CHECK(rules.talents.size() >= 6);
    // Every card of a skill has a picture.
    for (const std::string &skill : rules.skills)
    {
        CHECK_MESSAGE(rules.skillIcons.contains(skill), skill);
    }
    // The alchemy shop of every camp sells small stacks of herbs and bottles.
    size_t alchemyShops = 0;
    for (const OpenYAMM::Game::DungeonRunShop &shop : rules.shops)
    {
        if (shop.stock.empty())
        {
            continue;
        }
        ++alchemyShops;
        CHECK_EQ(shop.name, "Potions and magic");
        for (const OpenYAMM::Game::DungeonRunShopStack &stack : shop.stock)
        {
            // MM6's herbs and the table's empty alchemy bottle (220), never MM6's own bottle row 1765.
            CHECK(((stack.itemId >= 1762 && stack.itemId <= 1764) || stack.itemId == 220));
            CHECK(stack.count >= 1);
            CHECK(stack.count <= 10);
        }
    }
    CHECK_EQ(alchemyShops, rules.route.size() - 1);
    // A reward for each guardian: artifacts first, relics later, and no gold after the last (no camp follows it).
    REQUIRE_EQ(rules.guardianRewards.size(), rules.route.size());
    CHECK_EQ(rules.guardianRewards[0].item, "artifact");
    CHECK_EQ(rules.guardianRewards[1].item, "artifact");
    CHECK_EQ(rules.guardianRewards[2].item, "relic");
    CHECK_EQ(rules.guardianRewards[3].item, "relic");
    CHECK(rules.guardianRewards[0].gold > 0);
    CHECK_EQ(rules.guardianRewards[3].gold, 0);
    REQUIRE(rules.school("FireMagic") != nullptr);
    CHECK_EQ(rules.school("FireMagic")->combatSpell, 2u);
    CHECK(rules.school("Sword") == nullptr);
    // The Medusa heroine (character_data.txt row 79 of the package) has no class caps.
    CHECK_EQ(rules.unrestrictedCharacters, std::vector<uint32_t>{79});

    std::set<std::string> firstStops;
    std::set<std::string> lastStops;

    for (uint64_t seed = 0; seed < 60; ++seed)
    {
        const std::vector<size_t> order = OpenYAMM::Game::dungeonRunOrder(run, seed);
        REQUIRE_EQ(order.size(), 4u);
        CHECK_EQ(std::set<size_t>(order.begin(), order.end()).size(), 4u);

        for (size_t stop = 0; stop < order.size(); ++stop)
        {
            CHECK(OpenYAMM::Game::dungeonRunStopAtStage(run.dungeons[order[stop]], rules.route[stop]));
        }

        CHECK(OpenYAMM::Game::dungeonRunStop(run, seed, 4) == nullptr);
        firstStops.insert(run.dungeons[order[0]].mapFileName);
        lastStops.insert(run.dungeons[order[3]].mapFileName);
    }

    // Every stop random, the first too (2.4 always started in Goblinwatch).
    CHECK(firstStops.size() >= 3);
    CHECK(lastStops.size() >= 3);
}

TEST_CASE("a run's level-ups follow the slain share of hit points and the last comes with the guardian")
{
    DungeonRunRules rules;
    rules.route = {1, 2};
    rules.levelUps = {3, 2};
    rules.clearShare = 0.5;

    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 0, progressOf(0, 1000)), 0);
    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 0, progressOf(166, 1000)), 0);
    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 0, progressOf(167, 1000)), 1);
    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 0, progressOf(340, 1000)), 2);
    // Exactly the share is not yet more than it: no guardian, and the stop's last level-up waits with it.
    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 0, progressOf(500, 1000)), 2);
    CHECK_FALSE(OpenYAMM::Game::runClearThresholdPassed(rules, progressOf(500, 1000)));
    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 0, progressOf(501, 1000)), 3);
    CHECK(OpenYAMM::Game::runClearThresholdPassed(rules, progressOf(501, 1000)));
    // Clearing the rest gives nothing more.
    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 0, progressOf(1000, 1000)), 3);
    // The second stop adds to the first's.
    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 1, progressOf(0, 400)), 3);
    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 1, progressOf(100, 400)), 4);
    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 1, progressOf(400, 400)), 5);
    // A stop past the route gives none, and an empty map never passes.
    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 2, progressOf(400, 400)), 5);
    CHECK_FALSE(OpenYAMM::Game::runClearThresholdPassed(rules, progressOf(0, 0)));
    CHECK_EQ(OpenYAMM::Game::runClearFraction(rules, progressOf(250, 1000)), doctest::Approx(0.5));
}

TEST_CASE("a run's cards are four different ones, fixed by the seed, within the class caps")
{
    DungeonRunRules rules = *installedRulesRun().rules;
    const ClassSkillTable classSkills = testClassSkills();
    Character knight = testHero("Knight");
    knight.skills["Sword"] = {"Sword", 1, SkillMastery::Normal};

    for (uint64_t seed = 0; seed < 40; ++seed)
    {
        const std::vector<RunCard> cards = OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, seed, 0);
        REQUIRE_EQ(cards.size(), 4u);
        CHECK(cards == OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, seed, 0));
        std::set<std::string> ids;
        std::set<std::string> labels;

        // The first is a step of the one skill the knight knows, the second a talent.
        CHECK(cards[0].kind == RunCard::Kind::Skill);
        CHECK_EQ(cards[0].skill, "Sword");
        CHECK_EQ(cards[0].id, "skill:Sword");
        CHECK(cards[0].mastery == SkillMastery::Expert);
        CHECK(cards[1].kind == RunCard::Kind::Talent);

        for (const RunCard &card : cards)
        {
            ids.insert(card.id);
            labels.insert(card.label);
            // No magic for a knight.
            CHECK(card.skill != "FireMagic");
            CHECK(card.skill != "AirMagic");

            if (card.kind == RunCard::Kind::Talent)
            {
                // A stable id of a talent of the rules, whose need the knight meets (no bow: no arrows' talents).
                REQUIRE(rules.talent(card.talent) != nullptr);
                CHECK_EQ(card.id, "talent:" + card.talent);
                CHECK(OpenYAMM::Game::runTalentAvailable(*rules.talent(card.talent), knight));
                CHECK(rules.talent(card.talent)->needsSkill != "Bow");
                CHECK_EQ(card.rank, 1);
                CHECK_EQ(card.maxRank, rules.talent(card.talent)->maxRank);
            }
            else
            {
                CHECK_EQ(card.id, "skill:" + card.skill);
            }
        }

        CHECK_EQ(ids.size(), 4u);
        CHECK_EQ(labels.size(), 4u);
    }

    // Another hero's place and another count of cards taken draw differently somewhere.
    size_t different = 0;

    for (uint64_t seed = 0; seed < 40; ++seed)
    {
        different += OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, seed, 0)
            != OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, seed, 1);
    }

    CHECK(different > 20);

    // The number of offers is the rules' own; a pool smaller than it gives what there is, never a card twice.
    rules.cardOffers = 5;
    CHECK_EQ(OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, 7, 0).size(), 5u);
    rules.cardOffers = 4;
    rules.skills = {"Sword", "Bow"};
    rules.talents = {{.id = "one"}};
    const std::vector<RunCard> few = OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, 7, 0);
    REQUIRE_EQ(few.size(), 3u);
    CHECK_EQ(std::set<std::string>{few[0].id, few[1].id, few[2].id}.size(), 3u);
}

TEST_CASE("a run's skill card raises one step with its level, and stops at the class cap")
{
    DungeonRunRules rules = *installedRulesRun().rules;
    rules.talents.clear();
    rules.skills = {"Bow"};
    const ClassSkillTable classSkills = testClassSkills();
    Character knight = testHero("Knight");
    const std::vector<std::pair<SkillMastery, uint32_t>> steps = {
        {SkillMastery::Normal, 1}, {SkillMastery::Expert, 4}};

    for (const auto &[mastery, level] : steps)
    {
        const std::vector<RunCard> cards = OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, 1, 0);
        REQUIRE_EQ(cards.size(), 1u);
        CHECK(cards[0].mastery == mastery);
        knight.eventVariables[OpenYAMM::Game::RunPendingCardsVariable] = 1;
        CHECK(OpenYAMM::Game::applyRunCard(rules, knight, cards[0], nullptr, 1, 0, {}).empty());
        REQUIRE(knight.findSkillByCanonicalName("Bow") != nullptr);
        CHECK(knight.findSkillByCanonicalName("Bow")->mastery == mastery);
        CHECK_EQ(knight.findSkillByCanonicalName("Bow")->level, level);
        CHECK_EQ(knight.eventVariables[OpenYAMM::Game::RunPendingCardsVariable], 0);
    }

    // A knight's bow stops at Expert: nothing left to offer.
    CHECK(OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, 1, 0).empty());
    // A hero the rules free of class caps goes on to Grandmaster, in any class.
    knight.characterDataId = 79;
    rules.unrestrictedCharacters = {79};
    const std::vector<RunCard> freeCards = OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, 1, 0);
    REQUIRE_EQ(freeCards.size(), 1u);
    CHECK(freeCards[0].mastery == SkillMastery::Master);
    rules.skills = {"FireMagic"};
    CHECK_EQ(OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, 1, 0).size(), 1u);
    knight.characterDataId = 1;
    CHECK(OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, 1, 0).empty());
    CHECK_EQ(knight.eventVariables[OpenYAMM::Game::RunCardsTakenVariable], 2);
    CHECK_EQ(OpenYAMM::Game::runSkillLevelForMastery(SkillMastery::Master), 7u);
    CHECK_EQ(OpenYAMM::Game::runSkillLevelForMastery(SkillMastery::Grandmaster), 10u);
}

TEST_CASE("a run's talent card adds its rank under the talent's id and ends at the last rank")
{
    DungeonRunRules rules;
    rules.talents = {
        {.id = "might", .name = "Might +15", .stat = "might", .amount = 15, .maxRank = 2},
        {.id = "boom", .name = "Explosive arrows", .stat = "explosive_arrows", .maxRank = 1, .needsSkill = "Bow"},
    };
    Character hero = testHero("Knight");

    for (int rank = 1; rank <= 2; ++rank)
    {
        const std::vector<RunCard> cards = OpenYAMM::Game::dealRunCards(rules, hero, nullptr, 3, 0);
        REQUIRE_EQ(cards.size(), 1u);
        CHECK_EQ(cards[0].id, "talent:might");
        CHECK_EQ(cards[0].rank, rank);
        CHECK_EQ(cards[0].maxRank, 2);
        OpenYAMM::Game::applyRunCard(rules, hero, cards[0], nullptr, 3, 0, {});
        CHECK_EQ(hero.permanentBonuses.might, 15 * rank);
        CHECK_EQ(OpenYAMM::Game::runTalentRank(hero, "might"), rank);
    }

    CHECK(OpenYAMM::Game::dealRunCards(rules, hero, nullptr, 3, 0).empty());
    // The rank is kept by the id, not by the talent's place in run.yml: another order changes nothing.
    std::swap(rules.talents[0], rules.talents[1]);
    CHECK(OpenYAMM::Game::dealRunCards(rules, hero, nullptr, 3, 0).empty());
    CHECK_EQ(OpenYAMM::Game::runTalentRank(hero, "might"), 2);
    // A card of a rank the hero already has adds nothing.
    RunCard again = {};
    again.kind = RunCard::Kind::Talent;
    again.talent = "might";
    again.rank = 2;
    OpenYAMM::Game::applyRunCard(rules, hero, again, nullptr, 3, 0, {});
    CHECK_EQ(hero.permanentBonuses.might, 30);
    CHECK_EQ(OpenYAMM::Game::runTalentRank(hero, "might"), 2);
    // With a bow the arrows' talent is offered, and taking it sets the hero's bit.
    hero.skills["Bow"] = {"Bow", 1, SkillMastery::Normal};
    const std::vector<RunCard> cards = OpenYAMM::Game::dealRunCards(rules, hero, nullptr, 3, 0);
    REQUIRE_EQ(cards.size(), 1u);
    CHECK_EQ(cards[0].label, "Explosive arrows");
    OpenYAMM::Game::applyRunCard(rules, hero, cards[0], nullptr, 3, 0, {});
    CHECK(hero.playerBits.contains(OpenYAMM::Game::RunExplosiveArrowsPlayerBit));
    CHECK_EQ(OpenYAMM::Game::runTalentRank(hero, "boom"), 1);
}

TEST_CASE("a run's talent is offered only to a hero with the skill it needs at its mastery")
{
    using OpenYAMM::Game::DungeonRunTalent;
    Character hero = testHero("Knight");
    const DungeonRunTalent free = {.id = "free"};
    const DungeonRunTalent bow = {.id = "bow", .needsSkill = "Bow", .needsMastery = SkillMastery::Expert};
    const DungeonRunTalent melee = {.id = "melee", .needsSkill = "melee", .needsMastery = SkillMastery::Expert};
    const DungeonRunTalent magic = {.id = "magic", .needsSkill = "magic", .needsMastery = SkillMastery::Master};

    CHECK(OpenYAMM::Game::runTalentAvailable(free, hero));
    CHECK_FALSE(OpenYAMM::Game::runTalentAvailable(bow, hero));
    CHECK_FALSE(OpenYAMM::Game::runTalentAvailable(melee, hero));
    CHECK_FALSE(OpenYAMM::Game::runTalentAvailable(magic, hero));
    hero.skills["Bow"] = {"Bow", 1, SkillMastery::Normal};
    CHECK_FALSE(OpenYAMM::Game::runTalentAvailable(bow, hero));
    hero.skills["Bow"] = {"Bow", 4, SkillMastery::Expert};
    CHECK(OpenYAMM::Game::runTalentAvailable(bow, hero));
    // Any melee weapon skill or Armsmaster stands for "melee", any school for "magic": the best of them counts.
    hero.skills["Sword"] = {"Sword", 1, SkillMastery::Normal};
    hero.skills["Armsmaster"] = {"Armsmaster", 4, SkillMastery::Expert};
    CHECK(OpenYAMM::Game::runTalentAvailable(melee, hero));
    hero.skills["FireMagic"] = {"FireMagic", 4, SkillMastery::Expert};
    CHECK_FALSE(OpenYAMM::Game::runTalentAvailable(magic, hero));
    hero.skills["BodyMagic"] = {"BodyMagic", 7, SkillMastery::Master};
    CHECK(OpenYAMM::Game::runTalentAvailable(magic, hero));
    CHECK(OpenYAMM::Game::runSkillGroupMastery(hero, "magic") == SkillMastery::Master);
    CHECK(OpenYAMM::Game::runSkillGroupMastery(hero, "Mace") == SkillMastery::None);
}

TEST_CASE("run.yml talents: numbers by rank, needs and the errors of a broken table")
{
    const std::string head = "dungeons:\n  - {map: a.blv, entrance: {x: 0, y: 0, z: 0}}\nrules:\n  route: [0]\n";
    std::string error;
    const std::optional<DungeonRunDefinition> run = OpenYAMM::Game::parseDungeonRunDefinition(
        head
            + "  levels: {perStop: [2], offers: 5, heal: 0.5}\n"
            + "  camp: {restFood: 1}\n"
            + "  talents:\n"
            + "    - {id: fan, name: Fan, needs: Bow, mastery: expert, maxRank: 3, values: [2, 4, 6], share: 50,\n"
            + "       arc: 12, icon: talent_multishot, text: \"{value} more arrows\"}\n"
            + "    - {id: guard, needs: melee, values: [15], values2: [1, 2], seconds: 3, cooldown: 20, radius: 500}\n",
        error);
    REQUIRE_MESSAGE(run.has_value(), error);
    const DungeonRunRules &rules = *run->rules;
    CHECK_EQ(rules.cardOffers, 5);
    CHECK_EQ(rules.levelUpHealShare, doctest::Approx(0.5));
    CHECK_EQ(rules.campRestFood, 1);
    REQUIRE(rules.talent("fan") != nullptr);
    const OpenYAMM::Game::DungeonRunTalent &fan = *rules.talent("fan");
    CHECK_EQ(fan.needsSkill, "Bow");
    CHECK(fan.needsMastery == SkillMastery::Expert);
    CHECK_EQ(fan.maxRank, 3);
    CHECK_EQ(fan.value(0), doctest::Approx(0));
    CHECK_EQ(fan.value(1), doctest::Approx(2));
    CHECK_EQ(fan.value(3), doctest::Approx(6));
    // A rank past the list reads the last number.
    CHECK_EQ(fan.value(7), doctest::Approx(6));
    CHECK_EQ(fan.share, doctest::Approx(50));
    CHECK_EQ(fan.arc, doctest::Approx(12));
    CHECK_EQ(fan.icon, "talent_multishot");
    const OpenYAMM::Game::DungeonRunTalent &guard = *rules.talent("guard");
    CHECK_EQ(guard.needsSkill, "melee");
    CHECK(guard.needsMastery == SkillMastery::Normal);
    CHECK_EQ(guard.name, "guard");
    CHECK_EQ(guard.value2(2), doctest::Approx(2));
    CHECK_EQ(guard.seconds, doctest::Approx(3));
    CHECK_EQ(guard.cooldown, doctest::Approx(20));
    CHECK_EQ(guard.radius, doctest::Approx(500));
    CHECK(rules.talent("none") == nullptr);

    // Without the keys the defaults stand: four offers, a quarter of the hit points, two food.
    const std::optional<DungeonRunDefinition> bare =
        OpenYAMM::Game::parseDungeonRunDefinition(head + "  levels: {perStop: [2]}\n", error);
    REQUIRE_MESSAGE(bare.has_value(), error);
    CHECK_EQ(bare->rules->cardOffers, 4);
    CHECK_EQ(bare->rules->levelUpHealShare, doctest::Approx(0.25));
    CHECK_EQ(bare->rules->campRestFood, 2);

    for (const char *pBroken : {
             "  talents:\n    - {id: a}\n    - {id: a}\n",
             "  talents:\n    - {name: Nameless}\n",
             "  talents:\n    - {id: a, needs: NoSuchSkill}\n",
             "  talents:\n    - {id: a, needs: Bow, mastery: supreme}\n",
             "  shops:\n    - {afterStop: 1, name: Shop, house: 5, stock: [{count: 3}]}\n",
             "  rewards:\n    guardians:\n      - {item: sword}\n"})
    {
        CHECK_FALSE(OpenYAMM::Game::parseDungeonRunDefinition(head + pBroken, error).has_value());
        CHECK_FALSE(error.empty());
    }
}

TEST_CASE("a run's level-up heals: a quarter of the maximum, a hero who is out comes round, the dead stay dead")
{
    using OpenYAMM::Game::CharacterCondition;
    using OpenYAMM::Game::Party;
    DungeonRunRules rules;
    rules.levelsPerLevelUp = 4;
    rules.levelUpHealShare = 0.25;
    Party party;
    Party::Snapshot snapshot = {};

    for (int index = 0; index < 5; ++index)
    {
        // The maximum as the game works it out (without a class table: the engine's fallback by level).
        Character hero = testHero("Knight");
        OpenYAMM::Game::GameMechanics::refreshCharacterBaseResources(hero, true, nullptr);
        snapshot.members.push_back(hero);
    }

    party.restoreSnapshot(snapshot);
    REQUIRE_EQ(party.members().size(), 5u);
    const auto set = [&party](size_t index, int health, std::initializer_list<CharacterCondition> conditions)
    {
        Character &hero = *party.member(index);
        hero.health = health;

        for (CharacterCondition condition : conditions)
        {
            hero.conditions.set(static_cast<size_t>(condition));
        }
    };
    const int maximum = Party::effectiveMaximumHealth(*party.member(0));
    REQUIRE(maximum >= 8);
    const int quarter = (maximum + 3) / 4;
    // Hurt, out below zero, nearly whole, dead, petrified.
    set(0, 10, {});
    set(1, -30, {CharacterCondition::Unconscious});
    set(2, maximum - 1, {});
    set(3, 0, {CharacterCondition::Dead});
    set(4, 40, {CharacterCondition::Petrified});

    OpenYAMM::Game::grantRunLevelUp(rules, party);
    // The levels raise the maximum; the share is of the new one.
    const int raised = Party::effectiveMaximumHealth(*party.member(0));
    const int share = (raised * 25 + 99) / 100;
    CHECK(raised > maximum);
    CHECK(share >= quarter);
    CHECK_EQ(party.member(0)->health, 10 + share);
    // Below zero: up to the share, awake.
    CHECK_EQ(party.member(1)->health, share);
    CHECK_FALSE(party.member(1)->conditions.test(static_cast<size_t>(CharacterCondition::Unconscious)));
    // Never above the maximum.
    CHECK_EQ(party.member(2)->health, std::min(raised, maximum - 1 + share));
    CHECK(party.member(2)->health <= raised);
    // The dead and the petrified are left as they are, but level up and are owed their card like everyone.
    CHECK_EQ(party.member(3)->health, 0);
    CHECK(party.member(3)->conditions.test(static_cast<size_t>(CharacterCondition::Dead)));
    CHECK_EQ(party.member(4)->health, 40);
    CHECK(party.member(4)->conditions.test(static_cast<size_t>(CharacterCondition::Petrified)));

    for (size_t index = 0; index < 5; ++index)
    {
        CHECK_EQ(party.member(index)->level, 5u);
        CHECK_EQ(party.member(index)->eventVariables[OpenYAMM::Game::RunPendingCardsVariable], 1);
    }

    // A share of nothing heals nothing.
    rules.levelUpHealShare = 0.0;
    set(0, 5, {});
    OpenYAMM::Game::grantRunLevelUp(rules, party);
    CHECK_EQ(party.member(0)->health, 5);
}

TEST_CASE("a run saved before stable ids: perk levels move to the perks' ids once, and nothing is added twice")
{
    using OpenYAMM::Game::migrateLegacyRunPerks;
    using OpenYAMM::Game::RunLegacyPerkLevelVariableBase;
    using OpenYAMM::Game::runTalentRank;
    // By hand: every perk of the first version's run.yml, by its index there.
    Character hero = testHero("Archer");
    const std::vector<std::string> ids = {"might", "endurance", "accuracy", "speed", "intellect", "personality",
        "health", "mana", "armor", "resistances", "melee_damage", "ranged_damage", "explosive_arrows"};

    for (size_t index = 0; index < ids.size(); ++index)
    {
        hero.eventVariables[static_cast<uint16_t>(RunLegacyPerkLevelVariableBase + index)] =
            static_cast<int32_t>(index % 3 + 1);
    }

    hero.eventVariables[OpenYAMM::Game::RunCardsTakenVariable] = 7;
    hero.eventVariables[OpenYAMM::Game::RunPendingCardsVariable] = 2;
    hero.permanentBonuses.might = 15;
    hero.playerBits.insert(OpenYAMM::Game::RunExplosiveArrowsPlayerBit);
    CHECK(migrateLegacyRunPerks(hero));

    for (size_t index = 0; index < ids.size(); ++index)
    {
        CHECK_EQ(runTalentRank(hero, ids[index]), static_cast<int>(index % 3 + 1));
        CHECK_FALSE(hero.eventVariables.contains(static_cast<uint16_t>(RunLegacyPerkLevelVariableBase + index)));
    }

    // The amounts were in the hero already; the counts of cards are kept.
    CHECK_EQ(hero.permanentBonuses.might, 15);
    CHECK_EQ(hero.eventVariables[OpenYAMM::Game::RunCardsTakenVariable], 7);
    CHECK_EQ(hero.eventVariables[OpenYAMM::Game::RunPendingCardsVariable], 2);
    // Once: a second pass finds nothing, and a hero of a new run has nothing to move.
    CHECK_FALSE(migrateLegacyRunPerks(hero));
    Character fresh = testHero("Knight");
    CHECK_FALSE(migrateLegacyRunPerks(fresh));
    CHECK(fresh.talentRanks.empty());
    // A perk at its last level is not offered again; the arrows neither.
    DungeonRunRules rules = *installedRulesRun().rules;
    hero.skills["Bow"] = {"Bow", 1, SkillMastery::Normal};

    for (uint64_t seed = 0; seed < 30; ++seed)
    {
        for (const RunCard &card : OpenYAMM::Game::dealRunCards(rules, hero, nullptr, seed, 0))
        {
            CHECK(card.talent != "accuracy");
            CHECK(card.talent != "explosive_arrows");

            if (card.talent == "might")
            {
                CHECK_EQ(card.rank, 2);
            }
        }
    }
}

TEST_CASE("Minotaur frozen 2.4 v184 run keeps its perks under stable ids and plays on")
{
    using namespace OpenYAMM::Game;
    const std::filesystem::path fixture = std::filesystem::path(OPENYAMM_SOURCE_DIR)
        / "tests/fixtures/minotaur-2.4-v184-roguelike-perks.oysav";
    std::string error;
    std::optional<GameSaveData> save = loadGameDataFromPath(fixture, error);
    REQUIRE_MESSAGE(save, error);
    CHECK(save->sourceSaveVersion == 183);
    CHECK(save->mapFileName == "6d01.blv");
    CHECK(save->ruleset.id == "roguelike");
    REQUIRE(save->party.members.size() == 4);
    RoguelikeRunState state;
    readRunState(save->namedGlobalVars, state);
    CHECK(state.entered == 1);
    CHECK(state.levelUps == 3);
    CHECK(state.guardian == RoguelikeRunState::GuardianAlive);
    // No guardian was slain yet in that save, and the old version gave no rewards.
    CHECK(state.rewarded == 0);

    std::vector<Character> &heroes = save->party.members;
    // As the old writer left it: levels by index (1 endurance, 6 health, 7 mana, 5 personality, 3 speed, 8 armor).
    const std::vector<std::vector<std::pair<int, int>>> oldLevels = {
        {{1, 1}, {6, 1}}, {{7, 1}, {5, 1}}, {{3, 2}}, {{8, 1}}};

    for (size_t index = 0; index < heroes.size(); ++index)
    {
        CHECK(heroes[index].talentRanks.empty());

        for (const auto &[perk, level] : oldLevels[index])
        {
            CHECK_EQ(heroes[index].eventVariables[static_cast<uint16_t>(RunLegacyPerkLevelVariableBase + perk)],
                level);
        }
    }

    const std::vector<CharacterStatBonuses> before = {heroes[0].permanentBonuses, heroes[1].permanentBonuses,
        heroes[2].permanentBonuses, heroes[3].permanentBonuses};
    // The amounts the old cards added, once each.
    CHECK_EQ(before[0].endurance, 15);
    CHECK_EQ(before[0].maxHealth, 30);
    CHECK_EQ(before[1].maxSpellPoints, 25);
    CHECK_EQ(before[1].personality, 15);
    CHECK_EQ(before[2].speed, 30);
    CHECK_EQ(before[3].armorClass, 8);

    for (Character &hero : heroes)
    {
        CHECK(migrateLegacyRunPerks(hero));
        CHECK_FALSE(migrateLegacyRunPerks(hero));
    }

    const std::vector<std::unordered_map<std::string, uint16_t>> ranks = {
        {{"endurance", 1}, {"health", 1}},
        {{"mana", 1}, {"personality", 1}},
        {{"speed", 2}},
        {{"armor", 1}}};

    for (size_t index = 0; index < heroes.size(); ++index)
    {
        const Character &hero = heroes[index];
        CHECK(hero.talentRanks == ranks[index]);
        // Nothing doubled, nothing lost.
        CHECK(bonusNumbers(hero.permanentBonuses) == bonusNumbers(before[index]));
        CHECK_EQ(hero.level, 13u);

        for (const auto &[key, value] : hero.eventVariables)
        {
            CHECK_FALSE((key >= RunLegacyPerkLevelVariableBase && key < RunLegacyPerkLevelVariableBase + 64));
        }
    }

    // The skill cards and the counts of cards are as they were: Zoltan is still owed his third.
    CHECK(heroes[0].findSkillByCanonicalName("LeatherArmor")->mastery == SkillMastery::Expert);
    CHECK(heroes[1].findSkillByCanonicalName("Bow")->mastery == SkillMastery::Expert);
    CHECK(heroes[2].findSkillByCanonicalName("BodyMagic")->mastery == SkillMastery::Expert);
    CHECK(heroes[3].findSkillByCanonicalName("Bow")->mastery == SkillMastery::Normal);

    for (size_t index = 0; index < heroes.size(); ++index)
    {
        CHECK_EQ(heroes[index].eventVariables[RunCardsTakenVariable], index == 3 ? 2 : 3);
        CHECK_EQ(heroes[index].eventVariables[RunPendingCardsVariable], index == 3 ? 1 : 0);
    }

    // The owed card comes in the new form: four different cards, the same after another load.
    const DungeonRunRules rules = *installedRulesRun().rules;
    const std::vector<RunCard> offer = dealRunCards(rules, heroes[3], nullptr, save->randomRootSeed, 3);
    CHECK(offer == dealRunCards(rules, heroes[3], nullptr, save->randomRootSeed, 3));

    // The migrated run is written and read back as it is.
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "minotaur-v184-run-ids.oysav";
    REQUIRE_MESSAGE(saveGameDataToPath(path, *save, error), error);
    const std::optional<GameSaveData> restored = loadGameDataFromPath(path, error);
    REQUIRE_MESSAGE(restored, error);
    REQUIRE(restored->party.members.size() == 4);

    for (size_t index = 0; index < heroes.size(); ++index)
    {
        CHECK(restored->party.members[index].talentRanks == ranks[index]);
        CHECK(restored->party.members[index].eventVariables == heroes[index].eventVariables);
        CHECK(bonusNumbers(restored->party.members[index].permanentBonuses) == bonusNumbers(before[index]));
    }

    std::filesystem::remove(path);
}

TEST_CASE("a run's school card owes real books: the weakest attack spell first, none known or already carried")
{
    DungeonRunRules rules = *installedRulesRun().rules;
    Character sorcerer = testHero("Sorcerer");
    RunCard card = {};
    card.kind = RunCard::Kind::Skill;
    card.skill = "FireMagic";
    card.mastery = SkillMastery::Normal;

    // A new school: three books of the first four spells, Fire Bolt among them.
    std::vector<uint32_t> books = OpenYAMM::Game::applyRunCard(rules, sorcerer, card, nullptr, 9, 3, {});
    REQUIRE_EQ(books.size(), 3u);
    CHECK_EQ(books[0], 2u);
    CHECK(std::set<uint32_t>(books.begin(), books.end()).size() == 3);

    for (uint32_t spellId : books)
    {
        CHECK(spellId >= 1);
        CHECK(spellId <= 4);
        sorcerer.knownSpellIds.insert(spellId);
    }

    // Expert: two more, from the spells Expert opened (5-7) before the one left of the first four; a book the party
    // carries is skipped.
    card.mastery = SkillMastery::Expert;
    books = OpenYAMM::Game::applyRunCard(
        rules, sorcerer, card, nullptr, 9, 3, [](uint32_t spellId) { return spellId == 6; });
    REQUIRE_EQ(books.size(), 2u);

    for (uint32_t spellId : books)
    {
        CHECK((spellId == 5 || spellId == 7));
    }

    CHECK(sorcerer.findSkillByCanonicalName("FireMagic")->mastery == SkillMastery::Expert);
    CHECK_EQ(sorcerer.findSkillByCanonicalName("FireMagic")->level, 4u);
}

TEST_CASE("a run's numbers go to the save's variables and back")
{
    RoguelikeRunState state;
    state.entered = 3;
    state.levelUps = 7;
    state.guardian = RoguelikeRunState::GuardianAlive;
    state.guardianActor = 212;
    state.doorOffered = 1;
    state.promoted = 1;
    state.route = {OpenYAMM::Game::dungeonRunMapKey("7d06.blv"), OpenYAMM::Game::dungeonRunMapKey("6t1.blv"),
        OpenYAMM::Game::dungeonRunMapKey("d11.blv"), OpenYAMM::Game::dungeonRunMapKey("6d17.blv")};
    std::unordered_map<std::string, int32_t> variables;
    OpenYAMM::Game::writeRunState(state, variables);
    CHECK_EQ(variables.at(OpenYAMM::Game::DungeonRunEnteredVariable), 3);
    CHECK_EQ(variables.at("run.route.count"), 4);
    RoguelikeRunState restored;
    OpenYAMM::Game::readRunState(variables, restored);
    CHECK(restored == state);
    OpenYAMM::Game::readRunState({}, restored);
    CHECK(restored == RoguelikeRunState{});

    // The route names the run's dungeons whatever the order of run.yml.
    const DungeonRunDefinition run = installedRulesRun();
    REQUIRE(OpenYAMM::Game::runRouteStop(run, state, 3) != nullptr);
    CHECK_EQ(OpenYAMM::Game::runRouteStop(run, state, 0)->mapFileName, "7d06.blv");
    CHECK_EQ(OpenYAMM::Game::runRouteStop(run, state, 3)->mapFileName, "6d17.blv");
    CHECK(OpenYAMM::Game::runRouteStop(run, state, 4) == nullptr);
}

TEST_CASE("a run's stop counts its own clearing: monsters dead when the party arrived are not the party's")
{
    MapKillProgress map;
    map.countedMonsters = 40;
    map.slainMonsters = 9;
    map.countedHealth = 7601;
    map.slainHealth = 1145;
    RoguelikeRunState state;
    state.entered = 1;
    // Not taken yet: the map as it is.
    state.arrivalStop = 0;
    CHECK_EQ(OpenYAMM::Game::runStopProgress(state, map).slainHealth, doctest::Approx(1145));
    // Taken on arrival (MM9's Inventa Storca: nine corpses of its first fill): the stop starts at nothing slain.
    state.arrivalStop = 1;
    state.arrivalSlainMonsters = 9;
    state.arrivalSlainHealth = 1145;
    const MapKillProgress stop = OpenYAMM::Game::runStopProgress(state, map);
    CHECK_EQ(stop.slainHealth, doctest::Approx(0));
    CHECK_EQ(stop.countedHealth, doctest::Approx(6456));
    CHECK_EQ(stop.slainMonsters, 0);
    CHECK_EQ(stop.countedMonsters, 31);
    const DungeonRunRules rules = *installedRulesRun().rules;
    CHECK_EQ(OpenYAMM::Game::runClearFraction(rules, stop), doctest::Approx(0));
    CHECK_EQ(OpenYAMM::Game::runLevelUpsDue(rules, 0, stop), 0);
    // The next stop's arrival is its own: an old stop's numbers never apply to it.
    state.entered = 2;
    CHECK_EQ(OpenYAMM::Game::runStopProgress(state, map).slainHealth, doctest::Approx(1145));

    // Saved and read back; a save from before arrivals were taken reads its stop under way as taken, with none.
    std::unordered_map<std::string, int32_t> variables;
    state.entered = 1;
    OpenYAMM::Game::writeRunState(state, variables);
    RoguelikeRunState restored;
    OpenYAMM::Game::readRunState(variables, restored);
    CHECK(restored == state);
    variables.erase("run.arrival_stop");
    variables.erase("run.arrival_slain");
    variables.erase("run.arrival_slain_health");
    OpenYAMM::Game::readRunState(variables, restored);
    CHECK_EQ(restored.arrivalStop, 1);
    CHECK_EQ(restored.arrivalSlainHealth, 0);
    CHECK_EQ(OpenYAMM::Game::runStopProgress(restored, map).slainHealth, doctest::Approx(1145));
}

TEST_CASE("a kept route whose map a later pool lost draws that stop anew and keeps the others")
{
    const DungeonRunDefinition run = installedRulesRun();
    RoguelikeRunState state;
    state.entered = 2;
    state.route = {OpenYAMM::Game::dungeonRunMapKey("6d01.blv"), OpenYAMM::Game::dungeonRunMapKey("6t3.blv"),
        OpenYAMM::Game::dungeonRunMapKey("no_such_map.blv"), OpenYAMM::Game::dungeonRunMapKey("6d17.blv")};
    REQUIRE(OpenYAMM::Game::ensureRunRoute(run, 4, state));
    REQUIRE_EQ(state.route.size(), 4u);
    CHECK_EQ(OpenYAMM::Game::runRouteStop(run, state, 0)->mapFileName, "6d01.blv");
    CHECK_EQ(OpenYAMM::Game::runRouteStop(run, state, 1)->mapFileName, "6t3.blv");
    REQUIRE(OpenYAMM::Game::runRouteStop(run, state, 2) != nullptr);
    CHECK(OpenYAMM::Game::dungeonRunStopAtStage(*OpenYAMM::Game::runRouteStop(run, state, 2), 3));
    CHECK_EQ(OpenYAMM::Game::runRouteStop(run, state, 3)->mapFileName, "6d17.blv");
    // Whole now: nothing more to change.
    CHECK_FALSE(OpenYAMM::Game::ensureRunRoute(run, 4, state));
}

TEST_CASE("a run saved by 2.4 keeps the route that version drew; a new run draws its own")
{
    const DungeonRunDefinition run = installedRulesRun();
    // A 2.4 save: in its second dungeon, no route kept.
    RoguelikeRunState state;
    state.entered = 2;
    REQUIRE(OpenYAMM::Game::ensureRunRoute(run, 4, state));
    REQUIRE_EQ(state.route.size(), 4u);
    const std::vector<std::string> expected = {"6d01.blv", "6t3.blv", "6d12.blv", "6d07.blv"};

    for (size_t stop = 0; stop < expected.size(); ++stop)
    {
        REQUIRE(OpenYAMM::Game::runRouteStop(run, state, stop) != nullptr);
        CHECK_EQ(OpenYAMM::Game::runRouteStop(run, state, stop)->mapFileName, expected[stop]);
    }

    // Once kept, it stays; a game that is no run under way gets none.
    CHECK_FALSE(OpenYAMM::Game::ensureRunRoute(run, 4, state));
    RoguelikeRunState notStarted;
    CHECK_FALSE(OpenYAMM::Game::ensureRunRoute(run, 4, notStarted));
    CHECK(notStarted.route.empty());

    // A new run: the seed's draw, a stage per stop.
    const std::vector<int32_t> drawn = OpenYAMM::Game::drawRunRoute(run, 4);
    REQUIRE_EQ(drawn.size(), 4u);
    RoguelikeRunState fresh;
    fresh.route = drawn;

    for (size_t stop = 0; stop < drawn.size(); ++stop)
    {
        REQUIRE(OpenYAMM::Game::runRouteStop(run, fresh, stop) != nullptr);
        const DungeonRunStop &dungeon = *OpenYAMM::Game::runRouteStop(run, fresh, stop);
        CHECK(OpenYAMM::Game::dungeonRunStopAtStage(dungeon, run.rules->route[stop]));
    }
}

TEST_CASE("Minotaur frozen 2.3 v180 preserves the Roguelike run when migrated to 2.4")
{
    using namespace OpenYAMM::Game;
    const std::filesystem::path fixture = std::filesystem::path(OPENYAMM_SOURCE_DIR)
        / "tests/fixtures/minotaur-2.3-v180-roguelike.oysav";
    std::string error;
    const std::optional<GameSaveData> original = loadGameDataFromPath(fixture, error);
    REQUIRE_MESSAGE(original, error);
    CHECK(original->sourceSaveVersion == 179);
    CHECK(original->mapFileName == "6d01.blv");
    CHECK(original->ruleset.id == "roguelike");
    CHECK_FALSE(original->ruleset.options.soloRun);
    REQUIRE(original->party.members.size() == 4);
    RoguelikeRunState state;
    readRunState(original->namedGlobalVars, state);
    CHECK(state.entered == 1);
    CHECK(state.levelUps == 3);
    CHECK(state.guardian == RoguelikeRunState::GuardianAlive);
    const std::optional<GameSaveSummary> summary = loadGameSaveSummaryFromPath(fixture, error);
    REQUIRE_MESSAGE(summary, error);
    CHECK(summary->ruleset == original->ruleset);
    CHECK(summary->mapFileName == original->mapFileName);

    const std::filesystem::path path = std::filesystem::temp_directory_path() / "minotaur-v180-to-v202.oysav";
    REQUIRE_MESSAGE(saveGameDataToPath(path, *original, error), error);
    const std::optional<GameSaveData> restored = loadGameDataFromPath(path, error);
    REQUIRE_MESSAGE(restored, error);
    // This version writes OYMINO1/v202, upstream 3.2's schema 201.
    CHECK(restored->sourceSaveVersion == 201);
    CHECK(restored->namedGlobalVars == original->namedGlobalVars);
    CHECK(restored->ruleset == original->ruleset);
    CHECK(restored->randomRootSeed == original->randomRootSeed);
    REQUIRE(restored->party.members.size() == original->party.members.size());
    for (size_t index = 0; index < original->party.members.size(); ++index)
    {
        const Character &before = original->party.members[index];
        const Character &after = restored->party.members[index];
        CHECK(after.level == 13);
        CHECK(after.eventVariables == before.eventVariables);
        CHECK(after.inventory == before.inventory);
        CHECK(after.equipment == before.equipment);
        CHECK(after.knownSpellIds == before.knownSpellIds);
        CHECK(after.characterDataId == before.characterDataId);
        CHECK(after.portraitTextureName == before.portraitTextureName);
        CHECK(after.actionBar == before.actionBar);
        CHECK(after.talentRanks == before.talentRanks);
    }
    std::filesystem::remove(path);
}
