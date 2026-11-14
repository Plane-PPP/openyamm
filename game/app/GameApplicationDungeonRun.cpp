#include "game/app/GameApplication.h"

#include "engine/Localization.h"

#include "game/StringUtils.h"
#include "game/app/NewGameOptions.h"
#include "game/content/DungeonRun.h"
#include "game/content/RulesetDefinition.h"
#include "game/debug/GameplayDebugTrace.h"
#include "game/events/EventRuntime.h"
#include "game/gameplay/GameMechanics.h"
#include "game/gameplay/GameplayMonsterSpawn.h"
#include "game/party/Party.h"
#include "game/rules/roguelike/RoguelikeRules.h"
#include "game/scene/IndoorSceneRuntime.h"
#include "game/scene/OutdoorSceneRuntime.h"
#include "game/tables/ClassSkillTable.h"
#include "game/tables/ItemTable.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

// GameApplication's part of dungeon runs (game/content/DungeonRun.h), kept out of GameApplication.cpp so that file
// only carries the calls: the start of a new session, a move to another map, the party's defeat and the main menu's
// status line. Every function here does nothing for a game whose ruleset package has no run.yml.

namespace OpenYAMM::Game
{
std::optional<DungeonRunDefinition> GameApplication::loadSessionDungeonRun() const
{
    if (m_pAssetFileSystem == nullptr || m_gameSession.ruleset().isStandard())
    {
        return std::nullopt;
    }

    std::string error;
    std::optional<DungeonRunDefinition> dungeonRun =
        loadDungeonRunDefinition(*m_pAssetFileSystem, m_gameSession.ruleset().id, error);

    if (!dungeonRun.has_value() && !error.empty())
    {
        std::cerr << "GameApplication: " << error << '\n';
    }

    return dungeonRun;
}

int32_t GameApplication::dungeonRunEntered()
{
    // The live map's copy of the named globals is the newest; the session's follows on the next capture.
    if (m_pMapSceneRuntime != nullptr)
    {
        if (const EventRuntimeState *pRuntimeState = m_pMapSceneRuntime->eventRuntimeState())
        {
            m_gameSession.mergeNamedGlobalVarsFromRuntime(*pRuntimeState);
        }
    }

    return std::max(m_gameSession.namedGlobalVar(DungeonRunEnteredVariable, 0), 0);
}

void GameApplication::setDungeonRunEntered(int32_t entered)
{
    m_gameSession.setNamedGlobalVar(DungeonRunEnteredVariable, entered);

    if (RoguelikeRules *pRules = m_gameSession.roguelikeRules())
    {
        pRules->state().entered = entered;
    }

    if (m_pMapSceneRuntime != nullptr)
    {
        if (EventRuntimeState *pRuntimeState = m_pMapSceneRuntime->eventRuntimeState())
        {
            pRuntimeState->namedGlobalVars[DungeonRunEnteredVariable] = entered;
        }
    }
}

std::optional<GameApplication::MapStartDestination> GameApplication::beginDungeonRun()
{
    const std::optional<DungeonRunDefinition> dungeonRun = loadSessionDungeonRun();
    const DungeonRunStop *pStop =
        dungeonRun.has_value() ? dungeonRunStop(*dungeonRun, m_gameSession.randomRootSeed(), 0) : nullptr;

    if (pStop == nullptr)
    {
        return std::nullopt;
    }

    // The session is new and has no map yet: the first map's runtime takes the variable from the session.
    m_gameSession.setNamedGlobalVar(DungeonRunEnteredVariable, 1);

    if (RoguelikeRules *pRules = m_gameSession.roguelikeRules())
    {
        pRules->state() = RoguelikeRunState{.entered = 1};
        m_storedDungeonRunState.reset();
        m_dungeonRunChoice = DungeonRunChoice::None;
    }
    GAMEPLAY_DEBUG_TRACE("dungeon_run_started map=\"" + pStop->mapFileName + "\"");
    return MapStartDestination{
        .mapFileName = pStop->mapFileName,
        .start = DebugMapJumpStart{
            .x = pStop->x,
            .y = pStop->y,
            .z = pStop->z,
            .directionYawUnits = pStop->yawUnits,
        },
    };
}

bool GameApplication::redirectMapMoveForDungeonRun(EventRuntimeState::PendingMapMove &pendingMapMove)
{
    const std::optional<DungeonRunDefinition> dungeonRun = loadSessionDungeonRun();

    if (!dungeonRun.has_value())
    {
        return true;
    }

    const int32_t entered = dungeonRunEntered();
    const DungeonRunStop *pStop =
        dungeonRunStop(*dungeonRun, m_gameSession.randomRootSeed(), static_cast<size_t>(entered));

    if (pStop == nullptr)
    {
        // The party left the run's last dungeon: the move is dropped and the next frame opens the main menu.
        m_mainMenuStatus = dungeonRunEndText(false, static_cast<size_t>(entered));

        if (const RoguelikeRules *pRules = m_gameSession.roguelikeRules())
        {
            m_mainMenuStatus = "Victory! " + m_mainMenuStatus + " Level-ups: "
                + std::to_string(pRules->state().levelUps) + ".";
        }

        if (EventRuntimeState *pRuntimeState = m_gameplayController.eventRuntimeState())
        {
            pRuntimeState->pendingReturnToMainMenu = true;
        }

        GAMEPLAY_DEBUG_TRACE("dungeon_run_completed dungeons=" + std::to_string(entered));
        return false;
    }

    GAMEPLAY_DEBUG_TRACE(
        "dungeon_run_next from=\"" + m_gameSession.currentMapFileName() + "\""
        + " original_target=\"" + pendingMapMove.mapName.value_or(std::string()) + "\""
        + " map=\"" + pStop->mapFileName + "\""
        + " entered=" + std::to_string(entered + 1));
    restDungeonRunPartyBetweenStops();
    setDungeonRunEntered(entered + 1);
    const int32_t normalizedYawUnits = ((pStop->yawUnits % 2048) + 2048) % 2048;
    pendingMapMove.mapName = pStop->mapFileName;
    pendingMapMove.x = pStop->x;
    pendingMapMove.y = pStop->y;
    pendingMapMove.z = pStop->z;
    pendingMapMove.directionDegrees = normalizedYawUnits * 360 / 2048;
    pendingMapMove.useMapStartPosition = false;
    pendingMapMove.startPointName.clear();
    pendingMapMove.traceDestinationName = pStop->mapFileName;
    return true;
}

bool GameApplication::processPendingDungeonRunStart()
{
    if (!m_pendingDungeonRunStart.has_value())
    {
        return false;
    }

    const PendingDungeonRunStart pendingStart = *m_pendingDungeonRunStart;
    m_pendingDungeonRunStart.reset();
    std::string error;

    if (!activateRuleset(pendingStart.rulesetId, error))
    {
        m_debugConsole.addMessage(DebugConsole::MessageKind::Error, error);
        return true;
    }

    NewGameOptions options;
    options.ruleset.id = pendingStart.rulesetId;
    options.ruleset.options = m_gameDataLoader.activeRuleset().defaultOptions();
    options.randomRootSeed = pendingStart.rootSeed;
    // A test party stands in for party creation; a run is played by four.
    std::vector<Character> characters = Party::createDefaultSeed().members;

    if (characters.size() > 4)
    {
        characters.resize(4);
    }

    std::string runError;
    const std::optional<DungeonRunDefinition> run =
        loadDungeonRunDefinition(*m_pAssetFileSystem, pendingStart.rulesetId, runError);

    if (run.has_value() && run->rules.has_value())
    {
        // A run with rules starts as a new game does: four heroes of level 1 with their classes' first skills.
        characters.clear();
        const std::pair<const char *, const char *> heroes[] = {
            {"Roderick", "Knight"}, {"Alexis", "Archer"}, {"Serena", "Cleric"}, {"Zoltan", "Sorcerer"}};
        const ClassSkillTable &classSkills = m_gameDataLoader.getClassSkillTable();

        for (const auto &[pName, pClassName] : heroes)
        {
            Character hero = {};
            hero.name = pName;
            hero.className = pClassName;
            hero.role = pClassName;
            hero.characterDataId = static_cast<uint32_t>(characters.size() + 1);
            hero.portraitPictureId = static_cast<uint32_t>(characters.size());
            hero.birthYear = 1150;
            hero.level = 1;
            hero.might = hero.intellect = hero.personality = hero.endurance = 15;
            hero.speed = hero.accuracy = hero.luck = 15;

            for (const CharacterSkill &skill : classSkills.getDefaultSkillsForClass(hero.className))
            {
                hero.skills[skill.name] = {skill.name, 1, SkillMastery::Normal};
            }

            GameMechanics::refreshCharacterBaseResources(hero, true, &m_gameDataLoader.getClassMultiplierTable());
            characters.push_back(std::move(hero));
        }
    }

    if (!startNewSessionFromCharacterCreation(characters, options, true))
    {
        m_debugConsole.addMessage(DebugConsole::MessageKind::Error, "The new run did not start.");
        return true;
    }

    m_gameSession.playthroughStatistics().markAssisted(std::string(Engine::trNoop("debug console")), "run new");

    m_debugConsole.addMessage(
        DebugConsole::MessageKind::Success,
        "Run started in " + m_gameSession.currentMapFileName()
            + ", seed " + std::to_string(m_gameSession.randomRootSeed()) + ".");
    return true;
}

void GameApplication::registerDungeonRunConsoleCommand()
{
    const auto commandResult = [](bool success, const std::string &message)
    {
        return DebugConsole::CommandResult{.success = success, .message = message};
    };

    m_debugConsole.registerCommand({
        .name = "run",
        .description = "Dungeon runs: this run's progress, a new run with a test party, or an answer to its choice.",
        .usage = "run | run new [seed] | run pick <n> | run leave | run close",
        .callback = [this, commandResult](const DebugConsole::CommandContext &context)
        {
            const std::string action = context.args.empty() ? std::string("status") : toLowerCopy(context.args[0]);

            if (action == "new")
            {
                if (m_pAssetFileSystem == nullptr)
                {
                    return commandResult(false, "Game data unavailable.");
                }

                std::optional<uint64_t> rootSeed;

                if (context.args.size() >= 2)
                {
                    const std::string &text = context.args[1];
                    uint64_t value = 0;
                    const std::from_chars_result parsed =
                        std::from_chars(text.data(), text.data() + text.size(), value);

                    if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size())
                    {
                        return commandResult(false, "Usage: run new [seed], the seed a whole number");
                    }

                    rootSeed = value;
                }

                std::vector<RulesetDefinition> rulesets;
                std::string error;

                if (!loadAvailableRulesets(*m_pAssetFileSystem, rulesets, error))
                {
                    return commandResult(false, error);
                }

                for (const RulesetDefinition &ruleset : rulesets)
                {
                    if (loadDungeonRunDefinition(*m_pAssetFileSystem, ruleset.id, error).has_value())
                    {
                        m_pendingDungeonRunStart =
                            PendingDungeonRunStart{.rulesetId = ruleset.id, .rootSeed = rootSeed};
                        return commandResult(true, "Starting a new " + ruleset.name + " run.");
                    }

                    if (!error.empty())
                    {
                        return commandResult(false, error);
                    }
                }

                return commandResult(false, "No installed ruleset is a dungeon run.");
            }

            if (action == "pick")
            {
                // Answers the run's open choice box (a card, the way to the door) without the mouse.
                IGameplayWorldRuntime *pWorldRuntime = m_gameSession.activeWorldRuntime();
                EventRuntimeState *pRuntimeState =
                    pWorldRuntime != nullptr ? pWorldRuntime->eventRuntimeState() : nullptr;
                int answer = 0;

                if (context.args.size() >= 2)
                {
                    const std::string &text = context.args[1];
                    std::from_chars(text.data(), text.data() + text.size(), answer);
                }

                if (pRuntimeState == nullptr || m_dungeonRunChoice == DungeonRunChoice::None || answer <= 0
                    || !pRuntimeState->pendingInputPrompt
                    || static_cast<size_t>(answer) > pRuntimeState->pendingInputPrompt->answers.size())
                {
                    return commandResult(false, "Usage: run pick <n>, while the run asks to choose");
                }

                const std::string picked = pRuntimeState->pendingInputPrompt->answers[static_cast<size_t>(answer - 1)];
                pRuntimeState->pendingInputPrompt.reset();
                pRuntimeState->pendingDialogueContext.reset();
                pRuntimeState->messages.clear();
                m_gameSession.gameplayScreenRuntime().activeEventDialog() = {};
                answerDungeonRunChoice(answer);

                // The answer may move the party (to the entrance, out of the camp).
                if (pRuntimeState->pendingMapMove)
                {
                    processPendingMapMove();
                }

                updateDungeonRunRules();
                return commandResult(true, "Picked: " + picked + "; map " + m_gameSession.currentMapFileName());
            }

            if (action == "close")
            {
                // Closes the open dialog (a camp's shop), as Escape does.
                m_gameSession.gameplayScreenRuntime().closeActiveEventDialog();
                updateDungeonRunRules();
                return commandResult(true, m_dungeonRunCampOpen ? "The camp." : "Closed.");
            }

            if (action == "leave")
            {
                // Walks out of the dungeon as its exit does, without the exit's own event and dialog.
                IGameplayWorldRuntime *pWorldRuntime = m_gameSession.activeWorldRuntime();
                EventRuntimeState *pRuntimeState =
                    pWorldRuntime != nullptr ? pWorldRuntime->eventRuntimeState() : nullptr;

                if (pRuntimeState == nullptr || !loadSessionDungeonRun().has_value())
                {
                    return commandResult(false, "This game is not a dungeon run.");
                }

                const std::string from = m_gameSession.currentMapFileName();
                EventRuntimeState::PendingMapMove move = {};
                move.mapName = "dungeon_run.exit";
                move.useMapStartPosition = true;
                pRuntimeState->pendingMapMove = std::move(move);
                processPendingMapMove();
                updateDungeonRunRules();

                if (m_gameSession.currentMapFileName() != from)
                {
                    return commandResult(true, "Left " + from + " for " + m_gameSession.currentMapFileName() + ".");
                }

                if (m_dungeonRunCampOpen)
                {
                    return commandResult(true, "The camp.");
                }

                if (dungeonRunExitSealed())
                {
                    return commandResult(true, "The way out is sealed.");
                }

                return commandResult(true, m_mainMenuStatus.empty() ? "Still in " + from + "." : m_mainMenuStatus);
            }

            if (action != "status")
            {
                return commandResult(false, "Usage: run | run new [seed] | run pick <n> | run leave | run close");
            }

            const std::optional<DungeonRunDefinition> dungeonRun = loadSessionDungeonRun();

            if (!dungeonRun.has_value())
            {
                return commandResult(false, "This game is not a dungeon run.");
            }

            const int32_t entered = dungeonRunEntered();
            const uint64_t rootSeed = m_gameSession.randomRootSeed();
            const std::string &currentMap = m_gameSession.currentMapFileName();
            const DungeonRunStop *pCurrent = entered > 0
                ? dungeonRunStop(*dungeonRun, rootSeed, static_cast<size_t>(entered - 1))
                : nullptr;
            const bool onCourse =
                pCurrent != nullptr && toLowerCopy(pCurrent->mapFileName) == toLowerCopy(currentMap);
            std::ostringstream out;
            out << "Run: dungeon " << entered << " of " << dungeonRunOrder(*dungeonRun, rootSeed).size()
                << ", map " << currentMap << (onCourse ? " (on course)" : " (off course)")
                << ", seed " << rootSeed << ", order:";

            for (size_t index : dungeonRunOrder(*dungeonRun, rootSeed))
            {
                out << ' ' << dungeonRun->dungeons[index].mapFileName;
            }

            if (const RoguelikeRules *pRules = m_gameSession.roguelikeRules())
            {
                static const char *const GuardianNames[] = {"not called", "called", "alive", "slain"};
                const RoguelikeRunState &state = pRules->state();
                IGameplayWorldRuntime *pWorldRuntime = m_gameSession.activeWorldRuntime();
                MapKillProgress progress;
                out << "; level-ups " << state.levelUps << " of " << pRules->rules().totalLevelUps()
                    << ", guardian " << GuardianNames[std::clamp(state.guardian, 0, 3)];

                if (pWorldRuntime != nullptr && pWorldRuntime->mapKillProgress(progress))
                {
                    out << ", slain " << static_cast<long long>(progress.slainHealth) << " of "
                        << static_cast<long long>(progress.countedHealth) << " hit points, toward the guardian "
                        << static_cast<int>(runClearFraction(pRules->rules(), progress) * 100.0) << "%";
                }

                if (const Party *pParty = pWorldRuntime != nullptr ? pWorldRuntime->party() : nullptr)
                {
                    out << "; party";

                    for (const Character &member : pParty->members())
                    {
                        const auto pending = member.eventVariables.find(RunPendingCardsVariable);
                        out << ' ' << member.name << " L" << member.level << ' ' << member.className
                            << " cards owed "
                            << (pending != member.eventVariables.end() ? pending->second : 0);
                    }
                }

                if (pWorldRuntime != nullptr && pWorldRuntime->eventRuntimeState() != nullptr
                    && pWorldRuntime->eventRuntimeState()->pendingInputPrompt
                    && m_dungeonRunChoice != DungeonRunChoice::None)
                {
                    out << "; asks:";
                    int number = 0;

                    for (const std::string &answer : pWorldRuntime->eventRuntimeState()->pendingInputPrompt->answers)
                    {
                        out << " [" << ++number << "] " << answer;
                    }
                }
            }

            return commandResult(true, out.str());
        },
        .readOnlyWith = [](const std::vector<std::string> &args)
        {
            return args.empty() || toLowerCopy(args.front()) == "status";
        }});
}

