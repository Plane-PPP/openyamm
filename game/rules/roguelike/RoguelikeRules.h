#pragma once

#include "game/content/DungeonRun.h"
#include "game/party/SkillData.h"
#include "game/rules/GameRules.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace OpenYAMM::Game
{
class ClassMultiplierTable;
class ClassSkillTable;
class Party;
struct Character;

// The rules of a dungeon run with a `rules` block in its run.yml (game/content/DungeonRun.h). A run is cleared map by
// map: the share of a map's monster hit points the party has slain fills the HUD's bar, gives the stop's level-ups on
// the way and calls the map's guardian when it passes the threshold; the way out opens when the guardian is slain.
// A level-up raises every hero by several levels at once and owes each of them a card: one of three skill steps and
// perks. GameApplicationDungeonRun.cpp does what needs the application (the guardian, the card dialogs, the exits).
//
// Nothing here is saved by itself: the run's numbers live in the save's named global variables (RoguelikeRunState)
// and each hero's in Character::eventVariables, so the save format does not change.

// Each hero's run numbers, kept in Character::eventVariables: the cards still owed, the cards taken and each perk's
// level (RunPerkLevelVariableBase + the perk's index in run.yml).
inline constexpr uint16_t RunPendingCardsVariable = 0xF0F0;
inline constexpr uint16_t RunCardsTakenVariable = 0xF0F1;
inline constexpr uint16_t RunPerkLevelVariableBase = 0xF100;

struct RoguelikeRunState
{
    enum Guardian : int
    {
        GuardianNone = 0,
        // The threshold is passed; the application places the guardian.
        GuardianDue = 1,
        GuardianAlive = 2,
        GuardianSlain = 3,
    };

    // Dungeons the run entered, the current one included (the skeleton's DungeonRunEnteredVariable).
    int entered = 0;
    // Level-ups the run has given.
    int levelUps = 0;
    // The current map's guardian and its actor index (-1: none placed).
    int guardian = GuardianNone;
    int guardianActor = -1;
    // The party was offered the way to the door on this map.
    int doorOffered = 0;
    // The promotion was given.
    int promoted = 0;

    bool operator==(const RoguelikeRunState &) const = default;
};

void readRunState(const std::unordered_map<std::string, int32_t> &variables, RoguelikeRunState &state);
void writeRunState(const RoguelikeRunState &state, std::unordered_map<std::string, int32_t> &variables);

// A level-up card: one step of a skill (none, Normal 1, Expert 4, Master 7, Grandmaster 10) or one level of a perk.
struct RunCard
{
    enum class Kind
    {
        Skill,
        Perk,
    };

    Kind kind = Kind::Skill;
    // The skill and the mastery the card raises it to.
    std::string skill;
    SkillMastery mastery = SkillMastery::None;
    // The perk (an index into DungeonRunRules::perks) and the level the card raises it to.
    size_t perkIndex = 0;
    int perkLevel = 0;
    // The button's text.
    std::string label;

    bool operator==(const RunCard &) const = default;
};

// The skill level a mastery step sets.
uint32_t runSkillLevelForMastery(SkillMastery mastery);
// The hero's next cards: up to three different ones, a step of a skill the hero knows, a perk and anything else where
// the pool allows. The draw is keyed by the run's seed, the hero's place and the cards the hero has taken, so a
// loaded save offers the same. Skills stop at the class's cap; a perk at its top level.
std::vector<RunCard> dealRunCards(
    const DungeonRunRules &rules,
    const Character &character,
    const ClassSkillTable *pClassSkillTable,
    uint64_t rootSeed,
    size_t memberIndex);
// Gives the card to the hero and counts it. Returns the spells whose books a school card owes the hero: its weakest
// attack spell first, then others the new mastery can learn; none the hero knows or `spellTaken` reports (a book the
// party already carries or is owed).
std::vector<uint32_t> applyRunCard(
    const DungeonRunRules &rules,
    Character &character,
    const RunCard &card,
    const ClassMultiplierTable *pClassMultiplierTable,
    uint64_t rootSeed,
    size_t memberIndex,
    const std::function<bool(uint32_t)> &spellTaken);
// One level-up: every hero gains the rules' levels and is owed a card.
void grantRunLevelUp(const DungeonRunRules &rules, Party &party);
// How far the map is cleared toward its guardian, 0-1, and whether the threshold is passed (strictly more than the
// share of the counted hit points).
double runClearFraction(const DungeonRunRules &rules, const MapKillProgress &progress);
bool runClearThresholdPassed(const DungeonRunRules &rules, const MapKillProgress &progress);
// Level-ups the run owes in all at this progress of the stop (0-based): the stops before in full, this one's spread
// evenly up to the guardian, the last with it.
int runLevelUpsDue(const DungeonRunRules &rules, size_t stop, const MapKillProgress &progress);

class RoguelikeRules
{
public:
    explicit RoguelikeRules(DungeonRunDefinition run);
    ~RoguelikeRules();

    const GameRules &gameRules() const;
    const DungeonRunDefinition &run() const;
    const DungeonRunRules &rules() const;
    // The run's numbers; the application mirrors them into the save's named global variables.
    RoguelikeRunState &state() const;

private:
    class Experience;
    class Monsters;

    DungeonRunDefinition m_run;
    mutable RoguelikeRunState m_state;
    std::unique_ptr<Experience> m_pExperience;
    std::unique_ptr<Monsters> m_pMonsters;
    std::unique_ptr<GameRules> m_pGameRules;
};
}
