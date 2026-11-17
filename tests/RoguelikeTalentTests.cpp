#include "doctest/doctest.h"

#include "game/content/DungeonRun.h"
#include "game/gameplay/HouseServiceRuntime.h"
#include "game/items/ItemGenerator.h"
#include "game/items/ItemRuntime.h"
#include "game/party/Party.h"
#include "game/rules/roguelike/RoguelikeRules.h"
#include "game/rules/roguelike/RoguelikeTalents.h"
#include "game/tables/SpellTable.h"
#include "game/ui/RunCardPresentation.h"
#include "tests/RegressionGameData.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace OpenYAMM::Game;

// The talents of a dungeon run (run.yml `talents`, game/rules/roguelike/RoguelikeTalents.h): the table the user
// approved on 2026-11-17, each effect's numbers, and the run's rules calling them from the shared hooks.

namespace
{
DungeonRunDefinition installedRun()
{
    std::ifstream input(
        std::filesystem::path(OPENYAMM_SOURCE_DIR) / "assets_dev/engine/rulesets/roguelike/run.yml");
    const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    std::string error;
    const std::optional<DungeonRunDefinition> run = parseDungeonRunDefinition(text, error);
    REQUIRE_MESSAGE(run.has_value(), error);
    REQUIRE(run->rules.has_value());
    return *run;
}

const OpenYAMM::Tests::RegressionGameData &data()
{
    REQUIRE_MESSAGE(OpenYAMM::Tests::regressionGameDataLoaded(), OpenYAMM::Tests::regressionGameDataFailure());
    return OpenYAMM::Tests::regressionGameData();
}

Character hero(const std::string &name = "Hero")
{
    Character member = {};
    member.name = name;
    member.className = "Knight";
    member.level = 10;
    member.maxHealth = 200;
    member.health = 200;
    member.might = member.intellect = member.personality = member.endurance = 15;
    member.speed = member.accuracy = member.luck = 15;
    return member;
}

Character heroWith(const std::string &talent, int rank)
{
    Character member = hero();
    member.talentRanks[talent] = static_cast<uint8_t>(rank);
    return member;
}

// A shield of the game's item table (items.txt 99, Trollish Shield).
constexpr uint32_t ShieldItemId = 99;

// A party of the run's rules with these heroes.
Party partyOf(const RoguelikeRules &rules, const std::vector<Character> &members)
{
    Party party;
    party.setItemTable(&data().itemTable);
    PartySeed seed;
    seed.members = members;
    // A seeded party plays by Standard's rules until its owner installs others, as the session does.
    party.seed(seed);
    party.setRules(&rules.gameRules());

    for (size_t index = 0; index < members.size(); ++index)
    {
        // The seed restores everyone; the tests' own ranks and equipment come after it.
        party.member(index)->talentRanks = members[index].talentRanks;
        party.member(index)->equipment = members[index].equipment;
    }

    party.refreshDerivedState();
    return party;
}
}

TEST_CASE("the installed run's talents: the approved table, no numeric card but the arrows'")
{
    const DungeonRunRules rules = *installedRun().rules;
    struct Expected
    {
        const char *pId;
        int maxRank;
        const char *pNeeds;
        SkillMastery mastery;
    };
    const std::vector<Expected> expected = {
        {"sweeping_blows", 3, "melee", SkillMastery::Expert},
        {"onslaught", 3, "", SkillMastery::Normal},
        {"shield_wall", 3, "Shield", SkillMastery::Expert},
        {"multishot", 3, "Bow", SkillMastery::Expert},
        {"piercing_arrows", 3, "Bow", SkillMastery::Expert},
        {"pinning_shot", 3, "Bow", SkillMastery::Expert},
        {"explosive_arrows", 1, "Bow", SkillMastery::Normal},
        {"attunement", 3, "magic", SkillMastery::Normal},
        {"overload", 3, "magic", SkillMastery::Expert},
        {"spell_surge", 3, "magic", SkillMastery::Master},
        {"binding_circle", 3, "magic", SkillMastery::Expert},
        {"resolve", 3, "", SkillMastery::Normal},
        {"bloodthirst", 3, "", SkillMastery::Normal},
        {"unbroken", 1, "", SkillMastery::Normal},
        {"cheat_death", 1, "", SkillMastery::Normal},
    };
    REQUIRE_EQ(rules.talents.size(), expected.size());

    for (const Expected &entry : expected)
    {
        const DungeonRunTalent *pTalent = rules.talent(entry.pId);
        REQUIRE_MESSAGE(pTalent != nullptr, entry.pId);
        CHECK_MESSAGE(pTalent->maxRank == entry.maxRank, entry.pId);
        CHECK_MESSAGE(pTalent->needsSkill == entry.pNeeds, entry.pId);
        CHECK_MESSAGE(pTalent->needsMastery == entry.mastery, entry.pId);
        // A card needs a name, an effect line and a picture of the talents' own set.
        CHECK_FALSE(pTalent->name.empty());
        CHECK_FALSE(pTalent->text.empty());
        CHECK_MESSAGE(pTalent->icon.starts_with("talent_"), entry.pId);
        // The numeric perks (characteristics, hit and spell points, armor class, flat damage) are gone.
        CHECK_MESSAGE((pTalent->stat.empty() || pTalent->stat == "explosive_arrows"), entry.pId);
        CHECK_EQ(pTalent->amount, 0);
    }

    for (const char *pGone : {"might", "endurance", "accuracy", "speed", "intellect", "personality", "health", "mana",
             "armor", "resistances", "melee_damage", "ranged_damage"})
    {
        CHECK_MESSAGE(rules.talent(pGone) == nullptr, pGone);
    }
}