const DungeonRunStop *GameApplication::currentDungeonRunStop()
{
    const RoguelikeRules *pRules = m_gameSession.roguelikeRules();

    if (pRules == nullptr || pRules->state().entered <= 0)
    {
        return nullptr;
    }

    return dungeonRunStop(
        pRules->run(), m_gameSession.randomRootSeed(), static_cast<size_t>(pRules->state().entered - 1));
}

void GameApplication::storeDungeonRunState()
{
    const RoguelikeRules *pRules = m_gameSession.roguelikeRules();

    if (pRules == nullptr || (m_storedDungeonRunState.has_value() && *m_storedDungeonRunState == pRules->state()))
    {
        return;
    }

    std::unordered_map<std::string, int32_t> variables;
    writeRunState(pRules->state(), variables);
    EventRuntimeState *pRuntimeState =
        m_pMapSceneRuntime != nullptr ? m_pMapSceneRuntime->eventRuntimeState() : nullptr;

    for (const auto &[name, value] : variables)
    {
        m_gameSession.setNamedGlobalVar(name, value);

        if (pRuntimeState != nullptr)
        {
            pRuntimeState->namedGlobalVars[name] = value;
        }
    }

    m_storedDungeonRunState = pRules->state();
}

bool GameApplication::dungeonRunExitSealed(bool announce)
{
    const RoguelikeRules *pRules = m_gameSession.roguelikeRules();

    if (pRules == nullptr || pRules->state().entered <= 0
        || pRules->state().guardian == RoguelikeRunState::GuardianSlain)
    {
        return false;
    }

    if (announce)
    {
        if (EventRuntimeState *pRuntimeState = m_gameplayController.eventRuntimeState())
        {
            pRuntimeState->statusMessages.push_back(
                pRules->state().guardian == RoguelikeRunState::GuardianAlive
                    ? Engine::tr("The way out is sealed: slay the guardian at the entrance.")
                    : Engine::tr("The way out is sealed: clear more of the dungeon to call its guardian."));
        }
    }

    return true;
}

