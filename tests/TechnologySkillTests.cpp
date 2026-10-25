#include "doctest/doctest.h"

#include "engine/AssetFileSystem.h"
#include "engine/TextTable.h"
#include "game/events/EventRuntime.h"
#include "game/events/EvtEnums.h"
#include "game/events/ScriptedEventProgram.h"
#include "game/gameplay/GameMechanics.h"
#include "game/gameplay/HouseInteraction.h"
#include "game/gameplay/MasteryTeacherDialog.h"
#include "game/items/InventoryItemUseRuntime.h"
#include "game/maps/SaveGame.h"
#include "game/party/Party.h"
#include "game/party/SkillData.h"
#include "tests/RegressionGameData.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace
{
using namespace OpenYAMM::Game;

const OpenYAMM::Tests::RegressionGameData &gameData()
{
    REQUIRE_MESSAGE(
        OpenYAMM::Tests::regressionGameDataLoaded(),
        OpenYAMM::Tests::regressionGameDataFailure().c_str());
    return OpenYAMM::Tests::regressionGameData();
}

std::string sourceFileText(const std::string &relativePath)
{
    std::ifstream file(std::filesystem::path(OPENYAMM_SOURCE_DIR) / relativePath);
    REQUIRE(file.good());
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

Character makeMember(const std::string &name, const std::string &className)
{
    Character member = {};
    member.name = name;
    member.className = className;
    member.role = className;
    member.level = 5;
    member.might = 15;
    member.intellect = 15;
    member.personality = 15;
    member.endurance = 15;
    member.speed = 15;
    member.accuracy = 15;
    member.luck = 15;
    member.maxHealth = 50;
    member.health = 50;
    return member;
}

Party makeParty(const std::vector<Character> &members, int gold)
{
    Party party = {};
    party.setClassSkillTable(&gameData().classSkillTable);
    PartySeed seed = {};
    seed.gold = gold;
    seed.food = 7;
    seed.members = members;
    party.seed(seed);
    return party;
}

std::vector<std::vector<std::string>> rowsFromText(const std::string &text)
{
    const std::optional<OpenYAMM::Engine::TextTable> table = OpenYAMM::Engine::TextTable::parseTabSeparated(text);
    REQUIRE(table.has_value());
    std::vector<std::vector<std::string>> rows;

    for (size_t rowIndex = 0; rowIndex < table->getRowCount(); ++rowIndex)
    {
        rows.push_back(table->getRow(rowIndex));
    }

    return rows;
}

// The engine's items plus three of Hesper's draft rows (design/kreegan_world/data/items.txt): the Pulse Pistol, the
// Plasma Saber and the Engram Shard.
const ItemTable &itemTableWithHesperItems()
{
    static const ItemTable table = []()
    {
        OpenYAMM::Engine::AssetFileSystem assetFileSystem;
        const std::filesystem::path sourceRoot = OPENYAMM_SOURCE_DIR;
        REQUIRE(assetFileSystem.initialize(sourceRoot, sourceRoot / "assets_dev", OpenYAMM::Engine::AssetScaleTier::X1));
        std::vector<std::vector<std::string>> itemRows =
            rowsFromText(*assetFileSystem.readTextFile("engine/data_tables/items.txt"));
        const std::vector<std::vector<std::string>> randomItemRows =
            rowsFromText(*assetFileSystem.readTextFile("engine/data_tables/random_items.txt"));
        const std::vector<std::vector<std::string>> hesperRows = rowsFromText(
            "11000\thes_pulse_pistol\tPulse Pistol\t10000\tWeapon\tEnergy Guns\t4d6\t10\t4\t15\tKreegan Sidearm\t0\t0"
            "\t0\t0\t0\tA pistol.\thesper\thesper:item/pulse_pistol\t\tKreeganEnergy|EnergyWeapon|Fast\n"
            "11022\thes_plasma_saber\tPlasma Saber\t22000\tWeapon\tSword\t4d10\t12\t5\t25\tKreegan Sword\t0\t0"
            "\t0\t0\t0\tA saber.\thesper\thesper:item/plasma_saber\t\tKreeganEnergy|EnergyWeapon|EnergyBlade\n"
            "11330\thes_engram_shard\tEngram Shard\t0\tN / A\tMisc\t0\t0\t4\t0\tEngram\t0\t2\t0\t0\t0\tTech points."
            "\thesper\thesper:item/engram_shard\t\t\t4\tgrantTechPoints\tcharacter\n");
        itemRows.insert(itemRows.end(), hesperRows.begin(), hesperRows.end());
        ItemTable loaded = {};
        REQUIRE(loaded.load(assetFileSystem, itemRows, randomItemRows));
        return loaded;
    }();
    return table;
}

std::pair<int, int> damageRange(const std::string &text)
{
    std::istringstream stream(text);
    int minimum = 0;
    int maximum = 0;
    char dash = 0;
    stream >> minimum >> dash >> maximum;
    return {minimum, maximum};
}

MergedTeacherTopicTable teacherTopicsWithHesperRows()
{
    MergedTeacherTopicTable table = {};
    REQUIRE(table.loadFromRows({
        {"TopicId", "Notes", "SkillId", "Mastery", "TextId", "Req. Gold", "Req. Skill"},
        {"303", "Expert Sword", "1", "1", "303", "", ""},
        {"9201", "Expert Energy Guns", "EnergyGuns", "1", "9201", "15000", ""},
        {"9208", "Master Hacking", "Hacking", "2", "9208", "", ""},
    }));
    return table;
}
}

TEST_CASE("technology skills are appended after the classic skill ids")
{
    const std::vector<std::string> classicSkills = {
        "Staff", "Sword", "Dagger", "Axe", "Spear", "Bow", "Mace", "Blaster", "Shield", "LeatherArmor", "ChainArmor",
        "PlateArmor", "FireMagic", "AirMagic", "WaterMagic", "EarthMagic", "SpiritMagic", "MindMagic", "BodyMagic",
        "LightMagic", "DarkMagic", "DarkElfAbility", "VampireAbility", "DragonAbility", "IdentifyItem", "Merchant",
        "RepairItem", "Bodybuilding", "Meditation", "Perception", "Regeneration", "DisarmTraps", "Dodging", "Unarmed",
        "IdentifyMonster", "Armsmaster", "Stealing", "Alchemy", "Learning", "Throwing"};
    const std::vector<std::string> allSkills = allCanonicalSkillNames();

    REQUIRE_EQ(allSkills.size(), 48u);
    CHECK(std::equal(classicSkills.begin(), classicSkills.end(), allSkills.begin()));
    CHECK_EQ(FirstTechnologySkillId, 40u);
    CHECK_EQ(skillNameForSkillId(40).value_or(""), "EnergyGuns");
    CHECK_EQ(skillNameForSkillId(41).value_or(""), "EnergyBlades");
    CHECK_EQ(skillNameForSkillId(42).value_or(""), "ShieldTech");
    CHECK_EQ(skillNameForSkillId(43).value_or(""), "Hacking");
    CHECK_EQ(skillNameForSkillId(44).value_or(""), "Cover");
    CHECK_EQ(skillNameForSkillId(47).value_or(""), "Retaliation");
    CHECK_FALSE(skillNameForSkillId(48).has_value());
    CHECK_FALSE(isTechnologySkill("Cover"));
    CHECK_EQ(displaySkillName("ManaShield"), "Mana Shield");
    CHECK_EQ(skillIdForSkillName("Energy Guns").value_or(0), 40u);
    CHECK_EQ(skillIdForSkillName("Learning").value_or(0), 38u);
    CHECK_EQ(canonicalSkillName("Shield Tech"), "ShieldTech");
    CHECK_EQ(displaySkillName("EnergyBlades"), "Energy Blades");
    CHECK_EQ(displaySkillName("Hacking"), "Hacking");
    CHECK(isTechnologySkill("Hacking"));
    CHECK_FALSE(isTechnologySkill("Blaster"));
    CHECK_FALSE(isTechnologySkill("Throwing"));
}

TEST_CASE("technology skills have no class caps and never start on a character")
{
    const ClassSkillTable &table = gameData().classSkillTable;

    for (const char *pClassName : {"Knight", "Sorcerer", "Lich", "Peasant", "Dragon", "Monk"})
    {
        for (const std::string &skillName : skillSheetGroupSkillNames(SkillSheetGroup::Technology))
        {
            CHECK_EQ(table.getEffectiveCap(pClassName, 0, skillName), SkillMastery::Grandmaster);
            CHECK_EQ(
                table.getEffectiveStartingSkillAvailability(pClassName, 0, skillName),
                StartingSkillAvailability::None);
        }

        for (const CharacterSkill &skill : table.getDefaultSkillsForCharacter(pClassName, 0))
        {
            CHECK_FALSE(isTechnologySkill(skill.name));
        }
    }

    // Classic caps still come from class_skills.txt.
    CHECK_EQ(table.getClassCap("Sorcerer", "Sword"), SkillMastery::None);
    CHECK_EQ(table.getClassCap("Knight", "FireMagic"), SkillMastery::None);
}

TEST_CASE("the sheet's skill groups are unchanged and Technology shows only with a technology skill")
{
    CHECK_EQ(skillSheetGroupSkillNames(SkillSheetGroup::Weapons),
        std::vector<std::string>{"Axe", "Bow", "Throwing", "Dagger", "Mace", "Spear", "Staff", "Sword", "Unarmed",
            "Blaster"});
    CHECK_EQ(skillSheetGroupSkillNames(SkillSheetGroup::Armor),
        std::vector<std::string>{"LeatherArmor", "ChainArmor", "PlateArmor", "Shield", "Dodging", "Cover", "Retaliation",
            "ManaShield"});
    CHECK_EQ(skillSheetGroupSkillNames(SkillSheetGroup::Misc).size(), 12u);
    CHECK_EQ(skillSheetGroupSkillNames(SkillSheetGroup::Magic).size(), 9u);
    CHECK_EQ(skillSheetGroupSkillNames(SkillSheetGroup::Technology),
        std::vector<std::string>{"EnergyGuns", "EnergyBlades", "ShieldTech", "Hacking"});

    Character character = makeMember("Ada", "Knight");
    character.skills["Sword"] = {"Sword", 10, SkillMastery::Grandmaster};
    character.skills["Blaster"] = {"Blaster", 10, SkillMastery::Grandmaster};
    CHECK_FALSE(character.hasTechnologySkill());
    character.skills["Hacking"] = {"Hacking", 1, SkillMastery::Normal};
    CHECK(character.hasTechnologySkill());
}

TEST_CASE("teacher topics name technology skills by name and keep classic skill ids")
{
    const MergedTeacherTopicTable table = teacherTopicsWithHesperRows();

    const std::optional<MasteryTeacherTopicDefinition> sword = resolveMasteryTeacherTopic(303, &table);
    REQUIRE(sword.has_value());
    CHECK_EQ(sword->skillName, "Sword");
    CHECK_EQ(sword->targetMastery, SkillMastery::Expert);

    const std::optional<MasteryTeacherTopicDefinition> guns = resolveMasteryTeacherTopic(9201, &table);
    REQUIRE(guns.has_value());
    CHECK_EQ(guns->skillId, 40u);
    CHECK_EQ(guns->skillName, "EnergyGuns");
    CHECK_EQ(guns->targetMastery, SkillMastery::Expert);
    CHECK_EQ(guns->requiredGold, 15000u);

    const std::optional<MasteryTeacherTopicDefinition> hacking = resolveMasteryTeacherTopic(9208, &table);
    REQUIRE(hacking.has_value());
    CHECK_EQ(hacking->skillName, "Hacking");
    CHECK_EQ(hacking->targetMastery, SkillMastery::Master);

    // The engine's own teacher table is unchanged.
    const std::optional<MasteryTeacherTopicDefinition> engineSword =
        resolveMasteryTeacherTopic(303, &gameData().mergedTeacherTopicTable);
    REQUIRE(engineSword.has_value());
    CHECK_EQ(engineSword->skillName, "Sword");
}

TEST_CASE("a technology teacher promotes any class at the rank, for the topic's gold or the Hesper price")
{
    const MergedTeacherTopicTable table = teacherTopicsWithHesperRows();
    Character sorcerer = makeMember("Bram", "Sorcerer");
    sorcerer.skills["EnergyGuns"] = {"EnergyGuns", 3, SkillMastery::Normal};
    sorcerer.skills["Hacking"] = {"Hacking", 7, SkillMastery::Expert};
    Party party = makeParty({sorcerer}, 60000);

    std::optional<MasteryTeacherEvaluation> evaluation =
        evaluateMasteryTeacherTopic(9201, party, gameData().classSkillTable, gameData().npcDialogTable, &table);
    REQUIRE(evaluation.has_value());
    CHECK_FALSE(evaluation->approved);

    party.member(0)->skills["EnergyGuns"].level = 4;
    evaluation =
        evaluateMasteryTeacherTopic(9201, party, gameData().classSkillTable, gameData().npcDialogTable, &table);
    REQUIRE(evaluation.has_value());
    CHECK(evaluation->approved);
    CHECK_EQ(evaluation->cost, 15000);
    CHECK_EQ(evaluation->displayText, "Become Expert in Energy Guns for 15000 gold");

    // No Req. Gold: the Hesper Master price.
    evaluation =
        evaluateMasteryTeacherTopic(9208, party, gameData().classSkillTable, gameData().npcDialogTable, &table);
    REQUIRE(evaluation.has_value());
    CHECK(evaluation->approved);
    CHECK_EQ(evaluation->cost, 40000);
}

TEST_CASE("a house that lists offered skills sells their Novice; plain MM houses are unchanged")
{
    const ClassSkillTable &classSkillTable = gameData().classSkillTable;
    Party party = makeParty({makeMember("Ada", "Knight")}, 5000);

    HouseEntry salvage = {};
    salvage.id = 9201;
    salvage.type = "House";
    salvage.name = "Salvage & Arms";
    salvage.priceMultiplier = 1.0f;
    salvage.skillPriceMultiplier = 2.0f;
    salvage.offeredSkills = {"EnergyGuns", "EnergyBlades"};

    std::vector<HouseActionOption> options =
        buildHouseActionOptions(salvage, &party, &classSkillTable, nullptr, 0.0f, DialogueMenuId::None);
    REQUIRE_EQ(options.size(), 1u);
    CHECK_EQ(options.front().id, HouseActionId::OpenLearnSkillsMenu);

    options = buildHouseActionOptions(salvage, &party, &classSkillTable, nullptr, 0.0f, DialogueMenuId::LearnSkills);
    REQUIRE_EQ(options.size(), 2u);
    CHECK_EQ(options[0].label, "Learn Energy Blades for 1000 gold");
    CHECK_EQ(options[1].label, "Learn Energy Guns for 1000 gold");

    const HouseActionResult result =
        performHouseAction(options[1], salvage, party, &classSkillTable, nullptr, &gameData().npcDialogTable);
    CHECK(result.succeeded);
    CHECK_EQ(party.gold(), 4000);
    const CharacterSkill *pSkill = party.member(0)->findSkill("EnergyGuns");
    REQUIRE(pSkill != nullptr);
    CHECK_EQ(pSkill->level, 1u);
    CHECK_EQ(pSkill->mastery, SkillMastery::Normal);

    HouseEntry plainHouse = salvage;
    plainHouse.offeredSkills.clear();
    CHECK(buildHouseActionOptions(plainHouse, &party, &classSkillTable, nullptr, 0.0f, DialogueMenuId::None).empty());
}

TEST_CASE("tech points raise technology skills, skill points raise classic skills")
{
    Character member = makeMember("Ada", "Knight");
    member.skills["Sword"] = {"Sword", 1, SkillMastery::Normal};
    member.skills["EnergyGuns"] = {"EnergyGuns", 1, SkillMastery::Normal};
    member.skillPoints = 10;
    member.techPoints = 0;
    Party party = makeParty({member}, 0);

    CHECK(party.canIncreaseActiveMemberSkillLevel("Sword"));
    CHECK_FALSE(party.canIncreaseActiveMemberSkillLevel("EnergyGuns"));

    party.member(0)->techPoints = 2;
    REQUIRE(party.increaseActiveMemberSkillLevel("EnergyGuns"));
    CHECK_EQ(party.member(0)->findSkill("EnergyGuns")->level, 2u);
    CHECK_EQ(party.member(0)->techPoints, 0u);
    CHECK_EQ(party.member(0)->skillPoints, 10u);

    REQUIRE(party.increaseActiveMemberSkillLevel("Sword"));
    CHECK_EQ(party.member(0)->skillPoints, 8u);
    CHECK_EQ(party.member(0)->techPoints, 0u);
}

TEST_CASE("an Engram gives its VarA in tech points")
{
    const ItemTable &itemTable = itemTableWithHesperItems();
    Party party = makeParty({makeMember("Ada", "Knight")}, 0);
    InventoryItem engram = {};
    engram.objectDescriptionId = 11330;
    engram.identified = true;

    CHECK_EQ(InventoryItemUseRuntime::classifyItemUse(engram, itemTable), InventoryItemUseAction::GrantTechPoints);
    const InventoryItemUseResult result =
        InventoryItemUseRuntime::useItemOnMember(party, 0, engram, itemTable, nullptr);
    CHECK(result.handled);
    CHECK(result.consumed);
    CHECK_EQ(result.statusText, "+2 Tech Points!");
    CHECK_EQ(party.member(0)->techPoints, 2u);
    CHECK_EQ(party.member(0)->skillPoints, 0u);
}

TEST_CASE("save round trips keep technology skills and tech points, and leave other characters unchanged")
{
    Character hesperMember = makeMember("Ada", "Knight");
    hesperMember.skills["Sword"] = {"Sword", 10, SkillMastery::Grandmaster};
    hesperMember.skills["EnergyBlades"] = {"EnergyBlades", 4, SkillMastery::Expert};
    hesperMember.skills["Hacking"] = {"Hacking", 2, SkillMastery::Normal};
    hesperMember.skillPoints = 3;
    hesperMember.techPoints = 7;
    Character homeMember = makeMember("Bram", "Sorcerer");
    homeMember.skills["FireMagic"] = {"FireMagic", 5, SkillMastery::Expert};
    homeMember.skillPoints = 12;
    Party party = makeParty({hesperMember, homeMember}, 100);

    GameSaveData saveData = {};
    saveData.mapFileName = "tech_skills_roundtrip.odm";
    saveData.party = party.snapshot();
    const std::filesystem::path savePath =
        std::filesystem::temp_directory_path() / "openyamm_technology_skill_roundtrip.oysav";
    std::string error;
    REQUIRE(saveGameDataToPath(savePath, saveData, error));
    const std::optional<GameSaveData> loaded = loadGameDataFromPath(savePath, error);
    std::filesystem::remove(savePath);
    REQUIRE_MESSAGE(loaded.has_value(), error.c_str());
    REQUIRE_EQ(loaded->party.members.size(), 2u);

    for (size_t memberIndex = 0; memberIndex < 2; ++memberIndex)
    {
        const Character &before = saveData.party.members[memberIndex];
        const Character &after = loaded->party.members[memberIndex];
        CHECK_EQ(after.skillPoints, before.skillPoints);
        CHECK_EQ(after.techPoints, before.techPoints);
        REQUIRE_EQ(after.skills.size(), before.skills.size());

        for (const auto &[name, skill] : before.skills)
        {
            const CharacterSkill *pSkill = after.findSkill(name);
            REQUIRE(pSkill != nullptr);
            CHECK_EQ(pSkill->level, skill.level);
            CHECK_EQ(pSkill->mastery, skill.mastery);
        }
    }

    CHECK_EQ(loaded->party.members[0].techPoints, 7u);
    CHECK_EQ(loaded->party.members[1].techPoints, 0u);
    CHECK_FALSE(loaded->party.members[1].hasTechnologySkill());
}

TEST_CASE("technology skill event variables and Hacking checks in Lua events")
{
    Character member = makeMember("Ada", "Knight");
    Party party = makeParty({member, makeMember("Bram", "Sorcerer")}, 0);
    EventRuntimeState runtimeState = {};
    const uint32_t hackingVariable = static_cast<uint32_t>(EvtVariable::HackingSkill);

    CHECK_EQ(EventRuntime::getVariableValue(runtimeState, EventRuntime::decodeVariable(hackingVariable), &party, 0), 0);
    EventRuntime::setVariableValue(runtimeState, EventRuntime::decodeVariable(hackingVariable), 1, &party, {1});
    const CharacterSkill *pHacking = party.member(1)->findSkill("Hacking");
    REQUIRE(pHacking != nullptr);
    CHECK_EQ(pHacking->level, 1u);
    CHECK_EQ(pHacking->mastery, SkillMastery::Normal);
    CHECK(Party::isPartyWideUtilitySkill("Hacking"));

    const std::string source = sourceFileText("assets_dev/engine/scripts/common/event_support.lua") + "\n"
        "RegisterGlobalEvent(9001, \"Terminal\", function()\n"
        "    if evt.CheckSkill(const.Skills.Hacking, const.SkillCheck.Expert, 4) then\n"
        "        evt.StatusText(\"open\")\n"
        "    elseif IsAtLeast(HackingSkill, 1) then\n"
        "        evt.StatusText(\"novice\")\n"
        "    else\n"
        "        evt.StatusText(\"locked\")\n"
        "    end\n"
        "end)\n";
    std::string error;
    const std::optional<ScriptedEventProgram> program =
        ScriptedEventProgram::loadFromLuaText(source, "@tech_skill_test.lua", ScriptedEventScope::Global, error);
    REQUIRE_MESSAGE(program.has_value(), error.c_str());

    EventRuntime eventRuntime = {};
    runtimeState.statusMessages.clear();
    REQUIRE(eventRuntime.executeEventById(std::nullopt, program, 9001, runtimeState, &party));
    REQUIRE_EQ(runtimeState.statusMessages.size(), 1u);
    CHECK_EQ(runtimeState.statusMessages.front(), "novice");

    party.member(1)->skills["Hacking"] = {"Hacking", 4, SkillMastery::Expert};
    runtimeState.statusMessages.clear();
    REQUIRE(eventRuntime.executeEventById(std::nullopt, program, 9001, runtimeState, &party));
    REQUIRE_EQ(runtimeState.statusMessages.size(), 1u);
    CHECK_EQ(runtimeState.statusMessages.front(), "open");
}

TEST_CASE("energy guns fire as blasters with Energy Guns damage; energy blades add Energy Blades rank")
{
    const ItemTable &itemTable = itemTableWithHesperItems();

    Character gunner = makeMember("Ada", "Knight");
    gunner.skills["EnergyGuns"] = {"EnergyGuns", 3, SkillMastery::Normal};
    gunner.equipment.mainHand = 11000;
    const CharacterAttackProfile gun = GameMechanics::buildCharacterAttackProfile(gunner, &itemTable, nullptr);
    CHECK(gun.hasBlaster);
    CHECK_EQ(gun.rangedMinDamage, 4 + 10 + 3);
    CHECK_EQ(gun.rangedMaxDamage, 24 + 10 + 3);
    CHECK_EQ(gun.rangedSkillLevel, 3u);

    Character swordsman = makeMember("Bram", "Knight");
    swordsman.skills["Sword"] = {"Sword", 4, SkillMastery::Normal};
    swordsman.equipment.mainHand = 11022;
    const CharacterSheetSummary classic = GameMechanics::buildCharacterSheetSummary(swordsman, &itemTable);
    swordsman.skills["EnergyBlades"] = {"EnergyBlades", 5, SkillMastery::Normal};
    const CharacterSheetSummary energy = GameMechanics::buildCharacterSheetSummary(swordsman, &itemTable);
    CHECK_EQ(classic.combat.attack, energy.combat.attack);
    CHECK_EQ(damageRange(energy.combat.meleeDamageText).first, damageRange(classic.combat.meleeDamageText).first + 5);
    CHECK_EQ(damageRange(energy.combat.meleeDamageText).second, damageRange(classic.combat.meleeDamageText).second + 5);

    // A classic sword with the same skills gets nothing from Energy Blades.
    swordsman.equipment.mainHand = 2;
    const CharacterSheetSummary classicSword = GameMechanics::buildCharacterSheetSummary(swordsman, &itemTable);
    swordsman.skills.erase("EnergyBlades");
    CHECK_EQ(GameMechanics::buildCharacterSheetSummary(swordsman, &itemTable).combat.meleeDamageText,
        classicSword.combat.meleeDamageText);
}