TEST_CASE("a hero of an older run keeps the removed perks' ranks and is never offered them again")
{
    const DungeonRunRules rules = *installedRun().rules;
    Character archer = hero();
    archer.className = "Archer";
    archer.skills["Bow"] = {"Bow", 4, SkillMastery::Expert};
    archer.eventVariables[static_cast<uint16_t>(RunLegacyPerkLevelVariableBase + 0)] = 2;
    archer.eventVariables[static_cast<uint16_t>(RunLegacyPerkLevelVariableBase + 9)] = 1;
    archer.eventVariables[static_cast<uint16_t>(RunLegacyPerkLevelVariableBase + 12)] = 1;
    archer.permanentBonuses.might = 30;
    archer.permanentBonuses.resistances.fire = 10;
    REQUIRE(migrateLegacyRunPerks(archer));
    CHECK_EQ(runTalentRank(archer, "might"), 2);
    CHECK_EQ(runTalentRank(archer, "resistances"), 1);
    CHECK_EQ(runTalentRank(archer, "explosive_arrows"), 1);
    // The old amounts stay as they were; the removed perks have no effect of their own in the new rules.
    CHECK_EQ(archer.permanentBonuses.might, 30);
    CHECK_FALSE(RunTalents::learned(rules, archer, "might"));
    Character before = archer;
    RunTalents::applyPassives(rules, archer);
    CHECK_EQ(archer.magicalBonuses.resistances.fire, before.magicalBonuses.resistances.fire);
    std::set<std::string> offered;

    for (uint64_t seed = 0; seed < 200; ++seed)
    {
        for (const RunCard &card : dealRunCards(rules, archer, nullptr, seed, 0))
        {
            offered.insert(card.talent);
        }
    }

    CHECK_FALSE(offered.contains("might"));
    CHECK_FALSE(offered.contains("resistances"));
    // The arrows burst already: their talent, which stays in the rules, is at its last rank.
    CHECK_FALSE(offered.contains("explosive_arrows"));
    // The new talents the archer can take are offered: the bow's, and those that need nothing.
    CHECK(offered.contains("multishot"));
    CHECK(offered.contains("piercing_arrows"));
    CHECK(offered.contains("pinning_shot"));
    CHECK(offered.contains("resolve"));
    CHECK(offered.contains("cheat_death"));
    // No school of magic, no shield, no melee weapon skill at Expert: none of theirs.
    CHECK_FALSE(offered.contains("attunement"));
    CHECK_FALSE(offered.contains("spell_surge"));
    CHECK_FALSE(offered.contains("shield_wall"));
    CHECK_FALSE(offered.contains("sweeping_blows"));
}

TEST_CASE("Sweeping Blows, Multishot and Piercing (Chaining) Arrows: the extras of an ordinary attack by rank")
{
    const RoguelikeRules run(installedRun());
    const DungeonRunRules &rules = run.rules();
    const CombatRules &combat = run.gameRules().combat;
    // No talent: nothing, as under Standard.
    const CharacterAttackExtras none = combat.characterAttackExtras(hero());
    CHECK_EQ(none.sweepTargets, 0);
    CHECK_EQ(none.fanArrows, 0);
    CHECK_EQ(none.chainCount, 0);
    CHECK_EQ(standardGameRules().combat.characterAttackExtras(heroWith("sweeping_blows", 3)).sweepTargets, 0);

    for (int rank = 1; rank <= 3; ++rank)
    {
        const CharacterAttackExtras sweep = combat.characterAttackExtras(heroWith("sweeping_blows", rank));
        CHECK_EQ(sweep.sweepTargets, rank);
        CHECK_EQ(sweep.sweepShare, doctest::Approx(0.5));
        // 60 degrees either side of the view: a 120 degree arc.
        CHECK_EQ(sweep.sweepArcDegrees, doctest::Approx(60));
        CHECK_EQ(sweep.fanArrows, 0);

        const CharacterAttackExtras fan = combat.characterAttackExtras(heroWith("multishot", rank));
        CHECK_EQ(fan.fanArrows, 2 * rank);
        CHECK_EQ(fan.fanShare, doctest::Approx(0.5));
        CHECK_EQ(fan.fanArcDegrees, doctest::Approx(12));
        CHECK_EQ(fan.sweepTargets, 0);

        // OpenYAMM 2.7 made the piercing flight a chain (the same id): leaps to the nearest monster within 10 m.
        const CharacterAttackExtras chain = combat.characterAttackExtras(heroWith("piercing_arrows", rank));
        CHECK_EQ(chain.chainCount, rank);
        CHECK_EQ(chain.chainShare, doctest::Approx(0.5));
        CHECK_EQ(chain.chainRadius, doctest::Approx(1024));
    }

    // The talents add up in one hero.
    Character archer = heroWith("multishot", 2);
    archer.talentRanks["piercing_arrows"] = 3;
    const CharacterAttackExtras both = RunTalents::attackExtras(rules, archer);
    CHECK_EQ(both.fanArrows, 4);
    CHECK_EQ(both.chainCount, 3);
}