void GameApplication::restDungeonRunPartyBetweenStops()
{
    RoguelikeRules *pRules = m_gameSession.roguelikeRules();

    if (pRules == nullptr)
    {
        return;
    }

    restoreDungeonRunParty();
    RoguelikeRunState &state = pRules->state();
    state.guardian = RoguelikeRunState::GuardianNone;
    state.guardianActor = -1;
    state.doorOffered = 0;
    m_dungeonRunCampOpen = false;
    m_dungeonRunCampMove.reset();
}

void GameApplication::restoreDungeonRunParty()
{
    IGameplayWorldRuntime *pWorldRuntime = m_gameSession.activeWorldRuntime();
    Party *pParty = pWorldRuntime != nullptr ? pWorldRuntime->party() : nullptr;

    // Hit and spell points only: conditions, the dead and broken gear stay as they are.
    for (size_t memberIndex = 0; pParty != nullptr && memberIndex < pParty->members().size(); ++memberIndex)
    {
        Character *pMember = pParty->member(memberIndex);
        GameMechanics::refreshCharacterBaseResources(*pMember, false, pParty->classMultiplierTable());

        if (pMember->health > 0)
        {
            pMember->health = std::max(
                pMember->health,
                GameMechanics::calculateEffectiveCharacterMaxHealth(*pMember, pParty->classMultiplierTable()));
            pMember->spellPoints = std::max(
                pMember->spellPoints,
                GameMechanics::calculateEffectiveCharacterMaxSpellPoints(*pMember, pParty->classMultiplierTable()));
        }
    }
}

