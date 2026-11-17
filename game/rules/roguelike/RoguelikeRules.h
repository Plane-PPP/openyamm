#pragma once

#include "game/content/DungeonRun.h"
#include "game/party/SkillData.h"
#include "game/rules/GameRules.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace OpenYAMM::Game
{
class ClassMultiplierTable;
class ItemTable;
enum class ItemRarity : uint8_t;
class ClassSkillTable;
class Party;
struct Character;

// The rules of a dungeon run with a `rules` block in its run.yml (game/content/DungeonRun.h). A run is cleared map by
// map: the share of a map's monster hit points the party has slain fills the HUD's bar, gives the stop's level-ups on
// the way and calls the map's guardian when it passes the threshold; the way out opens when the guardian is slain.
// A level-up raises every hero by several levels at once and owes each of them a card: one of a few skill steps and
// talents. GameApplicationDungeonRun.cpp does what needs the application (the guardian, the card screen, the exits).
//
// Nothing here is saved by itself: the run's numbers live in the save's named global variables (RoguelikeRunState),
// each hero's in Character::eventVariables and the hero's talent ranks in Character::talentRanks under the talents'
// ids (run.yml), so the save format does not change.

// Each hero's run numbers, kept in Character::eventVariables: the cards still owed, the cards taken, and the dungeon
// (its number in the run) in which the hero's Cheat Death last saved them.
inline constexpr uint16_t RunPendingCardsVariable = 0xF0F0;
inline constexpr uint16_t RunCardsTakenVariable = 0xF0F1;
inline constexpr uint16_t RunCheatDeathStopVariable = 0xF0F2;
// Until stable ids (OpenYAMM 2.4's first run) a perk's level was kept by the perk's index in that version's run.yml,
// at this variable + the index; migrateLegacyRunPerks moves such levels to Character::talentRanks.
inline constexpr uint16_t RunLegacyPerkLevelVariableBase = 0xF100;

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
    // The last stop (1-based) whose guardian's reward the party was given; each stop's comes once.
    int rewarded = 0;

    bool operator==(const RoguelikeRunState &) const = default;
};

void readRunState(const std::unordered_map<std::string, int32_t> &variables, RoguelikeRunState &state);
void writeRunState(const RoguelikeRunState &state, std::unordered_map<std::string, int32_t> &variables);

// A level-up card: one step of a skill (none, Normal 1, Expert 4, Master 7, Grandmaster 10) or one rank of a talent.
struct RunCard
{
    enum class Kind
    {
        Skill,
        Talent,
    };

    Kind kind = Kind::Skill;
    // The card's stable id: "skill:<Skill>" or "talent:<id>".
    std::string id;
    // The skill and the mastery the card raises it to; the spell books a school's card gives at most.
    std::string skill;
    SkillMastery mastery = SkillMastery::None;
    int books = 0;
    // The talent (DungeonRunTalent::id), the rank the card raises it to and the talent's last rank.
    std::string talent;
    int rank = 0;
    int maxRank = 0;
    // The line of the text choice (the debug console's `run pick`, a headless scenario).
    std::string label;

    bool operator==(const RunCard &) const = default;
};

// The skill level a mastery step sets.
uint32_t runSkillLevelForMastery(SkillMastery mastery);
// The hero's rank of a talent (0: not learned), by the talent's id.
int runTalentRank(const Character &character, std::string_view id);
// The hero's best mastery among the skills a talent may need: a canonical skill, "melee" (a melee weapon skill or
// Armsmaster) or "magic" (a school of magic).
SkillMastery runSkillGroupMastery(const Character &character, const std::string &skillOrGroup);
// Whether the hero has what the talent needs before its card is offered.
bool runTalentAvailable(const DungeonRunTalent &talent, const Character &character);
// The hero's next cards: up to rules.cardOffers different ones, a step of a skill the hero knows, a talent and
// anything else where the pool allows. The draw is keyed by the run's seed, the hero's place and the cards the hero
// has taken, so a loaded save offers the same. Skills stop at the class's cap; a talent at its last rank.
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
// A hero of a run saved before stable ids: every perk level kept by run.yml index becomes the rank of that perk's id
// in Character::talentRanks and the old variable goes, so it happens once. The amounts those perks added are already
// in the hero's permanent bonuses and are not added again. Returns whether anything was moved.
bool migrateLegacyRunPerks(Character &character);
// Gives the owed items (item ids) to the party, each to the first hero with room, identified; returns those that
// fit no pack and must wait.
std::vector<uint32_t> grantRunItems(Party &party, const ItemTable &itemTable, const std::vector<uint32_t> &owed);
// The rare item a guardian leaves: one of the item table's artifacts or relics (by `rarity`, an exact rarity, not
// ItemRuntime::isArtifact's both) of the map's world, none `taken` reports (carried, given before); another world's
// when the world has none left. Keyed by the run's seed and the stop.
std::optional<uint32_t> pickRunGuardianItem(
    const ItemTable &itemTable,
    ItemRarity rarity,
    const std::string &worldId,
    uint64_t rootSeed,
    int stop,
    const std::function<bool(uint32_t)> &taken);
// One level-up: every hero gains the rules' levels and is owed a card; a hero who is out comes round and every hero
// but the dead, the petrified and the eradicated gets back rules.levelUpHealShare of the maximum hit points, at least
// up to that share.
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
    class Combat;
    class Spells;
    class Classes;
    class Items;

    DungeonRunDefinition m_run;
    mutable RoguelikeRunState m_state;
    std::unique_ptr<Experience> m_pExperience;
    std::unique_ptr<Monsters> m_pMonsters;
    std::unique_ptr<Combat> m_pCombat;
    std::unique_ptr<Spells> m_pSpells;
    std::unique_ptr<Classes> m_pClasses;
    std::unique_ptr<Items> m_pItems;
    std::unique_ptr<GameRules> m_pGameRules;
};
}