TEST_CASE("Onslaught: a kill makes the hero's weapon attacks faster for ten seconds")
{
    const RoguelikeRules run(installedRun());
    const DungeonRunRules &rules = run.rules();
    const std::vector<float> percent = {15.0f, 25.0f, 35.0f};

    for (int rank = 1; rank <= 3; ++rank)
    {
        Party party = partyOf(run, {heroWith("onslaught", rank), hero("Other")});
        Character &killer = *party.member(0);
        CHECK_EQ(RunTalents::attackRecoveryScale(rules, killer), doctest::Approx(1.0));
        // A hit that does not kill starts nothing.
        PartyHitOnMonster hit = {};
        hit.memberIndex = 0;
        hit.damage = 10;
        hit.melee = true;
        run.gameRules().combat.applyPartyHitEffects(party, hit);
        CHECK_EQ(RunTalents::attackRecoveryScale(rules, killer), doctest::Approx(1.0));
        hit.killed = true;
        run.gameRules().combat.applyPartyHitEffects(party, hit);
        const float scale = 1.0f / (1.0f + percent[static_cast<size_t>(rank - 1)] / 100.0f);
        CHECK_EQ(RunTalents::attackRecoveryScale(rules, killer), doctest::Approx(scale));

        // The attack's recovery is cut through the rules' attack hook.
        CharacterAttackResult attack = {};
        attack.recoverySeconds = 2.0f;
        CharacterAttackOnMonster context = {};
        context.pCharacter = &killer;
        run.gameRules().combat.resolveCharacterAttack(attack, context);
        CHECK_EQ(attack.recoverySeconds, doctest::Approx(2.0f * scale));
        // Another hero's attacks are as they were.
        attack.recoverySeconds = 2.0f;
        context.pCharacter = party.member(1);
        run.gameRules().combat.resolveCharacterAttack(attack, context);
        CHECK_EQ(attack.recoverySeconds, doctest::Approx(2.0f));

        // It lasts ten seconds; a kill on the way renews it.
        RunTalents::update(party, 9.0f);
        CHECK(RunTalents::attackRecoveryScale(rules, killer) < 1.0f);
        run.gameRules().combat.applyPartyHitEffects(party, hit);
        RunTalents::update(party, 9.0f);
        CHECK(RunTalents::attackRecoveryScale(rules, killer) < 1.0f);
        RunTalents::update(party, 1.5f);
        CHECK_EQ(RunTalents::attackRecoveryScale(rules, killer), doctest::Approx(1.0));
    }

    // A hero without the talent gains nothing from a kill.
    Character plain = hero();
    RunTalents::noteKill(rules, plain);
    CHECK_EQ(plain.talentState.onslaughtSeconds, doctest::Approx(0.0));
}

TEST_CASE("Bloodthirst: the hero heals for a share of the damage of their blows, arrows and spells")
{
    const RoguelikeRules run(installedRun());
    const DungeonRunRules &rules = run.rules();
    CHECK_EQ(RunTalents::leech(rules, hero(), 100), 0);
    CHECK_EQ(RunTalents::leech(rules, heroWith("bloodthirst", 1), 100), 5);
    CHECK_EQ(RunTalents::leech(rules, heroWith("bloodthirst", 2), 100), 10);
    CHECK_EQ(RunTalents::leech(rules, heroWith("bloodthirst", 3), 100), 15);
    // A small hit still gives a point; no damage gives none.
    CHECK_EQ(RunTalents::leech(rules, heroWith("bloodthirst", 1), 3), 1);
    CHECK_EQ(RunTalents::leech(rules, heroWith("bloodthirst", 3), 0), 0);

    Party party = partyOf(run, {heroWith("bloodthirst", 2), hero("Other")});
    party.member(0)->health = 50;
    party.member(1)->health = 50;
    CHECK(run.gameRules().combat.usesPartyHitEffects());

    for (const bool melee : {true, false})
    {
        for (const uint32_t spellId : {0u, 6u})
        {
            const int before = party.member(0)->health;
            PartyHitOnMonster hit = {};
            hit.memberIndex = 0;
            hit.damage = 80;
            hit.melee = melee;
            hit.spellId = melee ? 0 : spellId;
            run.gameRules().combat.applyPartyHitEffects(party, hit);
            CHECK_EQ(party.member(0)->health, before + 8);
        }
    }

    // Only the hero who struck, and never above the maximum.
    CHECK_EQ(party.member(1)->health, 50);
    party.member(0)->health = Party::effectiveMaximumHealth(*party.member(0)) - 1;
    PartyHitOnMonster hit = {};
    hit.memberIndex = 0;
    hit.damage = 500;
    hit.melee = true;
    run.gameRules().combat.applyPartyHitEffects(party, hit);
    CHECK_EQ(party.member(0)->health, Party::effectiveMaximumHealth(*party.member(0)));
}

TEST_CASE("Pinning Shot: an arrow's hit slows for the rank's seconds")
{
    const DungeonRunRules rules = *installedRun().rules;
    CHECK_EQ(RunTalents::pinSeconds(rules, hero()), doctest::Approx(0));
    CHECK_EQ(RunTalents::pinSeconds(rules, heroWith("pinning_shot", 1)), doctest::Approx(2));
    CHECK_EQ(RunTalents::pinSeconds(rules, heroWith("pinning_shot", 2)), doctest::Approx(3));
    CHECK_EQ(RunTalents::pinSeconds(rules, heroWith("pinning_shot", 3)), doctest::Approx(4));
}