std::vector<DungeonRunShop> GameApplication::currentDungeonRunShops()
{
    std::vector<DungeonRunShop> shops;

    if (const RoguelikeRules *pRules = m_gameSession.roguelikeRules())
    {
        for (const DungeonRunShop &shop : pRules->rules().shops)
        {
            // No camp follows the run's last dungeon.
            if (shop.afterStop == pRules->state().entered
                && static_cast<size_t>(shop.afterStop) < pRules->rules().route.size())
            {
                shops.push_back(shop);
            }
        }
    }

    return shops;
}

bool GameApplication::openDungeonRunCamp()
{
    std::vector<std::string> answers;

    for (const DungeonRunShop &shop : currentDungeonRunShops())
    {
        answers.push_back(shop.name);
    }

    answers.push_back("Go on to the next dungeon");

    if (!openDungeonRunChoice(
            "Camp",
            "The party rests: hit points and spell points are restored. The shops are open before the next dungeon.",
            answers))
    {
        return false;
    }

    m_dungeonRunChoice = DungeonRunChoice::Camp;
    return true;
}

bool GameApplication::holdMapMoveForDungeonRunCamp(const EventRuntimeState::PendingMapMove &pendingMapMove)
{
    const RoguelikeRules *pRules = m_gameSession.roguelikeRules();

    if (pRules == nullptr || pRules->state().guardian != RoguelikeRunState::GuardianSlain)
    {
        return false;
    }

    if (m_dungeonRunCampLeaving)
    {
        m_dungeonRunCampLeaving = false;
        return false;
    }

    const bool leavesMap = pendingMapMove.mapName && !pendingMapMove.mapName->empty()
        && *pendingMapMove.mapName != "0" && *pendingMapMove.mapName != "0."
        && (pendingMapMove.useMapStartPosition
            || toLowerCopy(*pendingMapMove.mapName) != toLowerCopy(m_gameSession.currentMapFileName()));

    if (!leavesMap || currentDungeonRunShops().empty())
    {
        return false;
    }

    restoreDungeonRunParty();

    // The camp lasts until the morning, when its shops are open (the houses keep their hours).
    if (IGameplayWorldRuntime *pWorldRuntime = m_gameSession.activeWorldRuntime())
    {
        const float minuteOfDay = std::fmod(pWorldRuntime->gameMinutes(), 1440.0f);

        if (minuteOfDay < 9.0f * 60.0f || minuteOfDay >= 16.0f * 60.0f)
        {
            const float untilMorning = std::fmod(9.0f * 60.0f - minuteOfDay + 1440.0f, 1440.0f);
            m_gameplayController.advanceGameMinutes(untilMorning);
        }
    }

    m_dungeonRunCampMove = pendingMapMove;
    m_dungeonRunCampOpen = true;
    openDungeonRunCamp();
    return true;
}

