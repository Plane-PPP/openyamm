#pragma once

#include "game/data/GameDataRepository.h"
#include "game/ui/MenuDesignScreen.h"
#include "game/ui/UiLayoutManager.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace OpenYAMM::Game
{
// What saving costs in food here (MAW from Nightmare): now, with enemies near, and what the party carries.
struct SaveFoodCost
{
    int food = 0;
    int foodWithEnemiesNear = 0;
    int carried = 0;
};

class LoadGameScreen : public MenuDesignScreen
{
  public:
    using LoadAction = std::function<bool(const std::filesystem::path &)>;
    using CancelAction = std::function<void()>;
    using SaveAction = std::function<bool(const std::filesystem::path &, const std::string &)>;

    // loadWarning: what loading from play costs now, said in the confirmation (MAW's pending Beyond Madness death).
    LoadGameScreen(const Engine::AssetFileSystem &assetFileSystem, const GameDataRepository &gameData,
                   LoadAction loadAction, CancelAction cancelAction, bool fromGameplay = false,
                   GameAudioSystem *pAudio = nullptr, std::string loadWarning = {});

    AppMode mode() const override;
    void prepareForFirstFrame();
    void onEnter() override;
    static void invalidateCachedSaveSlots();
    static std::optional<std::filesystem::path> mostRecentSave(std::string &mapFileName);
    void configureSave(SaveAction action, const std::string &location, float gameMinutes,
                       const std::vector<uint8_t> &previewBmp, SaveFoodCost foodCost = {});
    void handleSdlEvent(const SDL_Event &event) override;

  private:
    struct SaveSlotSummary
    {
        std::filesystem::path path;
        std::string fileLabel;
        std::string locationName;
        std::string weekdayClockText;
        std::string dateText;
        std::string worldName;
        std::string rulesetId;
        // "MAW · Hell difficulty · Horizontal progression"; empty for the Standard ruleset.
        std::string rulesetBadge;
        std::string savedAt;
        bool populated = false;
        int previewWidth = 0;
        int previewHeight = 0;
        std::vector<uint8_t> previewPixelsBgra;
    };

    struct RulesetFilter
    {
        std::string id;
        std::string name;
    };

    void drawScreen(float deltaSeconds) override;
    // The filter buttons, and whether the filter changed.
    bool drawRulesetFilters(const std::string &prefix);
    void refreshRulesetFilters();
    // The slots the ruleset filter lets through, as indices into m_slots.
    std::vector<size_t> shownSlots() const;
    // The cached list while the saves folder is unchanged, else a fresh read.
    void loadSaveSlots();
    void refreshSaveSlots();
    bool tryLoadSelectedSlot();
    void cancel();
    void loadCachedSaveSlots();

    const GameDataRepository *m_pGameData = nullptr;
    LoadAction m_loadAction;
    CancelAction m_cancelAction;
    size_t m_selectedIndex = 0;
    float m_scrollOffset = 0;
    bool m_revealSelection = false;
    uint64_t m_lastClickedSlotTicks = 0;
    size_t m_lastClickedSlotIndex = static_cast<size_t>(-1);
    std::vector<SaveSlotSummary> m_slots;
    // The rulesets the saves were played with, Standard first; the filter is a ruleset id, empty for all.
    std::vector<RulesetFilter> m_rulesetFilters;
    std::string m_rulesetFilter;
    size_t m_newerSaveCount = 0;
    bool m_slotsLoaded = false;
    uint64_t m_slotsSignature = 0;
    SaveAction m_saveAction;
    std::string m_saveName;
    std::string m_currentLocation;
    std::string m_currentDate;
    std::string m_currentClock;
    std::string m_error;
    SaveFoodCost m_foodCost;
    bool m_newSave = true;
    bool m_nameEditing = false;
    bool m_commitRequested = false;
    bool m_fromGameplay = false;
    std::string m_loadWarning;
    int m_previewWidth = 0;
    int m_previewHeight = 0;
    std::vector<uint8_t> m_previewPixels;
    void commitSave();
    void requestLoad();
    void requestDelete();
    void requestImportSaves();
    void finishImportSaves();

    // The folder picker may answer on another thread; the screen polls it while drawing.
    struct FolderPick
    {
        std::mutex mutex;
        bool finished = false;
        std::optional<std::filesystem::path> folder;
    };
    std::shared_ptr<FolderPick> m_folderPick;

    static std::vector<SaveSlotSummary> s_cachedSlots;
    static bool s_cachedSlotsValid;
    static uint64_t s_cachedSignature;
    static size_t s_cachedNewerSaveCount;
};
} // namespace OpenYAMM::Game