TEST_CASE("Shield Wall: less damage with a shield in hand, and blows meant for allies are taken")
{
    const RoguelikeRules run(installedRun());
    const DungeonRunRules &rules = run.rules();
    const std::vector<int> left = {85, 75, 65};
    const std::vector<float> chance = {15.0f, 25.0f, 35.0f};

    for (int rank = 1; rank <= 3; ++rank)
    {
        const Character bearer = heroWith("shield_wall", rank);
        CHECK_EQ(RunTalents::absorbDamage(rules, bearer, true, 100, 1000), left[static_cast<size_t>(rank - 1)]);
        // Without the shield the talent does nothing.
        CHECK_EQ(RunTalents::absorbDamage(rules, bearer, false, 100, 1000), 100);
        CHECK_EQ(RunTalents::coverChance(rules, bearer, true), doctest::Approx(chance[static_cast<size_t>(rank - 1)]));
        CHECK_EQ(RunTalents::coverChance(rules, bearer, false), doctest::Approx(0));
    }

    CHECK_EQ(RunTalents::coverChance(rules, hero(), true), doctest::Approx(0));

    // Through the rules: the bearer's own hits are cut, and a third of the blows at the other hero are covered.
    REQUIRE(data().itemTable.get(ShieldItemId) != nullptr);
    REQUIRE_EQ(data().itemTable.get(ShieldItemId)->equipStat, "Shield");
    Character bearer = heroWith("shield_wall", 3);
    bearer.equipment.offHand = ShieldItemId;
    Party party = partyOf(run, {bearer, hero("Ally")});
    const CombatRules &combat = run.gameRules().combat;
    CHECK_EQ(combat.absorbCharacterDamage(party, 0, 100), 65);
    CHECK_EQ(combat.absorbCharacterDamage(party, 1, 100), 100);
    std::mt19937 rng(7);
    int covered = 0;
    constexpr int Blows = 4000;

    for (int blow = 0; blow < Blows; ++blow)
    {
        const std::optional<CoverOutcome> cover =
            combat.coverMonsterAttack(party, MonsterAttackCover{1, false, CombatDamageType::Physical, &rng});

        if (cover)
        {
            CHECK_EQ(cover->memberIndex, 0u);
            CHECK_FALSE(cover->halved);
            ++covered;
        }
    }

    CHECK(covered > Blows * 30 / 100);
    CHECK(covered < Blows * 40 / 100);
    // A blow at the bearer is nobody's to cover; a spell is not caught; a bearer who is out covers nobody.
    CHECK_FALSE(combat.coverMonsterAttack(party, MonsterAttackCover{0, false, CombatDamageType::Physical, &rng}));

    for (int blow = 0; blow < 200; ++blow)
    {
        CHECK_FALSE(combat.coverMonsterAttack(party, MonsterAttackCover{1, true, CombatDamageType::Fire, &rng}));
    }

    party.member(0)->conditions.set(static_cast<size_t>(CharacterCondition::Unconscious));

    for (int blow = 0; blow < 200; ++blow)
    {
        CHECK_FALSE(combat.coverMonsterAttack(party, MonsterAttackCover{1, false, CombatDamageType::Physical, &rng}));
    }

    // The shield put away: no cut and no cover.
    party.member(0)->conditions.reset(static_cast<size_t>(CharacterCondition::Unconscious));
    party.member(0)->equipment.offHand = 0;
    CHECK_EQ(combat.absorbCharacterDamage(party, 0, 100), 100);

    for (int blow = 0; blow < 200; ++blow)
    {
        CHECK_FALSE(combat.coverMonsterAttack(party, MonsterAttackCover{1, false, CombatDamageType::Physical, &rng}));
    }

    // Standard's rules cover nobody, whatever the hero carries.
    CHECK_FALSE(standardGameRules().combat.coverMonsterAttack(
        party, MonsterAttackCover{1, false, CombatDamageType::Physical, &rng}));
}

TEST_CASE("Unbroken: the part of one hit above 30 % of the maximum hit points is halved")
{
    const DungeonRunRules rules = *installedRun().rules;
    const Character unbroken = heroWith("unbroken", 1);
    // Maximum 200: the threshold is 60.
    CHECK_EQ(RunTalents::absorbDamage(rules, unbroken, false, 40, 200), 40);
    CHECK_EQ(RunTalents::absorbDamage(rules, unbroken, false, 60, 200), 60);
    CHECK_EQ(RunTalents::absorbDamage(rules, unbroken, false, 100, 200), 80);
    CHECK_EQ(RunTalents::absorbDamage(rules, unbroken, false, 260, 200), 160);
    CHECK_EQ(RunTalents::absorbDamage(rules, hero(), false, 260, 200), 260);
    // With Shield Wall the shield's cut comes first.
    Character both = heroWith("unbroken", 1);
    both.talentRanks["shield_wall"] = 3;
    CHECK_EQ(RunTalents::absorbDamage(rules, both, true, 200, 200), 95);
}

TEST_CASE("Cheat Death: once in a dungeon a fatal hit leaves a tenth of the hit points, then three seconds of cut")
{
    RoguelikeRules run(installedRun());
    run.state().entered = 1;
    Party party = partyOf(run, {heroWith("cheat_death", 1), hero("Other")});
    Character &cheater = *party.member(0);
    const int maximum = Party::effectiveMaximumHealth(cheater);
    REQUIRE(maximum >= 100);
    const int tenth = (maximum + 9) / 10;
    cheater.health = 50;
    // A hit that does not fell the hero is taken as it is.
    REQUIRE(party.applyDamageToMember(0, 20, "test"));
    CHECK_EQ(cheater.health, 30);
    CHECK_FALSE(cheater.eventVariables.contains(RunCheatDeathStopVariable));
    // The fatal one: a tenth of the maximum is left, and the hero stands.
    cheater.health = maximum;
    REQUIRE(party.applyDamageToMember(0, maximum * 5, "test"));
    CHECK_EQ(cheater.health, tenth);
    CHECK(GameMechanics::canAct(cheater));
    CHECK_EQ(cheater.eventVariables[RunCheatDeathStopVariable], 1);
    CHECK(cheater.talentState.cheatDeathSeconds > 2.9f);
    // For three seconds the hero takes a twentieth of every hit.
    REQUIRE(party.applyDamageToMember(0, 100, "test"));
    CHECK_EQ(cheater.health, tenth - 5);
    RunTalents::update(party, 3.5f);
    CHECK_EQ(cheater.talentState.cheatDeathSeconds, doctest::Approx(0));
    // Once in a dungeon: the next fatal hit fells the hero, also after a save and a load (the use is the hero's
    // saved variable, the timers are not saved).
    cheater.talentState = {};
    cheater.health = 40;
    party.applyDamageToMember(0, 500, "test");
    CHECK(cheater.health <= 0);
    CHECK_FALSE(GameMechanics::canAct(cheater));

    // The next dungeon: it saves again. The other hero has no such talent.
    run.state().entered = 2;
    cheater.conditions.reset();
    cheater.health = maximum;
    REQUIRE(party.applyDamageToMember(0, maximum * 5, "test"));
    CHECK_EQ(cheater.health, tenth);
    CHECK_EQ(cheater.eventVariables[RunCheatDeathStopVariable], 2);
    party.applyDamageToMember(1, 100000, "test");
    CHECK(party.member(1)->health <= 0);

    // A hero already below the share is not healed up to it: the hit simply does nothing.
    const DungeonRunRules &rules = run.rules();
    Character weak = heroWith("cheat_death", 1);
    CHECK_EQ(RunTalents::cheatDeath(rules, weak, 500, 5, 200, 3), 0);
    CHECK_EQ(weak.eventVariables[RunCheatDeathStopVariable], 3);
    // Outside a dungeon of a run (no stop) nothing is cheated.
    Character lost = heroWith("cheat_death", 1);
    CHECK_EQ(RunTalents::cheatDeath(rules, lost, 500, 50, 200, 0), 500);
}