void GameApplication::promoteDungeonRunParty()
{
    RoguelikeRules *pRules = m_gameSession.roguelikeRules();
    IGameplayWorldRuntime *pWorldRuntime = m_gameSession.activeWorldRuntime();
    Party *pParty = pWorldRuntime != nullptr ? pWorldRuntime->party() : nullptr;

    if (pRules == nullptr || pParty == nullptr || pParty->classSkillTable() == nullptr)
    {
        return;
    }

    pRules->state().promoted = 1;
    const ClassSkillTable &classSkills = *pParty->classSkillTable();
    std::string promoted;

    for (size_t memberIndex = 0; memberIndex < pParty->members().size(); ++memberIndex)
    {
        const Character *pMember = pParty->member(memberIndex);
        const ClassMetadataEntry *pClass = classSkills.classMetadata(pMember->className);
        // The next class of the hero's line: more hit and spell points a level, counted over every level the hero
        // has, and higher skill caps.
        const std::vector<std::string> next = pClass != nullptr
            ? classSkills.classNamesOfKind(pClass->classKind, pClass->promotionStep + 1)
            : std::vector<std::string>();

        if (next.empty())
        {
            continue;
        }

        const std::string previous = displayClassName(pMember->className);
        pParty->setMemberClassName(memberIndex, next.front(), true);
        promoted += (promoted.empty() ? "" : ", ") + previous + " to " + displayClassName(next.front());
    }

    GAMEPLAY_DEBUG_TRACE("dungeon_run_promotion \"" + promoted + "\"");
    EventRuntimeState *pRuntimeState = pWorldRuntime->eventRuntimeState();

    if (!promoted.empty() && pRuntimeState != nullptr)
    {
        pRuntimeState->statusMessages.push_back("Promotion: " + promoted + ".");
    }
}

