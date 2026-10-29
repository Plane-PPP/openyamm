#pragma once

#include "game/content/ContentManifest.h"

#include "game/events/EventRuntime.h"
#include "game/app/GameSettings.h"
#include "game/gameplay/GameplayFxService.h"
#include "game/gameplay/GameplayActorService.h"
#include "game/gameplay/GameplayCombatController.h"
#include "game/gameplay/GameplayInputController.h"
#include "game/gameplay/GameplayScreenState.h"
#include "game/gameplay/GameplayItemService.h"
#include "game/gameplay/GameplayProjectileService.h"
#include "game/gameplay/GameplaySpellService.h"
#include "game/gameplay/GameplayScreenRuntime.h"
#include "game/gameplay/GameplayDialogController.h"
#include "game/gameplay/GameplayRuntimeInterfaces.h"
#include "game/gameplay/TurnBasedCombatRuntime.h"
#include "game/maps/SaveGame.h"
#include "game/mm9/Mm9QuestMarkers.h"
#include "game/rules/maw/MawRules.h"
#include "game/scene/SceneKind.h"
#include "game/data/GameDataRepository.h"
#include "game/ui/GameplayOverlayTypes.h"
#include "game/ui/GameplayUiController.h"
#include "game/ui/GameplayUiRuntime.h"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace OpenYAMM::Game
{
struct GameplayInputFrame;

struct GameplayUpdateFramePerformanceDiagnostics
{
    bool collected = false;
    uint64_t totalNanoseconds = 0;
    uint64_t sharedFrameStateNanoseconds = 0;
    uint64_t worldInteractionStateNanoseconds = 0;
    uint64_t activeMemberSyncNanoseconds = 0;
    uint64_t sharedInputNanoseconds = 0;
    uint64_t worldMovementNanoseconds = 0;
    uint64_t actorAiNanoseconds = 0;
    uint64_t combatEventsNanoseconds = 0;
    uint64_t interactionFrameNanoseconds = 0;
    uint64_t projectileAndCooldownNanoseconds = 0;
    uint64_t performanceTraceLogNanoseconds = 0;
    GameplayWorldMovementFrameDiagnostics worldMovement = {};
};

struct GameplayUiFramePerformanceDiagnostics
{
    bool collected = false;
    uint64_t totalNanoseconds = 0;
    uint64_t worldUiRenderStateNanoseconds = 0;
    uint64_t standardUiNanoseconds = 0;
    uint64_t pendingSpellOverlayNanoseconds = 0;
    GameplayUiOverlayFramePerformanceDiagnostics overlays = {};
};

class GameSession
{
public:
    GameSession();
    ~GameSession();

    using SaveGameToPathCallback = std::function<bool(
        const std::filesystem::path &,
        const std::string &,
        const std::vector<uint8_t> &,
        std::string &)>;
    using SettingsChangedCallback = std::function<void(const GameSettings &)>;

    void clear();
    void bindDataRepository(const GameDataRepository *pDataRepository);
    bool hasDataRepository() const;
    const GameDataRepository &data() const;

    const std::optional<Party> &partyState() const;
    std::optional<Party> &partyState();
    void setPartyState(const Party &party);
    void setPartyState(Party &&party);

    SceneKind currentSceneKind() const;
    void setCurrentSceneKind(SceneKind sceneKind);

    bool hasCurrentMapFileName() const;
    const std::string &currentMapFileName() const;
    void setCurrentMapFileName(const std::string &mapFileName);
    void setCurrentMapFileName(std::string &&mapFileName);
    float gameMinutes() const;
    void setGameMinutes(float gameMinutes);
    // The rules this game is played by; the loaded tables must match (GameApplication::activateRuleset).
    const RulesetSelection &ruleset() const;
    // Builds the ruleset's rules (MAW) and installs them on the party.
    void setRuleset(const RulesetSelection &ruleset);
    // Changes the options of the game's ruleset mid-game (the Current game page): MAW re-sizes the current map's
    // monsters now and every other map's when it loads. The caller checks what the ruleset allows.
    void setRulesetOptions(const RulesetOptions &options);
    // The rules the ruleset plays by; null for Standard.
    const GameRules *rulesetGameRules() const;
    // The MAW rules of a MAW game, else null.
    MawRules *mawRules();
    const MawRules *mawRules() const;
    // Root of the game's named random streams (game/rules/RandomStreams.h), chosen at new game and saved.
    uint64_t randomRootSeed() const;
    void setRandomRootSeed(uint64_t rootSeed);
    // Where MAW's Beyond Madness death ledger lives, for every MAW game the session creates (scenarios keep it in their
    // own save folder); the default is the game's saves folder.
    void setMadnessLedgerFile(const std::filesystem::path &file);

    GameplayUiController &gameplayUiController();
    const GameplayUiController &gameplayUiController() const;
    GameplayScreenState &gameplayScreenState();
    const GameplayScreenState &gameplayScreenState() const;
    GameplayUiRuntime &gameplayUiRuntime();
    const GameplayUiRuntime &gameplayUiRuntime() const;
    GameplayActorService &gameplayActorService();
    const GameplayActorService &gameplayActorService() const;
    // The active world's taverns seat generated mercenaries (world.yml tavernHirelings).
    void setTavernHirelings(bool enabled)
    {
        m_tavernHirelings = enabled;
    }
    // The active world's Hum (world.yml hum), handed to the party for each map it enters.
    void setWorldHum(const WorldHumDefinition &hum)
    {
        m_worldHum = hum;
    }

    void setWorldFloorBurn(const WorldFloorBurnDefinition &floorBurn)
    {
        m_worldFloorBurn = floorBurn;
    }

    void setWorldObjectiveQBits(const std::vector<uint32_t> &qbits)
    {
        m_worldObjectiveQBits = qbits;
    }
    GameplayCombatController &gameplayCombatController();
    const GameplayCombatController &gameplayCombatController() const;
    GameplayItemService &gameplayItemService();
    const GameplayItemService &gameplayItemService() const;
    GameplayProjectileService &gameplayProjectileService();
    const GameplayProjectileService &gameplayProjectileService() const;
    GameplayFxService &gameplayFxService();
    const GameplayFxService &gameplayFxService() const;
    GameplaySpellService &gameplaySpellService();
    const GameplaySpellService &gameplaySpellService() const;
    TurnBasedCombatRuntime &turnBasedCombatRuntime();
    const TurnBasedCombatRuntime &turnBasedCombatRuntime() const;
    GameplayScreenRuntime &gameplayScreenRuntime();
    const GameplayScreenRuntime &gameplayScreenRuntime() const;
    GameplayDialogController &gameplayDialogController();
    const GameplayDialogController &gameplayDialogController() const;
    GameplayOverlayInteractionState &overlayInteractionState();
    const GameplayOverlayInteractionState &overlayInteractionState() const;
    std::array<uint8_t, SDL_SCANCODE_COUNT> &previousKeyboardState();
    const std::array<uint8_t, SDL_SCANCODE_COUNT> &previousKeyboardState() const;

    IGameplayWorldRuntime *activeWorldRuntime() const;
    void bindActiveWorldRuntime(IGameplayWorldRuntime *pWorldRuntime);
    Mm9QuestMarkerState questMarkerForActor(size_t actorIndex) const;
    const GameplayInputFrame *currentGameplayInputFrame() const;
    void bindCurrentGameplayInputFrame(const GameplayInputFrame *pInputFrame);
    void updateGameplay(
        const GameplayInputFrame &input,
        float deltaSeconds,
        bool collectPerformanceDiagnostics = false);
    void beginFramePerformanceDiagnostics(bool enabled);
    const GameplayUpdateFramePerformanceDiagnostics &lastGameplayUpdateFramePerformanceDiagnostics() const;
    const GameplayUiFramePerformanceDiagnostics &lastGameplayUiFramePerformanceDiagnostics() const;
    void clearSharedInputFrameResult(bool blockWorldInput = false);
    void consumePendingGameplayAudioRequests();
    void renderGameplayUi(int width, int height);
    const GameplaySharedInputFrameResult &sharedInputFrameResult() const;
    bool sharedWorldInteractionBlockedThisFrame() const;
    void requestRelativeMouseMotionReset();
    bool consumeRelativeMouseMotionResetRequest();

    const std::optional<OutdoorPartyRuntime::Snapshot> &outdoorPartyState() const;
    void setOutdoorPartyState(const OutdoorPartyRuntime::Snapshot &snapshot);
    void setOutdoorPartyState(OutdoorPartyRuntime::Snapshot &&snapshot);

    const std::optional<OutdoorWorldRuntime::Snapshot> &currentOutdoorWorldState() const;
    void setCurrentOutdoorWorldState(const OutdoorWorldRuntime::Snapshot &snapshot);
    void setCurrentOutdoorWorldState(OutdoorWorldRuntime::Snapshot &&snapshot);

    const std::unordered_map<std::string, OutdoorWorldRuntime::Snapshot> &outdoorWorldStates() const;
    std::unordered_map<std::string, OutdoorWorldRuntime::Snapshot> &outdoorWorldStates();

    const std::optional<IndoorSceneRuntime::Snapshot> &currentIndoorSceneState() const;
    void setCurrentIndoorSceneState(const IndoorSceneRuntime::Snapshot &snapshot);
    void setCurrentIndoorSceneState(IndoorSceneRuntime::Snapshot &&snapshot);

    const std::unordered_map<std::string, IndoorSceneRuntime::Snapshot> &indoorSceneStates() const;
    std::unordered_map<std::string, IndoorSceneRuntime::Snapshot> &indoorSceneStates();

    const std::unordered_map<std::string, int32_t> &namedGlobalVars() const;
    int32_t namedGlobalVar(const std::string &name, int32_t defaultValue = 0) const;
    void setNamedGlobalVar(const std::string &name, int32_t value);
    void clearNamedGlobalVar(const std::string &name);
    void mergeNamedGlobalVarsFromRuntime(const EventRuntimeState &runtimeState);
    void applyNamedGlobalVarsToRuntime(EventRuntimeState &runtimeState) const;

    // Camera of the current scene (outdoor or indoor), saved and restored with the game.
    void setCameraAngles(float yawRadians, float pitchRadians);
    float cameraYawRadians() const;
    float cameraPitchRadians() const;
    bool cameraAnglesValid() const;

    const std::optional<std::filesystem::path> &currentSavePath() const;
    void setCurrentSavePath(const std::filesystem::path &path);
    void clearCurrentSavePath();

    const std::optional<EventRuntimeState::PendingMapMove> &pendingMapMove() const;
    void setPendingMapMove(const EventRuntimeState::PendingMapMove &pendingMapMove);
    void setPendingMapMove(EventRuntimeState::PendingMapMove &&pendingMapMove);
    std::optional<EventRuntimeState::PendingMapMove> consumePendingMapMove();
    void clearPendingMapMove();

    void setSaveGameToPathCallback(SaveGameToPathCallback callback);
    bool canSaveGameToPath() const;
    bool saveGameToPath(
        const std::filesystem::path &path,
        const std::string &saveName,
        const std::vector<uint8_t> &previewBmp,
        std::string &error) const;

    void setSettingsChangedCallback(SettingsChangedCallback callback);
    void notifySettingsChanged(const GameSettings &settings) const;

    void requestOpenNewGameScreen();
    void requestOpenLoadGameScreen();
    bool consumePendingOpenNewGameScreenRequest();
    bool consumePendingOpenLoadGameScreenRequest();

    void captureOutdoorRuntimeState(
        const std::string &mapFileName,
        const Party &party,
        const OutdoorPartyRuntime::Snapshot &partySnapshot,
        const OutdoorWorldRuntime::Snapshot &worldSnapshot,
        float yawRadians,
        float pitchRadians
    );

    void captureIndoorRuntimeState(
        const std::string &mapFileName,
        const Party &party,
        const IndoorSceneRuntime::Snapshot &snapshot
    );

    std::optional<GameSaveData> buildSaveData() const;
    void restoreFromSaveData(const GameSaveData &saveData);
    void restoreHeldInventoryItemFromSaveData(const GameSaveData &saveData);

private:
    // The one key a map's stored state goes under: its canonical id, or the file name of a map MapStats does not list.
    std::string mapStateKey(const std::string &mapFileName) const;

    struct GameplayUpdatePerformanceDiagnostics
    {
        uint64_t frames = 0;
        uint64_t activeWorldFrames = 0;
        uint64_t actorAiFrames = 0;
        uint64_t totalNanoseconds = 0;
        uint64_t sharedFrameStateNanoseconds = 0;
        uint64_t worldInteractionStateNanoseconds = 0;
        uint64_t activeMemberSyncNanoseconds = 0;
        uint64_t sharedInputNanoseconds = 0;
        uint64_t worldMovementNanoseconds = 0;
        uint64_t actorAiNanoseconds = 0;
        uint64_t combatEventsNanoseconds = 0;
        uint64_t interactionFrameNanoseconds = 0;
        uint64_t projectileAndCooldownNanoseconds = 0;
        uint64_t performanceTraceLogNanoseconds = 0;

        bool hasActivity() const
        {
            return frames != 0;
        }
    };

    struct TurnBasedFrameTraceState
    {
        bool active = false;
        uint32_t lastLogTick = 0;
        TurnBasedCombatStage stage = TurnBasedCombatStage::None;
        bool actorAiUpdate = false;
        bool gameplayPaused = false;
        bool allowMoveInput = false;
        bool cursorMode = false;
        bool modalBlocked = false;
        bool pendingSpellTarget = false;
        bool sharedWorldBlocked = false;
        int movementActionPoints = 0;
        int pendingActions = 0;
    };

    bool logGameplayUpdatePerformanceDiagnostics(uint32_t currentTick) const;
    void logTurnBasedFrameTraceIfNeeded(
        bool actorAiUpdate,
        bool gameplayPaused,
        bool allowMoveInput,
        bool movementStep,
        bool cursorMode,
        bool modalBlocked,
        bool pendingSpellTarget,
        bool sharedWorldBlocked,
        float movementDeltaSeconds,
        float frameDeltaSeconds);
    void updateMm9QuestMarkerCache();

    const GameDataRepository *m_pDataRepository = nullptr;
    std::optional<std::filesystem::path> m_madnessLedgerFile;
    std::optional<Party> m_partyState;
    SceneKind m_currentSceneKind = SceneKind::Outdoor;
    std::string m_currentMapFileName;
    GameplayScreenState m_gameplayScreenState;
    GameplayUiController m_gameplayUiController;
    GameplayUiRuntime m_gameplayUiRuntime;
    GameplayActorService m_gameplayActorService;
    bool m_tavernHirelings = true;
    WorldHumDefinition m_worldHum;
    WorldFloorBurnDefinition m_worldFloorBurn;
    std::vector<uint32_t> m_worldObjectiveQBits;
    GameplayCombatController m_gameplayCombatController;
    GameplayItemService m_gameplayItemService;
    GameplayProjectileService m_gameplayProjectileService;
    GameplayFxService m_gameplayFxService;
    GameplaySpellService m_gameplaySpellService;
    TurnBasedCombatRuntime m_turnBasedCombatRuntime;
    size_t m_turnBasedPendingWorldActions = 0;
    GameplayScreenRuntime m_gameplayScreenRuntime;
    GameplayDialogController m_gameplayDialogController;
    GameplayOverlayInteractionState m_overlayInteractionState;
    std::array<uint8_t, SDL_SCANCODE_COUNT> m_previousKeyboardState = {};
    IGameplayWorldRuntime *m_pActiveWorldRuntime = nullptr;
    Mm9LoadedLocationQuestMarkerCache m_mm9QuestMarkerCache;
    uint64_t m_mm9QuestMarkerMapRevision = 1;
    const GameplayInputFrame *m_pCurrentGameplayInputFrame = nullptr;
    GameplaySharedInputFrameResult m_sharedInputFrameResult = {};
    bool m_sharedWorldInteractionBlockedThisFrame = false;
    bool m_relativeMouseMotionResetRequested = false;
    std::optional<OutdoorPartyRuntime::Snapshot> m_outdoorPartyState;
    std::optional<OutdoorWorldRuntime::Snapshot> m_currentOutdoorWorldState;
    std::unordered_map<std::string, OutdoorWorldRuntime::Snapshot> m_outdoorWorldStates;
    std::optional<IndoorSceneRuntime::Snapshot> m_currentIndoorSceneState;
    std::unordered_map<std::string, IndoorSceneRuntime::Snapshot> m_indoorSceneStates;
    std::unordered_map<std::string, int32_t> m_namedGlobalVars;
    float m_gameMinutes = 9.0f * 60.0f;
    RulesetSelection m_ruleset;
    std::unique_ptr<MawRules> m_pMawRules;
    uint64_t m_randomRootSeed = 0;
    float m_cameraYawRadians = 0.0f;
    float m_cameraPitchRadians = 0.0f;
    bool m_cameraAnglesValid = false;
    std::optional<std::filesystem::path> m_currentSavePath;
    std::optional<EventRuntimeState::PendingMapMove> m_pendingMapMove;
    SaveGameToPathCallback m_saveGameToPathCallback;
    SettingsChangedCallback m_settingsChangedCallback;
    mutable GameplayUpdatePerformanceDiagnostics m_gameplayUpdatePerformanceDiagnostics;
    mutable uint32_t m_lastGameplayUpdatePerformanceLogTick = 0;
    bool m_framePerformanceDiagnosticsEnabled = false;
    GameplayUpdateFramePerformanceDiagnostics m_lastGameplayUpdateFramePerformanceDiagnostics = {};
    GameplayUiFramePerformanceDiagnostics m_lastGameplayUiFramePerformanceDiagnostics = {};
    TurnBasedFrameTraceState m_turnBasedFrameTraceState;
};
}