TEST_CASE("Attunement: the hero's spells hit harder and cost less")
{
    const RoguelikeRules run(installedRun());
    const DungeonRunRules &rules = run.rules();
    const std::vector<double> scale = {1.1, 1.2, 1.3};
    const std::vector<int> cost = {18, 16, 14};

    for (int rank = 1; rank <= 3; ++rank)
    {
        const Character caster = heroWith("attunement", rank);
        CHECK_EQ(RunTalents::spellDamageScale(rules, caster, 2, 1), doctest::Approx(scale[size_t(rank - 1)]));
        CHECK_EQ(RunTalents::manaCost(rules, caster, 20), cost[size_t(rank - 1)]);
        // Never free, and nothing stays nothing.
        CHECK_EQ(RunTalents::manaCost(rules, caster, 1), 1);
        CHECK_EQ(RunTalents::manaCost(rules, caster, 0), 0);
    }

    CHECK_EQ(RunTalents::spellDamageScale(rules, hero(), 2, 1), doctest::Approx(1.0));
    CHECK_EQ(RunTalents::manaCost(rules, hero(), 20), 20);

    // Through the rules: the same roll, a fifth stronger, and the cheaper cost; a scroll is the scroll's own.
    const SpellEntry *pFireBolt = data().spellTable.findById(2);
    REQUIRE(pFireBolt != nullptr);
    const Character caster = heroWith("attunement", 2);
    const Character plain = hero();
    CharacterSpellCast cast = {};
    cast.pSpell = pFireBolt;
    cast.mastery = SkillMastery::Master;
    cast.skillLevel = 7;
    long long attuned = 0;
    long long ordinary = 0;

    for (int roll = 0; roll < 400; ++roll)
    {
        cast.pCaster = &caster;
        attuned += run.gameRules().combat.rollSpellDamage(cast, nullptr);
        cast.pCaster = &plain;
        ordinary += run.gameRules().combat.rollSpellDamage(cast, nullptr);
    }

    REQUIRE(ordinary > 0);
    CHECK(double(attuned) / double(ordinary) > 1.12);
    CHECK(double(attuned) / double(ordinary) < 1.28);
    cast.pCaster = &caster;
    CHECK_EQ(run.gameRules().spells.manaCost(cast, 20), 16);
    cast.fromScroll = true;
    CHECK_EQ(run.gameRules().spells.manaCost(cast, 20), 20);
    CHECK_EQ(standardGameRules().spells.manaCost(cast, 20), 20);
}

TEST_CASE("Overload: an area spell into a crowd hits harder for each monster beyond the first, at most three")
{
    const DungeonRunRules rules = *installedRun().rules;
    constexpr uint32_t Fireball = 6;
    constexpr uint32_t FireBolt = 2;

    for (int rank = 1; rank <= 3; ++rank)
    {
        const Character caster = heroWith("overload", rank);
        const double step = 0.1 * rank;
        CHECK_EQ(RunTalents::spellDamageScale(rules, caster, Fireball, 0), doctest::Approx(1.0));
        CHECK_EQ(RunTalents::spellDamageScale(rules, caster, Fireball, 1), doctest::Approx(1.0));
        CHECK_EQ(RunTalents::spellDamageScale(rules, caster, Fireball, 2), doctest::Approx(1.0 + step));
        CHECK_EQ(RunTalents::spellDamageScale(rules, caster, Fireball, 4), doctest::Approx(1.0 + 3 * step));
        CHECK_EQ(RunTalents::spellDamageScale(rules, caster, Fireball, 12), doctest::Approx(1.0 + 3 * step));
        // A spell of one target gains nothing from the crowd.
        CHECK_EQ(RunTalents::spellDamageScale(rules, caster, FireBolt, 12), doctest::Approx(1.0));
    }

    // With Attunement the two multiply.
    Character both = heroWith("overload", 3);
    both.talentRanks["attunement"] = 3;
    CHECK_EQ(RunTalents::spellDamageScale(rules, both, Fireball, 4), doctest::Approx(1.3 * 1.9));
}

TEST_CASE("Spell Surge: every fifth, fourth or third spell is free and half again as strong")
{
    const RoguelikeRules run(installedRun());
    const DungeonRunRules &rules = run.rules();
    const std::vector<int> every = {5, 4, 3};

    for (int rank = 1; rank <= 3; ++rank)
    {
        Party party = partyOf(run, {heroWith("spell_surge", rank), hero("Other")});
        Character &caster = *party.member(0);
        const int number = every[static_cast<size_t>(rank - 1)];
        CharacterSpellCast cast = {};
        cast.pCaster = &caster;
        int freeCasts = 0;

        // Two rounds of the count, cast through the rules' hooks as the game casts.
        for (int spell = 1; spell <= number * 2; ++spell)
        {
            const bool surging = spell % number == 0;
            CHECK_EQ(RunTalents::surgeReady(rules, caster), surging);
            CHECK_EQ(run.gameRules().spells.manaCost(cast, 12), surging ? 0 : 12);
            // The damage of the free spell, rolled as it is cast or as its missile lands a moment after.
            CHECK_EQ(RunTalents::spellDamageScale(rules, caster, 2, 1), doctest::Approx(surging ? 1.5 : 1.0));
            run.gameRules().spells.afterCast(party, 0, 2, data().spellTable, true, false);
            freeCasts += surging ? 1 : 0;

            if (surging)
            {
                CHECK_FALSE(RunTalents::surgeReady(rules, caster));
                CHECK_EQ(RunTalents::spellDamageScale(rules, caster, 2, 1), doctest::Approx(1.5));
                RunTalents::update(party, 3.5f);
                CHECK_EQ(RunTalents::spellDamageScale(rules, caster, 2, 1), doctest::Approx(1.0));
            }
        }

        CHECK_EQ(freeCasts, 2);
        // The other hero's spells are not counted and never free.
        cast.pCaster = party.member(1);
        CHECK_EQ(run.gameRules().spells.manaCost(cast, 12), 12);
        CHECK_EQ(party.member(1)->talentState.spellCasts, 0);
    }
}

