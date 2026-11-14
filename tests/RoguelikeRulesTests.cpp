#include "doctest/doctest.h"

#include "game/content/DungeonRun.h"
#include "game/party/Party.h"
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

MapKillProgress progressOf(double slain, double counted)
{
    MapKillProgress progress;
    progress.slainHealth = slain;
    progress.countedHealth = counted;
    return progress;
}
}

TEST_CASE("the installed run's rules: a route of four by tier, ten level-ups and cards to deal")
{
    const DungeonRunDefinition run = installedRulesRun();
    const DungeonRunRules &rules = *run.rules;

    CHECK_EQ(rules.route, std::vector<int>{1, 2, 3, 3});
    CHECK_EQ(rules.levelUps.size(), rules.route.size());
    CHECK_EQ(rules.totalLevelUps(), 10);
    CHECK_EQ(rules.levelsPerLevelUp, 4);
    CHECK_EQ(rules.clearShare, doctest::Approx(0.5));
    CHECK_EQ(rules.guardianHealthScale, doctest::Approx(4.0));
    CHECK(rules.skills.size() >= 10);
    CHECK(rules.perks.size() >= 6);
    REQUIRE(rules.school("FireMagic") != nullptr);
    CHECK_EQ(rules.school("FireMagic")->combatSpell, 2u);
    CHECK(rules.school("Sword") == nullptr);

    std::set<std::string> lastStops;

    for (uint64_t seed = 0; seed < 60; ++seed)
    {
        const std::vector<size_t> order = OpenYAMM::Game::dungeonRunOrder(run, seed);
        REQUIRE_EQ(order.size(), 4u);
        CHECK_EQ(std::set<size_t>(order.begin(), order.end()).size(), 4u);

        for (size_t stop = 0; stop < order.size(); ++stop)
        {
            CHECK_EQ(run.dungeons[order[stop]].tier, rules.route[stop]);
        }

        CHECK_EQ(OpenYAMM::Game::dungeonRunStop(run, seed, 0)->mapFileName, "6d01.blv");
        CHECK(OpenYAMM::Game::dungeonRunStop(run, seed, 4) == nullptr);
        lastStops.insert(run.dungeons[order[3]].mapFileName);
    }

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

TEST_CASE("a run's cards are three different ones, fixed by the seed, within the class caps")
{
    DungeonRunRules rules = *installedRulesRun().rules;
    const ClassSkillTable classSkills = testClassSkills();
    Character knight = testHero("Knight");
    knight.skills["Sword"] = {"Sword", 1, SkillMastery::Normal};

    for (uint64_t seed = 0; seed < 40; ++seed)
    {
        const std::vector<RunCard> cards = OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, seed, 0);
        REQUIRE_EQ(cards.size(), 3u);
        CHECK(cards == OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, seed, 0));
        CHECK(std::set<std::string>{cards[0].label, cards[1].label, cards[2].label}.size() == 3);
        // The first is a step of the one skill the knight knows, the second a perk.
        CHECK(cards[0].kind == RunCard::Kind::Skill);
        CHECK_EQ(cards[0].skill, "Sword");
        CHECK(cards[0].mastery == SkillMastery::Expert);
        CHECK(cards[1].kind == RunCard::Kind::Perk);

        for (const RunCard &card : cards)
        {
            // No magic for a knight, no explosive arrows without a bow.
            CHECK(card.skill != "FireMagic");
            CHECK(card.skill != "AirMagic");

            if (card.kind == RunCard::Kind::Perk)
            {
                CHECK(rules.perks[card.perkIndex].requiresSkill.empty());
            }
        }
    }

    // Another hero's place and another count of cards taken draw differently somewhere.
    size_t different = 0;

    for (uint64_t seed = 0; seed < 40; ++seed)
    {
        different += OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, seed, 0)
            != OpenYAMM::Game::dealRunCards(rules, knight, &classSkills, seed, 1);
    }

    CHECK(different > 20);
}

TEST_CASE("a run's skill card raises one step with its level, and stops at the class cap")
{
    DungeonRunRules rules = *installedRulesRun().rules;
    rules.perks.clear();
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
    CHECK_EQ(knight.eventVariables[OpenYAMM::Game::RunCardsTakenVariable], 2);
    CHECK_EQ(OpenYAMM::Game::runSkillLevelForMastery(SkillMastery::Master), 7u);
    CHECK_EQ(OpenYAMM::Game::runSkillLevelForMastery(SkillMastery::Grandmaster), 10u);
}

TEST_CASE("a run's perk card adds its number, counts its level and ends at the top level")
{
    DungeonRunRules rules;
    rules.perks = {
        {.id = "might", .name = "Might +15", .stat = "might", .amount = 15, .maxLevel = 2},
        {.id = "boom", .name = "Explosive arrows", .stat = "explosive_arrows", .maxLevel = 1, .requiresSkill = "Bow"},
    };
    Character hero = testHero("Knight");

    for (int level = 1; level <= 2; ++level)
    {
        const std::vector<RunCard> cards = OpenYAMM::Game::dealRunCards(rules, hero, nullptr, 3, 0);
        REQUIRE_EQ(cards.size(), 1u);
        CHECK_EQ(cards[0].perkLevel, level);
        OpenYAMM::Game::applyRunCard(rules, hero, cards[0], nullptr, 3, 0, {});
        CHECK_EQ(hero.permanentBonuses.might, 15 * level);
    }

    CHECK(OpenYAMM::Game::dealRunCards(rules, hero, nullptr, 3, 0).empty());
    // With a bow the arrows' perk is offered, and taking it sets the hero's bit.
    hero.skills["Bow"] = {"Bow", 1, SkillMastery::Normal};
    const std::vector<RunCard> cards = OpenYAMM::Game::dealRunCards(rules, hero, nullptr, 3, 0);
    REQUIRE_EQ(cards.size(), 1u);
    CHECK_EQ(cards[0].label, "Explosive arrows");
    OpenYAMM::Game::applyRunCard(rules, hero, cards[0], nullptr, 3, 0, {});
    CHECK(hero.playerBits.contains(OpenYAMM::Game::RunExplosiveArrowsPlayerBit));
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
    std::unordered_map<std::string, int32_t> variables;
    OpenYAMM::Game::writeRunState(state, variables);
    CHECK_EQ(variables.at(OpenYAMM::Game::DungeonRunEnteredVariable), 3);
    RoguelikeRunState restored;
    OpenYAMM::Game::readRunState(variables, restored);
    CHECK(restored == state);
    OpenYAMM::Game::readRunState({}, restored);
    CHECK(restored == RoguelikeRunState{});
}