void GameApplication::placeDungeonRunGuardian()
{
    RoguelikeRules *pRules = m_gameSession.roguelikeRules();
    IGameplayWorldRuntime *pWorldRuntime = m_gameSession.activeWorldRuntime();
    const DungeonRunStop *pStop = currentDungeonRunStop();
    MapKillProgress progress;

    if (pRules == nullptr || pWorldRuntime == nullptr)
    {
        return;
    }

    RoguelikeRunState &state = pRules->state();
    std::vector<size_t> spawned;

    if (pStop != nullptr && pWorldRuntime->mapKillProgress(progress) && progress.strongestMonsterId != 0)
    {
        // The dungeon's strongest kind stands where the party came in, with several times its health.
        GameplayMonsterSpawnRequest request = {};
        request.monsterId = progress.strongestMonsterId;
        request.count = 1;
        request.x = static_cast<float>(pStop->x);
        request.y = static_cast<float>(pStop->y);
        request.z = static_cast<float>(pStop->z);
        request.radius = 96.0f;
        request.hostile = true;
        request.healthMultiplier = pRules->rules().guardianHealthScale;
        request.seed = m_gameSession.randomRootSeed() ^ (static_cast<uint64_t>(state.entered) << 20) ^ 0x6775617264ull;
        spawned = pWorldRuntime->spawnMonsters(request);
    }

    EventRuntimeState *pRuntimeState = pWorldRuntime->eventRuntimeState();

    if (spawned.empty())
    {
        // No place for a guardian: the clearing alone opens the way.
        state.guardian = RoguelikeRunState::GuardianSlain;
        GAMEPLAY_DEBUG_TRACE("dungeon_run_guardian_missing map=\"" + m_gameSession.currentMapFileName() + "\"");

        if (pRuntimeState != nullptr)
        {
            pRuntimeState->statusMessages.push_back(Engine::tr("The dungeon is cleared. The way out is open."));
        }

        return;
    }

    state.guardian = RoguelikeRunState::GuardianAlive;
    state.guardianActor = static_cast<int>(spawned.front());
    GAMEPLAY_DEBUG_TRACE(
        "dungeon_run_guardian map=\"" + m_gameSession.currentMapFileName() + "\""
        + " monster=" + std::to_string(progress.strongestMonsterId)
        + " actor=" + std::to_string(state.guardianActor));

    if (pRuntimeState != nullptr)
    {
        pRuntimeState->statusMessages.push_back(
            Engine::tr("The guardian has come to the entrance. Slay it to open the way out."));
    }
}

void GameApplication::deliverDungeonRunBooks()
{
    IGameplayWorldRuntime *pWorldRuntime = m_gameSession.activeWorldRuntime();
    Party *pParty = pWorldRuntime != nullptr ? pWorldRuntime->party() : nullptr;

    if (pParty == nullptr || m_gameSession.namedGlobalVar("run.books", 0) <= 0)
    {
        return;
    }

    // Owed books wait in the save's variables (run.book.<n> = spell * 16 + the hero's place) until the hero's pack,
    // or anyone's, has room.
    const int32_t owed = m_gameSession.namedGlobalVar("run.books", 0);
    const ItemTable &itemTable = m_gameDataLoader.getItemTable();
    const std::string worldId = worldIdOfMap(m_gameSession.currentMapFileName());
    std::vector<int32_t> waiting;

    for (int32_t index = 0; index < owed; ++index)
    {
        const int32_t packed = m_gameSession.namedGlobalVar("run.book." + std::to_string(index), 0);
        const uint32_t spellId = static_cast<uint32_t>(packed / 16);
        const size_t memberIndex = static_cast<size_t>(packed % 16);
        const std::optional<uint32_t> itemId = itemTable.spellbookItemIdForSpell(spellId, worldId);

        if (packed <= 0 || !itemId.has_value())
        {
            continue;
        }

        if (!pParty->grantItemToMember(memberIndex, *itemId))
        {
            bool granted = false;

            for (size_t other = 0; !granted && other < pParty->members().size(); ++other)
            {
                granted = pParty->grantItemToMember(other, *itemId);
            }

            if (!granted)
            {
                waiting.push_back(packed);
            }
        }
    }

    if (static_cast<int32_t>(waiting.size()) == owed)
    {
        return;
    }

    EventRuntimeState *pRuntimeState = pWorldRuntime->eventRuntimeState();
    const auto setVariable = [this, pRuntimeState](const std::string &name, int32_t value)
    {
        m_gameSession.setNamedGlobalVar(name, value);

        if (pRuntimeState != nullptr)
        {
            pRuntimeState->namedGlobalVars[name] = value;
        }
    };

    for (int32_t index = 0; index < owed; ++index)
    {
        setVariable(
            "run.book." + std::to_string(index),
            index < static_cast<int32_t>(waiting.size()) ? waiting[static_cast<size_t>(index)] : 0);
    }

    setVariable("run.books", static_cast<int32_t>(waiting.size()));
}