TEST_CASE("Resolve: resistances by rank, and no special attack takes hold at the last")
{
    const RoguelikeRules run(installedRun());
    const DungeonRunRules &rules = run.rules();

    for (int rank = 0; rank <= 3; ++rank)
    {
        Party party = partyOf(run, {rank > 0 ? heroWith("resolve", rank) : hero(), hero("Other")});
        const CharacterResistanceSet &with = party.member(0)->magicalBonuses.resistances;
        const CharacterResistanceSet &without = party.member(1)->magicalBonuses.resistances;
        const int amount = 10 * rank;
        CHECK_EQ(with.fire - without.fire, amount);
        CHECK_EQ(with.air - without.air, amount);
        CHECK_EQ(with.water - without.water, amount);
        CHECK_EQ(with.earth - without.earth, amount);
        CHECK_EQ(with.mind - without.mind, amount);
        CHECK_EQ(with.body - without.body, amount);
        CHECK_EQ(with.spirit - without.spirit, amount);
        CHECK_EQ(with.light - without.light, amount);
        CHECK_EQ(with.dark - without.dark, amount);
        // Worked out anew each time, never piling up.
        party.refreshDerivedState();
        party.refreshDerivedState();
        CHECK_EQ(party.member(0)->magicalBonuses.resistances.fire - party.member(1)->magicalBonuses.resistances.fire,
            amount);
        CHECK_EQ(run.gameRules().combat.immuneToMonsterSpecialAttacks(*party.member(0)), rank == 3);
        CHECK_EQ(RunTalents::immuneToSpecialAttacks(rules, *party.member(0)), rank == 3);
        CHECK_FALSE(run.gameRules().combat.immuneToMonsterSpecialAttacks(*party.member(1)));
    }
}

TEST_CASE("Binding Circle: the hold's seconds by rank, then twenty seconds of rest")
{
    const DungeonRunRules rules = *installedRun().rules;
    const std::vector<float> seconds = {2.0f, 2.5f, 3.0f};
    REQUIRE(rules.talent("binding_circle") != nullptr);
    CHECK_EQ(rules.talent("binding_circle")->radius, doctest::Approx(500));

    for (int rank = 1; rank <= 3; ++rank)
    {
        Party party;
        PartySeed seed;
        seed.members.push_back(heroWith("binding_circle", rank));
        party.seed(seed);
        Character &caster = *party.member(0);
        caster.talentRanks["binding_circle"] = static_cast<uint8_t>(rank);
        CHECK_EQ(RunTalents::springBindingCircle(rules, caster), doctest::Approx(seconds[size_t(rank - 1)]));
        // Resting: nothing for twenty seconds.
        CHECK_EQ(RunTalents::springBindingCircle(rules, caster), doctest::Approx(0));
        RunTalents::update(party, 19.0f);
        CHECK_EQ(RunTalents::springBindingCircle(rules, caster), doctest::Approx(0));
        RunTalents::update(party, 1.5f);
        CHECK_EQ(RunTalents::springBindingCircle(rules, caster), doctest::Approx(seconds[size_t(rank - 1)]));
    }

    Character plain = hero();
    CHECK_EQ(RunTalents::springBindingCircle(rules, plain), doctest::Approx(0));
    CHECK(plain.talentState.cooldownSeconds.empty());
}

TEST_CASE("a run's talent card of the installed table: taken by rank, with the arrows' bit")
{
    const DungeonRunRules rules = *installedRun().rules;
    Character archer = hero();
    archer.skills["Bow"] = {"Bow", 4, SkillMastery::Expert};
    RunCard card = {};
    card.kind = RunCard::Kind::Talent;
    card.talent = "multishot";

    for (int rank = 1; rank <= 3; ++rank)
    {
        card.rank = rank;
        applyRunCard(rules, archer, card, nullptr, 1, 0, {});
        CHECK_EQ(runTalentRank(archer, "multishot"), rank);
        CHECK_EQ(RunTalents::attackExtras(rules, archer).fanArrows, 2 * rank);
    }

    // The last rank is the last: another card changes nothing.
    applyRunCard(rules, archer, card, nullptr, 1, 0, {});
    CHECK_EQ(runTalentRank(archer, "multishot"), 3);
    CHECK_EQ(archer.eventVariables[RunCardsTakenVariable], 4);
    card.talent = "explosive_arrows";
    card.rank = 1;
    applyRunCard(rules, archer, card, nullptr, 1, 0, {});
    CHECK(archer.playerBits.contains(RunExplosiveArrowsPlayerBit));
    CHECK_EQ(runTalentRank(archer, "explosive_arrows"), 1);
    // A talent of an effect leaves the hero's permanent numbers alone.
    CHECK_EQ(archer.permanentBonuses.might, 0);
    CHECK_EQ(archer.permanentBonuses.maxHealth, 0);
    CHECK_EQ(archer.permanentBonuses.armorClass, 0);
}

