#include "game/app/GameApplication.h"

#include "game/StringUtils.h"
#include "game/app/NewGameOptions.h"
#include "game/content/DungeonRun.h"
#include "game/content/RulesetDefinition.h"
#include "game/debug/GameplayDebugTrace.h"
#include "game/events/EventRuntime.h"
#include "game/party/Party.h"
#include "game/scene/IndoorSceneRuntime.h"
#include "game/scene/OutdoorSceneRuntime.h"

#include <algorithm>
#include <charconv>
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
    // The default party stands in for party creation; a run is played by four.
    std::vector<Character> characters = Party::createDefaultSeed().members;

    if (characters.size() > 4)
    {
        characters.resize(4);
    }

    if (!startNewSessionFromCharacterCreation(characters, options, true))
    {
        m_debugConsole.addMessage(DebugConsole::MessageKind::Error, "The new run did not start.");
        return true;
    }

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
        .description = "Dungeon runs: this run's progress, or a new run with the default party.",
        .usage = "run | run new [seed]",
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

            if (action != "status")
            {
                return commandResult(false, "Usage: run | run new [seed]");
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
            out << "Run: dungeon " << entered << " of " << dungeonRun->dungeons.size()
                << ", map " << currentMap << (onCourse ? " (on course)" : " (off course)")
                << ", seed " << rootSeed << ", order:";

            for (size_t index : dungeonRunOrder(*dungeonRun, rootSeed))
            {
                out << ' ' << dungeonRun->dungeons[index].mapFileName;
            }

            return commandResult(true, out.str());
        }});
}
}