bool GameApplication::openDungeonRunChoice(
    const std::string &title, const std::string &text, const std::vector<std::string> &answers)
{
    IGameplayWorldRuntime *pWorldRuntime = m_gameSession.activeWorldRuntime();
    EventRuntimeState *pRuntimeState = pWorldRuntime != nullptr ? pWorldRuntime->eventRuntimeState() : nullptr;

    if (pRuntimeState == nullptr
        || pRuntimeState->pendingInputPrompt
        || pRuntimeState->pendingDialogueContext
        || pRuntimeState->pendingMapMove
        || m_gameSession.gameplayScreenRuntime().activeEventDialog().isActive)
    {
        return false;
    }

    EventRuntimeState::PendingInputPrompt prompt = {};
    prompt.kind = EventRuntimeState::PendingInputPrompt::Kind::Choice;
    prompt.eventId = EventRuntimeState::RulesetChoiceEventId;
    prompt.text = text;
    prompt.answers = answers;

    for (size_t answerIndex = 0; answerIndex < answers.size(); ++answerIndex)
    {
        prompt.answerContinueSteps.push_back(static_cast<uint8_t>(answerIndex + 1));
    }

    pRuntimeState->messages.clear();
    pRuntimeState->messages.push_back(text);
    pRuntimeState->pendingInputPrompt = std::move(prompt);
    EventRuntimeState::PendingDialogueContext context = {};
    context.kind = DialogueContextKind::MapEvent;
    context.titleOverride = title;
    pRuntimeState->pendingDialogueContext = std::move(context);
    m_gameSession.gameplayScreenRuntime().presentPendingEventDialog(0, true);
    return true;
}

void GameApplication::answerDungeonRunChoice(int answer)
{
    RoguelikeRules *pRules = m_gameSession.roguelikeRules();
    IGameplayWorldRuntime *pWorldRuntime = m_gameSession.activeWorldRuntime();
    Party *pParty = pWorldRuntime != nullptr ? pWorldRuntime->party() : nullptr;
    const DungeonRunChoice choice = m_dungeonRunChoice;
    m_dungeonRunChoice = DungeonRunChoice::None;

    if (pRules == nullptr || pParty == nullptr || answer <= 0)
    {
        return;
    }

    if (choice == DungeonRunChoice::Door)
    {
        const DungeonRunStop *pStop = currentDungeonRunStop();
        EventRuntimeState *pRuntimeState = pWorldRuntime->eventRuntimeState();

        if (answer == 1 && pStop != nullptr && pRuntimeState != nullptr)
        {
            // A move within the map, as the dungeons' own teleporters make.
            EventRuntimeState::PendingMapMove move = {};
            move.x = pStop->x;
            move.y = pStop->y;
            move.z = pStop->z;
            move.directionDegrees = (((pStop->yawUnits % 2048) + 2048) % 2048) * 360 / 2048;
            pRuntimeState->pendingMapMove = std::move(move);
        }

        return;
    }

    if (choice == DungeonRunChoice::Camp)
    {
        const std::vector<DungeonRunShop> shops = currentDungeonRunShops();
        EventRuntimeState *pRuntimeState = pWorldRuntime->eventRuntimeState();

        if (pRuntimeState == nullptr)
        {
            return;
        }

        if (static_cast<size_t>(answer) <= shops.size())
        {
            // The shop is a house of the game: its own dialog, stock and trade. The camp's menu returns after it.
            const uint32_t houseId = shops[static_cast<size_t>(answer - 1)].houseId;
            pRuntimeState->messages.clear();
            EventRuntimeState::PendingDialogueContext context = {};
            context.kind = DialogueContextKind::HouseService;
            context.sourceId = houseId;
            context.hostHouseId = houseId;
            pRuntimeState->dialogueState.hostHouseId = houseId;
            pRuntimeState->pendingDialogueContext = std::move(context);
            m_gameSession.gameplayScreenRuntime().presentPendingEventDialog(0, true);
            return;
        }

        m_dungeonRunCampOpen = false;

        if (m_dungeonRunCampMove.has_value())
        {
            m_dungeonRunCampLeaving = true;
            pRuntimeState->pendingMapMove = std::move(m_dungeonRunCampMove);
            m_dungeonRunCampMove.reset();
        }

        return;
    }

    Character *pMember = choice == DungeonRunChoice::Card ? pParty->member(m_dungeonRunChoiceMember) : nullptr;

    if (pMember == nullptr || static_cast<size_t>(answer) > m_dungeonRunChoiceCards.size())
    {
        return;
    }

    const RunCard card = m_dungeonRunChoiceCards[static_cast<size_t>(answer - 1)];
    const ItemTable &itemTable = m_gameDataLoader.getItemTable();
    const std::string worldId = worldIdOfMap(m_gameSession.currentMapFileName());
    std::vector<int32_t> owedBooks;
    int32_t owed = m_gameSession.namedGlobalVar("run.books", 0);

    for (int32_t index = 0; index < owed; ++index)
    {
        owedBooks.push_back(m_gameSession.namedGlobalVar("run.book." + std::to_string(index), 0) / 16);
    }

    // A book the party carries or is owed is not given twice.
    const auto spellTaken = [&](uint32_t spellId)
    {
        const std::optional<uint32_t> itemId = itemTable.spellbookItemIdForSpell(spellId, worldId);
        return !itemId.has_value()
            || pParty->hasItemAnywhere(*itemId)
            || std::find(owedBooks.begin(), owedBooks.end(), static_cast<int32_t>(spellId)) != owedBooks.end();
    };
    const std::vector<uint32_t> bookSpells = applyRunCard(
        pRules->rules(),
        *pMember,
        card,
        pParty->classMultiplierTable(),
        m_gameSession.randomRootSeed(),
        m_dungeonRunChoiceMember,
        spellTaken);
    EventRuntimeState *pRuntimeState = pWorldRuntime->eventRuntimeState();

    for (uint32_t spellId : bookSpells)
    {
        const std::string name = "run.book." + std::to_string(owed);
        const int32_t packed = static_cast<int32_t>(spellId * 16 + m_dungeonRunChoiceMember);
        m_gameSession.setNamedGlobalVar(name, packed);
        m_gameSession.setNamedGlobalVar("run.books", ++owed);

        if (pRuntimeState != nullptr)
        {
            pRuntimeState->namedGlobalVars[name] = packed;
            pRuntimeState->namedGlobalVars["run.books"] = owed;
        }
    }

    GAMEPLAY_DEBUG_TRACE(
        "dungeon_run_card member=" + std::to_string(m_dungeonRunChoiceMember)
        + " card=\"" + card.label + "\" books=" + std::to_string(bookSpells.size()));

    if (pRuntimeState != nullptr)
    {
        pRuntimeState->statusMessages.push_back(pMember->name + ": " + card.label);
    }
}

