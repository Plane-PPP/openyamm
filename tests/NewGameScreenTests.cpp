#include "engine/AssetFileSystem.h"
#include "game/data/GameDataLoader.h"
#include "game/ui/screens/NewGameScreen.h"

#include <doctest/doctest.h>

#include <algorithm>
#include <filesystem>

namespace OpenYAMM::Game
{
struct NewGameScreenTestAccess
{
    static void checkAllContinents(NewGameScreen &screen, const GameDataRepository &data,
                                   std::vector<Character> &createdParty, uint32_t &startingContinentId)
    {
        std::vector<uint32_t> expectedCharacterIds;
        for (const auto &[id, entry] : data.characterDollTable().characters())
        {
            if (entry.availableAtStart)
            {
                expectedCharacterIds.push_back(id);
            }
        }
        std::sort(expectedCharacterIds.begin(), expectedCharacterIds.end());
        const std::vector<uint32_t> dragonClassIds = {10};
        const std::vector<uint32_t> expectedDragonIds = {25, 26, 72, 75};

        for (const MergedCharacterSelectionContinent &continent : data.mergedCharacterSelectionTable().continents())
        {
            CAPTURE(continent.key);
            screen.selectContinent(continent.key);
            const uint32_t clericClassId = continent.key == "jadame" ? 5 : 4;
            const uint32_t mageClassId = continent.key == "enroth" || continent.key == "antagarich" ? 42 : 44;
            const std::vector<uint32_t> humanClassIds = {0, clericClassId, 12, 16, 22, 26, 30, 34, mageClassId};
            std::vector<uint32_t> characterIds;
            std::vector<size_t> dragonIndices;
            bool foundHuman = false;
            for (size_t index = 0; index < screen.m_candidates.size(); ++index)
            {
                const NewGameScreen::CreationCandidate &candidate = screen.m_candidates[index];
                CHECK(std::find(candidate.availableClassIds.begin(), candidate.availableClassIds.end(),
                                clericClassId == 4 ? 5u : 4u)
                      == candidate.availableClassIds.end());
                CHECK(std::find(candidate.availableClassIds.begin(), candidate.availableClassIds.end(),
                                mageClassId == 42 ? 44u : 42u)
                      == candidate.availableClassIds.end());
                REQUIRE(std::find(candidate.availableClassIds.begin(), candidate.availableClassIds.end(),
                                  candidate.classId) != candidate.availableClassIds.end());
                const CharacterDollEntry *entry = data.characterDollTable().getCharacter(candidate.characterDataId);
                REQUIRE(entry != nullptr);
                if (entry->defaultClassId == 4 || entry->defaultClassId == 5)
                {
                    CHECK_EQ(candidate.classId, clericClassId);
                }
                if (entry->defaultClassId == 42 || entry->defaultClassId == 44)
                {
                    CHECK_EQ(candidate.classId, mageClassId);
                }
                characterIds.push_back(candidate.characterDataId);
                if (candidate.raceName == "Human")
                {
                    CHECK(candidate.availableClassIds == humanClassIds);
                    foundHuman = true;
                }
                if (candidate.raceName == "Dragon")
                {
                    CHECK(candidate.availableClassIds == dragonClassIds);
                    dragonIndices.push_back(index);
                }
            }
            CHECK(std::search(characterIds.begin(), characterIds.end(),
                              expectedDragonIds.begin(), expectedDragonIds.end()) != characterIds.end());
            std::sort(characterIds.begin(), characterIds.end());
            CHECK(characterIds == expectedCharacterIds);
            CHECK(foundHuman);
            REQUIRE_EQ(dragonIndices.size(), expectedDragonIds.size());

            const auto human = std::find_if(screen.m_candidates.begin(), screen.m_candidates.end(),
                [](const NewGameScreen::CreationCandidate &candidate) { return candidate.raceName == "Human"; });
            REQUIRE(human != screen.m_candidates.end());
            const size_t humanIndex = static_cast<size_t>(human - screen.m_candidates.begin());
            for (uint32_t classId : {clericClassId, mageClassId})
            {
                screen.resetStateForCandidate(humanIndex);
                screen.m_state.selectedClassId = classId;
                screen.refreshSkillChoices(false);
                createdParty.clear();
                screen.confirmCreation();
                REQUIRE_EQ(createdParty.size(), 1);
                CHECK_EQ(createdParty.front().className,
                         classId == clericClassId ? (clericClassId == 5 ? "Priest" : "Cleric")
                                                 : (mageClassId == 44 ? "Necromancer" : "Sorcerer"));
                CHECK_EQ(startingContinentId, continent.id);
                CHECK(createdParty.front().hasSkill(classId == clericClassId ? "BodyMagic"
                                                       : mageClassId == 44 ? "DarkMagic" : "FireMagic"));
            }

            // A Dark Elf by race of another class gets the race's ability as a fixed skill, not one of the two choices.
            const auto darkElf = std::find_if(screen.m_candidates.begin(), screen.m_candidates.end(),
                [](const NewGameScreen::CreationCandidate &candidate) { return candidate.raceName == "DarkElf"; });
            REQUIRE(darkElf != screen.m_candidates.end());
            REQUIRE(std::find(darkElf->availableClassIds.begin(), darkElf->availableClassIds.end(), 0u)
                    != darkElf->availableClassIds.end());
            screen.resetStateForCandidate(static_cast<size_t>(darkElf - screen.m_candidates.begin()));
            screen.m_state.selectedClassId = 0;
            screen.refreshSkillChoices(false);
            const std::vector<std::string> &fixedSkills = screen.m_state.defaultSkills;
            const std::vector<std::string> &choices = screen.m_state.optionalSkills;
            CHECK(std::find(fixedSkills.begin(), fixedSkills.end(), "DarkElfAbility") != fixedSkills.end());
            CHECK(std::find(choices.begin(), choices.end(), "DarkElfAbility") == choices.end());
            createdParty.clear();
            screen.confirmCreation();
            REQUIRE_EQ(createdParty.size(), 1);
            CHECK_EQ(createdParty.front().className, "Archer");
            CHECK(createdParty.front().hasSkill("DarkElfAbility"));
            CHECK(createdParty.front().hasSkill("Bow"));
            CHECK(createdParty.front().hasSkill("AirMagic"));

            for (size_t dragonIndex : dragonIndices)
            {
                createdParty.clear();
                startingContinentId = 0;
                screen.resetStateForCandidate(dragonIndex);
                screen.confirmCreation();
                REQUIRE_EQ(createdParty.size(), 1);
                CHECK_EQ(createdParty.front().characterDataId, screen.m_candidates[dragonIndex].characterDataId);
                CHECK_EQ(createdParty.front().raceId, 5);
                CHECK_EQ(createdParty.front().className, "Dragon");
                CHECK_EQ(startingContinentId, continent.id);
            }
        }
    }