TEST_CASE("the card screen's texts: four cards with kind, step, pips, effect and picture")
{
    const DungeonRunRules rules = *installedRun().rules;
    Character archer = hero("Alexis");
    archer.className = "Archer";
    archer.skills["Bow"] = {"Bow", 4, SkillMastery::Expert};
    archer.talentRanks["multishot"] = 1;
    archer.eventVariables[RunCardsTakenVariable] = 3;
    archer.eventVariables[RunPendingCardsVariable] = 1;
    Party party;
    PartySeed seed;
    seed.members = {hero("Roderick"), archer};
    party.seed(seed);
    *party.member(1) = archer;

    RunCard bow = {};
    bow.kind = RunCard::Kind::Skill;
    bow.id = "skill:Bow";
    bow.skill = "Bow";
    bow.mastery = SkillMastery::Master;
    RunCard fire = {};
    fire.kind = RunCard::Kind::Skill;
    fire.id = "skill:FireMagic";
    fire.skill = "FireMagic";
    fire.mastery = SkillMastery::Normal;
    fire.books = 3;
    RunCard fan = {};
    fan.kind = RunCard::Kind::Talent;
    fan.id = "talent:multishot";
    fan.talent = "multishot";
    fan.rank = 2;
    fan.maxRank = 3;
    RunCard resolve = {};
    resolve.kind = RunCard::Kind::Talent;
    resolve.id = "talent:resolve";
    resolve.talent = "resolve";
    resolve.rank = 1;
    resolve.maxRank = 3;

    const RunCardDialog dialog = buildRunCardDialog(rules, party, 1, {bow, fire, fan, resolve}, 2);
    CHECK_EQ(dialog.memberIndex, 1u);
    CHECK_EQ(dialog.cardNumber, 4);
    CHECK_EQ(dialog.totalCards, 10);
    CHECK_EQ(dialog.stop, 2);
    CHECK_EQ(dialog.stops, 4);
    CHECK(dialog.membersOwed == std::vector<bool>{false, true});
    CHECK_FALSE(dialog.selected.has_value());
    REQUIRE_EQ(dialog.cards.size(), 4u);

    // The skill: Expert to Master, four pips (Normal to Grandmaster), its picture from run.yml.
    CHECK_EQ(dialog.cards[0].kind, "SKILL");
    CHECK_EQ(dialog.cards[0].stepFrom, "Expert");
    CHECK_EQ(dialog.cards[0].stepTo, "Master");
    CHECK_EQ(dialog.cards[0].pips, 4);
    CHECK_EQ(dialog.cards[0].pipsBefore, 2);
    CHECK_EQ(dialog.cards[0].pipsAfter, 3);
    CHECK_EQ(dialog.cards[0].icon, "talent_marksman");
    REQUIRE_FALSE(dialog.cards[0].effect.empty());
    CHECK_EQ(dialog.cards[0].effect[0], "Skill level 7");
    // A school: books, at most three of them, the school's sign and its attack spell behind it.
    CHECK_EQ(dialog.cards[1].kind, "BOOKS");
    CHECK_EQ(dialog.cards[1].stepFrom, "Not learned");
    CHECK_EQ(dialog.cards[1].icon, "stfu");
    CHECK_EQ(dialog.cards[1].iconSpell, 2u);
    REQUIRE_EQ(dialog.cards[1].effect.size(), 2u);
    CHECK_EQ(dialog.cards[1].effect[1], "Up to 3 spell books");
    // A talent: its rank's numbers written into its line.
    CHECK_EQ(dialog.cards[2].kind, "TALENT");
    CHECK_EQ(dialog.cards[2].title, "Multishot");
    CHECK_EQ(dialog.cards[2].stepFrom, "Rank 1");
    CHECK_EQ(dialog.cards[2].stepTo, "2 / 3");
    CHECK_EQ(dialog.cards[2].pips, 3);
    CHECK_EQ(dialog.cards[2].pipsBefore, 1);
    CHECK_EQ(dialog.cards[2].pipsAfter, 2);
    CHECK_EQ(dialog.cards[2].icon, "talent_multishot");
    REQUIRE_FALSE(dialog.cards[2].effect.empty());
    CHECK_EQ(dialog.cards[2].effect[0], "Every bow shot also looses 4 side arrows in a fan, at 50 % damage");
    CHECK_EQ(dialog.cards[3].effect[0],
        "+10 to every resistance; at rank 3 monsters' special attacks do not take hold");
    CHECK_FALSE(dialog.cards[3].detail.empty());

    // Every talent's line has no number left unfilled at any rank.
    for (const DungeonRunTalent &talent : rules.talents)
    {
        for (int rank = 1; rank <= talent.maxRank; ++rank)
        {
            const std::string text = runTalentEffectText(talent, rank);
            CHECK_MESSAGE(text.find('{') == std::string::npos, talent.id);
            const bool halfNumber = text.find("2.5") != std::string::npos;
            CHECK_MESSAGE((!halfNumber || talent.id == "binding_circle"), talent.id);
        }
    }

    CHECK(runTalentEffectText(*rules.talent("binding_circle"), 2).find("2.5 s") != std::string::npos);
}

TEST_CASE("a guardian's item: an artifact or a relic exactly, never one the party has or was given, fixed by seed")
{
    const ItemTable &items = data().itemTable;
    std::set<uint32_t> artifacts;
    std::set<uint32_t> relics;

    for (uint64_t seed = 0; seed < 60; ++seed)
    {
        const std::optional<uint32_t> artifact = pickRunGuardianItem(items, ItemRarity::Artifact, "mm6", seed, 1, {});
        const std::optional<uint32_t> relic = pickRunGuardianItem(items, ItemRarity::Relic, "mm6", seed, 3, {});
        REQUIRE(artifact.has_value());
        REQUIRE(relic.has_value());
        CHECK(items.get(*artifact)->rarity == ItemRarity::Artifact);
        CHECK(items.get(*relic)->rarity == ItemRarity::Relic);
        CHECK(artifact == pickRunGuardianItem(items, ItemRarity::Artifact, "mm6", seed, 1, {}));
        artifacts.insert(*artifact);
        relics.insert(*relic);
    }

    // Random enough to vary between runs.
    CHECK(artifacts.size() >= 5);
    CHECK(relics.size() >= 5);

    // Nothing taken is given again; with every one taken there is none.
    std::set<uint32_t> taken;

    for (int stop = 1; stop <= 200; ++stop)
    {
        const std::optional<uint32_t> item = pickRunGuardianItem(items, ItemRarity::Relic, "mm6", 9, stop,
            [&taken](uint32_t candidate) { return taken.contains(candidate); });

        if (!item)
        {
            break;
        }

        CHECK_FALSE(taken.contains(*item));
        taken.insert(*item);
    }

    CHECK(taken.size() >= 10);
    CHECK_FALSE(pickRunGuardianItem(items, ItemRarity::Relic, "mm6", 9, 1,
        [&taken](uint32_t candidate) { return taken.contains(candidate); }).has_value());
}