void GameApplication::updateDungeonRunRules()
{
    RoguelikeRules *pRules = m_gameSession.roguelikeRules();
    IGameplayWorldRuntime *pWorldRuntime = m_gameSession.activeWorldRuntime();
    EventRuntimeState *pRuntimeState = pWorldRuntime != nullptr ? pWorldRuntime->eventRuntimeState() : nullptr;
    Party *pParty = pWorldRuntime != nullptr ? pWorldRuntime->party() : nullptr;

    if (pRules == nullptr || pRuntimeState == nullptr || pParty == nullptr || m_pMapSceneRuntime == nullptr
        || m_screenManager.activeScreen() != nullptr)
    {
        return;
    }

    RoguelikeRunState &state = pRules->state();

    if (pRuntimeState->rulesetChoiceAnswer.has_value())
    {
        const int answer = *pRuntimeState->rulesetChoiceAnswer;
        pRuntimeState->rulesetChoiceAnswer.reset();
        answerDungeonRunChoice(answer);
    }

    if (state.guardian == RoguelikeRunState::GuardianDue)
    {
        placeDungeonRunGuardian();
    }

    if (state.guardian == RoguelikeRunState::GuardianSlain && state.promoted == 0
        && pRules->rules().promotionAfterStop > 0 && state.entered == pRules->rules().promotionAfterStop)
    {
        promoteDungeonRunParty();
    }

    const bool dialogBusy = pRuntimeState->pendingInputPrompt.has_value()
        || pRuntimeState->pendingDialogueContext.has_value()
        || m_gameSession.gameplayScreenRuntime().activeEventDialog().isActive;

    if (m_dungeonRunChoice != DungeonRunChoice::None && !dialogBusy)
    {
        // Closed without an answer: the door's offer is declined, the camp is left for the dungeon (its move
        // dropped), a card is offered again shortly.
        if (m_dungeonRunChoice == DungeonRunChoice::Camp)
        {
            m_dungeonRunCampOpen = false;
            m_dungeonRunCampMove.reset();
        }

        m_dungeonRunChoice = DungeonRunChoice::None;
        m_dungeonRunChoiceRetryTicks = SDL_GetTicks() + 400;
    }

    // Back from a shop: the camp's menu again.
    if (!dialogBusy && m_dungeonRunCampOpen && m_dungeonRunChoice == DungeonRunChoice::None
        && !pRuntimeState->pendingMapMove)
    {
        openDungeonRunCamp();
        deliverDungeonRunBooks();
        storeDungeonRunState();
        return;
    }

    if (!dialogBusy && m_dungeonRunChoice == DungeonRunChoice::None && !pRuntimeState->pendingMapMove
        && SDL_GetTicks() >= m_dungeonRunChoiceRetryTicks)
    {
        if (state.guardian == RoguelikeRunState::GuardianAlive && state.doorOffered == 0)
        {
            state.doorOffered = 1;

            if (openDungeonRunChoice(
                    "Guardian",
                    "The dungeon's guardian waits at the entrance. Its death opens the way out.",
                    {"Go to the entrance", "Stay here"}))
            {
                m_dungeonRunChoice = DungeonRunChoice::Door;
            }
        }
        else
        {
            for (size_t memberIndex = 0; memberIndex < pParty->members().size(); ++memberIndex)
            {
                Character *pMember = pParty->member(memberIndex);
                const auto pending = pMember->eventVariables.find(RunPendingCardsVariable);

                if (pending == pMember->eventVariables.end() || pending->second <= 0)
                {
                    continue;
                }

                std::vector<RunCard> cards = dealRunCards(
                    pRules->rules(),
                    *pMember,
                    pParty->classSkillTable(),
                    m_gameSession.randomRootSeed(),
                    memberIndex);

                if (cards.empty())
                {
                    // Nothing left to offer this hero.
                    pending->second = 0;
                    continue;
                }

                std::vector<std::string> answers;

                for (const RunCard &card : cards)
                {
                    answers.push_back(card.label);
                }

                if (openDungeonRunChoice(
                        "Level up",
                        pMember->name + ", level " + std::to_string(pMember->level) + " "
                            + displayClassName(pMember->className) + ": choose one.",
                        answers))
                {
                    m_dungeonRunChoice = DungeonRunChoice::Card;
                    m_dungeonRunChoiceMember = memberIndex;
                    m_dungeonRunChoiceCards = std::move(cards);
                }

                break;
            }
        }
    }

    deliverDungeonRunBooks();
    storeDungeonRunState();
}
}