    static void checkInnateLethalStrike(NewGameScreen &screen)
    {
        screen.selectContinent("jadame");
        size_t checked = 0;
        for (size_t i = 0; i < screen.m_candidates.size(); ++i)
        {
            screen.resetStateForCandidate(i);
            if (screen.classNameForState(screen.m_state) != "Minotaur") continue;
            const size_t oldSkillCount = screen.m_state.defaultSkills.size()
                + screen.m_state.selectedOptionalSkills.size();
            const Character c = screen.buildCharacter();
            REQUIRE(c.hasSkill("LethalStrike"));
            CHECK(c.findSkill("LethalStrike")->level == 1);
            CHECK(c.findSkill("LethalStrike")->mastery == SkillMastery::Normal);
            CHECK(c.skills.size() == oldSkillCount + 1);
            CHECK(c.skillPoints == 0);
            ++checked;
        }
        CHECK(checked > 0);
    }
};
}

TEST_CASE("party creation offers all starting characters with continent-specific caster classes")
{
    using namespace OpenYAMM::Game;
    const std::filesystem::path sourceRoot = OPENYAMM_SOURCE_DIR;
    OpenYAMM::Engine::AssetFileSystem assets;
    REQUIRE(assets.initialize(sourceRoot, sourceRoot / "assets_dev", OpenYAMM::Engine::AssetScaleTier::X1, "mm6"));
    GameDataLoader loader;
    REQUIRE(loader.loadForHeadlessGameplay(assets));
    GameDataRepository data;
    data.bind(loader);
    std::vector<Character> createdParty;
    uint32_t startingContinentId = 0;
    NewGameScreen screen(assets, nullptr, data, false, false, true,
        [&](const std::vector<Character> &characters, const NewGameOptions &options)
        {
            createdParty = characters;
            startingContinentId = options.continentId;
            CHECK_FALSE(options.preserveDebugLoadout);
            CHECK(options.ruleset.isStandard());
        }, {}, {});
    NewGameScreenTestAccess::checkAllContinents(screen, data, createdParty, startingContinentId);
}


TEST_CASE("Lethal Strike new game creation adds innate skill beyond chosen skills")
{
    using namespace OpenYAMM::Game;
    const std::filesystem::path root = OPENYAMM_SOURCE_DIR;
    OpenYAMM::Engine::AssetFileSystem assets;
    REQUIRE(assets.initialize(root, root / "assets_dev", OpenYAMM::Engine::AssetScaleTier::X1));
    GameDataLoader loader;
    REQUIRE(loader.loadCommonForGameplay(assets));
    GameDataRepository data;
    data.bind(loader);
    NewGameScreen screen(assets, nullptr, data, false, false, true, {}, {});
    NewGameScreenTestAccess::checkInnateLethalStrike(screen);
}