TEST_CASE("a guardian's item waits for room when every pack is full, and comes once there is room")
{
    const ItemTable &items = data().itemTable;
    const std::optional<uint32_t> artifact = pickRunGuardianItem(items, ItemRarity::Artifact, "mm6", 4, 1, {});
    REQUIRE(artifact.has_value());
    Party party;
    party.setItemTable(&items);
    PartySeed seed;
    seed.members = {hero("Roderick"), hero("Alexis")};
    party.seed(seed);

    // Fill every pack with small junk until nothing more fits.
    size_t added = 0;

    for (uint32_t id = 1; id < 600 && added < 2000; ++id)
    {
        const ItemDefinition *pCandidate = items.get(id);

        if (pCandidate == nullptr || pCandidate->inventoryWidth != 1 || pCandidate->inventoryHeight != 1
            || ItemRuntime::isRareItem(*pCandidate))
        {
            continue;
        }

        InventoryItem junk = ItemGenerator::makeInventoryItem(id, items, ItemGenerationMode::Generic);

        while (party.tryGrantInventoryItem(junk))
        {
            ++added;
        }

        break;
    }

    REQUIRE(added > 0);
    CHECK_FALSE(party.hasItemAnywhere(*artifact));
    const std::vector<uint32_t> waiting = grantRunItems(party, items, {*artifact});
    REQUIRE_EQ(waiting.size(), 1u);
    CHECK_FALSE(party.hasItemAnywhere(*artifact));
    // A second try with the packs still full keeps it waiting; nothing is lost or doubled.
    CHECK_EQ(grantRunItems(party, items, waiting), waiting);

    // Room made: it comes, identified, once.
    for (size_t member = 0; member < party.members().size(); ++member)
    {
        party.member(member)->inventory.clear();
    }

    CHECK(grantRunItems(party, items, waiting).empty());
    CHECK(party.hasItemAnywhere(*artifact));
    size_t count = 0;

    for (const Character &member : party.members())
    {
        for (const InventoryItem &item : member.inventory)
        {
            if (item.objectDescriptionId == *artifact)
            {
                ++count;
                CHECK(item.identified);
            }
        }
    }

    CHECK_EQ(count, 1u);
}

TEST_CASE("the camp's alchemy shop sells the run's stacks of herbs and bottles, only in a run")
{
    const RoguelikeRules run(installedRun());
    const HouseEntry *pShop = data().houseTable.get(97);
    REQUIRE(pShop != nullptr);
    const auto stockOf = [&](const GameRules *pRules)
    {
        Party party;
        party.setItemTable(&data().itemTable);
        party.setItemEnchantTables(&data().standardItemEnchantTable, &data().specialItemEnchantTable);

        PartySeed seed;
        seed.members = {hero()};
        party.seed(seed);

        if (pRules != nullptr)
        {
            party.setRules(pRules);
        }
        return HouseServiceRuntime::ensureStock(party, data().itemTable, data().standardItemEnchantTable,
            data().specialItemEnchantTable, *pShop, 9.0f * 60.0f, HouseStockMode::ShopStandard);
    };
    // The shop's own goods are random (each party has its own stock seed) and may hold a single herb too; the run's
    // stacks take the last places of the shelf, in run.yml's order: three of each herb and three bottles.
    for (int attempt = 0; attempt < 20; ++attempt)
    {
        const std::vector<InventoryItem> stock = stockOf(&run.gameRules());
        REQUIRE(stock.size() >= 4);
        std::vector<std::pair<uint32_t, uint32_t>> last;

        for (size_t slot = stock.size() - 4; slot < stock.size(); ++slot)
        {
            last.emplace_back(stock[slot].objectDescriptionId, stock[slot].quantity);
        }

        CHECK(last == std::vector<std::pair<uint32_t, uint32_t>>{{1764, 3}, {1763, 3}, {1762, 3}, {1765, 3}});
    }

    const std::vector<InventoryItem> stock = stockOf(&run.gameRules());
    // Bought one at a time at the herb's own price, as the engine's stacks are.
    Party buyer;
    buyer.setItemTable(&data().itemTable);
    buyer.setRules(&run.gameRules());
    const auto herb = std::find_if(stock.rbegin(), stock.rend(),
        [](const InventoryItem &item) { return item.objectDescriptionId == 1764; });
    REQUIRE(herb != stock.rend());
    CHECK_EQ(herb->quantity, 3u);
    const int price = HouseServiceRuntime::buyPrice(buyer, data().itemTable, data().standardItemEnchantTable,
        data().specialItemEnchantTable, *pShop, *herb);
    CHECK(price > 0);
    CHECK(price < 100);

    // The same shop in an ordinary game: none of the run's stacks.
    size_t ordinary = 0;

    for (const InventoryItem &item : stockOf(nullptr))
    {
        ordinary += item.objectDescriptionId >= 1762 && item.objectDescriptionId <= 1765 && item.quantity > 1;
    }

    CHECK_EQ(ordinary, 0u);
}
