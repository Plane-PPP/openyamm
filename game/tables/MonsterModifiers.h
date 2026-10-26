#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace OpenYAMM::Game
{
// One parameter of a catalogued modifier: its YAML key, its kind, its default and its range (numbers) or its choices
// (`straight|expire`).
struct MonsterModifierParameterInfo
{
    enum class Kind
    {
        Number,
        Boolean,
        Text,
        Choice,
    };

    const char *pName = "";
    Kind kind = Kind::Number;
    float defaultNumber = 0.0f;
    float minimum = 0.0f;
    float maximum = 0.0f;
    // The default of a Text or Choice parameter; Choice lists its values separated by '|'.
    const char *pDefaultText = "";
    const char *pChoices = "";
    const char *pDescription = "";
};

struct MonsterModifierInfo
{
    const char *pName = "";
    const char *pDescription = "";
    std::vector<MonsterModifierParameterInfo> parameters;
};

// The modifiers a world's modifiers.yml may give its monsters, with their parameters and defaults
// (devdocs/MONSTER_MODIFIERS.md).
const std::vector<MonsterModifierInfo> &monsterModifierCatalogue();
const MonsterModifierInfo *findMonsterModifierInfo(const std::string &name);

// true_aim: the monster's projectiles home onto the target they were fired at (the party or an actor), turning at
// most turnRadiansPerSecond and keeping their speed; after maxSeconds (0: their whole flight) they fly straight on,
// or expire with expireAfter. They still collide with everything in their way.
struct TrueAimModifier
{
    float turnRadiansPerSecond = 3.14159265f;
    float maxSeconds = 4.0f;
    bool expireAfter = false;
};

// kites_party: the tactics' ranged band (ActorTacticsFacts::rangedMinDistance/rangedMaxDistance).
struct KitesPartyModifier
{
    float minDistance = 768.0f;
    float maxDistance = 2304.0f;
};

// blink: a short teleport to a free spot in sight (GameplayActorModifiers), on a hit (onHitChance), when its energy
// shield breaks (onShieldBreak) and/or every everySeconds while it fights, at most once per cooldownSeconds.
struct BlinkModifier
{
    float onHitChance = 0.0f;
    bool onShieldBreak = false;
    float everySeconds = 0.0f;
    float cooldownSeconds = 6.0f;
    float minDistance = 384.0f;
    float maxDistance = 1024.0f;
    float maxHeightChange = 256.0f;
    std::string effect = "openyamm:fx/arcane_pulse";
    // A placeholder sound name (a file of that name in a world's or the engine's audio), and what plays meanwhile.
    std::string sound = "monster_blink";
    std::string fallbackSound = "30teleport03";
};

// charge: a readable dash onto the party (GameplayActorModifiers). While the party is within
// [triggerMinDistance, triggerMaxDistance] and in sight and its cooldown is ready, it winds up for windupSeconds
// (holds and faces the party, a flash and windupSound), then dashes straight along that line at speed for up to
// maxDistance, following the floor and stopping at walls, ledges, obstacles and other actors. Reaching the party it
// hits once (its melee attack, or damage) and knocks the party back knockback units; a miss ends in a stumble of
// stumbleSeconds. Then cooldownSeconds before the next charge.
struct ChargeModifier
{
    float triggerMinDistance = 384.0f;
    float triggerMaxDistance = 1280.0f;
    float windupSeconds = 0.6f;
    float speed = 1400.0f;
    float maxDistance = 1536.0f;
    // 0 with useMelee: the monster's melee attack (Attack1) rolls the damage.
    float damage = 0.0f;
    bool useMelee = true;
    float knockback = 256.0f;
    float cooldownSeconds = 8.0f;
    float stumbleSeconds = 1.2f;
    std::string effect = "openyamm:fx/ember_impact";
    // Placeholder sound names (a file of that name in a world's or the engine's audio), and what plays meanwhile.
    std::string windupSound = "monster_charge_windup";
    std::string windupFallbackSound = "roar 1";
    std::string hitSound = "monster_charge_hit";
    std::string hitFallbackSound = "swing with blunt weapon01";
};

// The modifiers of one monster id.
struct MonsterModifierSet
{
    int16_t monsterId = 0;
    std::optional<TrueAimModifier> trueAim;
    bool neverStaggers = false;
    std::optional<KitesPartyModifier> kitesParty;
    std::optional<BlinkModifier> blink;
    std::optional<ChargeModifier> charge;
    // The modifier names in file order (console, logs).
    std::vector<std::string> names;
};

// Per-monster modifiers from the mounted worlds' modifiers.yml (schema openyamm.monsterModifiers.v1). A monster no
// world lists has none, so the engine behaves exactly as without the file.
class MonsterModifierTable
{
public:
    // Adds one world's file; its monster ids (single ids or ranges "first-last") must lie in [idBegin, idEnd].
    // Errors name the file and line: unknown modifiers or parameters, bad values, ids listed twice.
    bool loadWorldYaml(const std::string &text, const std::string &packageId, uint32_t idBegin, uint32_t idEnd,
        std::string &error);
    void clear();
    bool empty() const
    {
        return m_sets.empty();
    }
    const MonsterModifierSet *find(int16_t monsterId) const;
    const std::unordered_map<int16_t, MonsterModifierSet> &sets() const
    {
        return m_sets;
    }
    // Some monster blinks (GameplayActorModifiers has work to do).
    bool hasBlink() const
    {
        return m_hasBlink;
    }
    // Some monster charges.
    bool hasCharge() const
    {
        return m_hasCharge;
    }

    // Test and tool setup without YAML.
    void set(const MonsterModifierSet &modifiers);

private:
    std::unordered_map<int16_t, MonsterModifierSet> m_sets;
    bool m_hasBlink = false;
    bool m_hasCharge = false;
    int16_t m_minimumMonsterId = INT16_MAX;
    int16_t m_maximumMonsterId = INT16_MIN;
};
}
