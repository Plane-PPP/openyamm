#include "game/tables/MonsterModifiers.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>
#include <exception>
#include <sstream>

namespace OpenYAMM::Game
{
namespace
{
constexpr const char *ModifiersSchema = "openyamm.monsterModifiers.v1";
constexpr float DegreesToRadians = 3.14159265f / 180.0f;

using Kind = MonsterModifierParameterInfo::Kind;

MonsterModifierParameterInfo number(const char *pName, float defaultValue, float minimum, float maximum,
    const char *pDescription)
{
    MonsterModifierParameterInfo info;
    info.pName = pName;
    info.kind = Kind::Number;
    info.defaultNumber = defaultValue;
    info.minimum = minimum;
    info.maximum = maximum;
    info.pDescription = pDescription;
    return info;
}

MonsterModifierParameterInfo boolean(const char *pName, bool defaultValue, const char *pDescription)
{
    MonsterModifierParameterInfo info;
    info.pName = pName;
    info.kind = Kind::Boolean;
    info.defaultNumber = defaultValue ? 1.0f : 0.0f;
    info.pDescription = pDescription;
    return info;
}

MonsterModifierParameterInfo text(const char *pName, const char *pDefault, const char *pDescription)
{
    MonsterModifierParameterInfo info;
    info.pName = pName;
    info.kind = Kind::Text;
    info.pDefaultText = pDefault;
    info.pDescription = pDescription;
    return info;
}

MonsterModifierParameterInfo choice(const char *pName, const char *pDefault, const char *pChoices,
    const char *pDescription)
{
    MonsterModifierParameterInfo info = text(pName, pDefault, pDescription);
    info.kind = Kind::Choice;
    info.pChoices = pChoices;
    return info;
}

std::vector<MonsterModifierInfo> buildCatalogue()
{
    return {
        {"true_aim",
            "Its projectiles home onto the target they were fired at (the party or an actor): they turn toward its "
            "current position, keep their speed and still hit whatever is in the way, so only cover stops them.",
            {
                number("turn_degrees_per_second", 180.0f, 1.0f, 3600.0f, "the most a projectile turns per second"),
                number("max_seconds", 4.0f, 0.0f, 60.0f, "how long it homes; 0: its whole flight"),
                choice("then", "straight", "straight|expire", "after max_seconds: fly straight on or expire"),
            }},
        {"never_staggers",
            "Hits never interrupt it: no hit reaction (the flinch and its knock-back). Stuns and paralysis from "
            "spells, skills and the EMP grenade still apply.",
            {}},
        {"kites_party",
            "Keeps its ranged band: the tactics ranged_band [min_distance, max_distance] (backs off inside, closes in "
            "beyond, circles between shots). A monster whose behaviors.yml tactics set ranged_band may not have it.",
            {
                number("min_distance", 768.0f, 0.0f, 20000.0f, "edge distance it backs off inside"),
                number("max_distance", 2304.0f, 1.0f, 20000.0f, "edge distance it closes in beyond"),
            }},
        {"blink",
            "A short teleport to a free spot it can see (floor, walls, actors and the party checked indoors and "
            "outdoors), with an effect and a sound at both ends. Needs at least one trigger.",
            {
                number("on_hit_chance", 0.0f, 0.0f, 1.0f, "chance per damaging hit"),
                boolean("on_shield_break", false, "when its energy shield breaks"),
                number("every_seconds", 0.0f, 0.0f, 600.0f, "periodically while it fights (0: never)"),
                number("cooldown_seconds", 6.0f, 0.0f, 600.0f, "at most one blink per this long"),
                number("min_distance", 384.0f, 32.0f, 8192.0f, "shortest jump"),
                number("max_distance", 1024.0f, 32.0f, 8192.0f, "longest jump"),
                number("max_height_change", 256.0f, 0.0f, 4096.0f, "how far the floor may rise or drop"),
                text("effect", "openyamm:fx/arcane_pulse", "effect id played where it leaves and arrives"),
                text("sound", "monster_blink", "placeholder sound name"),
                text("fallback_sound", "30teleport03", "sound played while the placeholder has no file"),
            }},
        {"charge",
            "A readable dash onto the party: with the party in sight within the trigger band it winds up (holds, "
            "flashes, a sound), then dashes straight at it, following the floor and stopping at walls, ledges and "
            "other bodies. Contact hits once and knocks the party back; a miss ends in a stumble.",
            {
                number("trigger_min_distance", 384.0f, 0.0f, 8192.0f, "closest party distance it charges from"),
                number("trigger_max_distance", 1280.0f, 32.0f, 8192.0f, "farthest party distance it charges from"),
                number("windup_seconds", 0.6f, 0.0f, 5.0f, "how long it holds and telegraphs before the dash"),
                number("speed", 1400.0f, 100.0f, 8000.0f, "dash speed in units a second"),
                number("max_distance", 1536.0f, 64.0f, 8192.0f, "longest dash"),
                number("damage", 0.0f, 0.0f, 10000.0f, "fixed damage of the hit instead of the melee attack"),
                boolean("use_melee", true, "the hit rolls the monster's melee attack (Attack1)"),
                number("knockback", 256.0f, 0.0f, 2048.0f, "how far the hit throws the party back"),
                number("cooldown_seconds", 8.0f, 0.0f, 600.0f, "time from the end of one charge to the next"),
                number("stumble_seconds", 1.2f, 0.0f, 10.0f, "how long it stumbles after a miss"),
                text("effect", "openyamm:fx/ember_impact", "effect id of the wind-up flash"),
                text("windup_sound", "monster_charge_windup", "placeholder sound of the wind-up"),
                text("windup_fallback_sound", "roar 1", "sound played while the wind-up placeholder has no file"),
                text("hit_sound", "monster_charge_hit", "placeholder sound of the hit"),
                text("hit_fallback_sound", "swing with blunt weapon01",
                    "sound played while the hit placeholder has no file"),
            }},
    };
}

struct ParsedModifier
{
    const MonsterModifierInfo *pInfo = nullptr;
    std::vector<float> numbers;
    std::vector<std::string> texts;
    // The parameters the file gives (not defaults).
    std::vector<bool> given;

    size_t index(const char *pName) const
    {
        for (size_t parameter = 0; parameter < pInfo->parameters.size(); ++parameter)
        {
            if (std::string(pInfo->parameters[parameter].pName) == pName)
            {
                return parameter;
            }
        }
        return 0;
    }
    float numberOf(const char *pName) const
    {
        return numbers[index(pName)];
    }
    bool flagOf(const char *pName) const
    {
        return numbers[index(pName)] != 0.0f;
    }
    const std::string &textOf(const char *pName) const
    {
        return texts[index(pName)];
    }
    bool givenOf(const char *pName) const
    {
        return given[index(pName)];
    }
};

std::string lineOf(const YAML::Node &node)
{
    const YAML::Mark mark = node.Mark();
    return mark.line >= 0 ? std::to_string(mark.line + 1) : "?";
}

std::string knownNames()
{
    std::string names;
    for (const MonsterModifierInfo &info : monsterModifierCatalogue())
    {
        names += names.empty() ? "" : ", ";
        names += info.pName;
    }
    return names;
}

// Names that are not modifiers because the data lives elsewhere: the error says where.
const char *redirectFor(const std::string &name)
{
    if (name == "energy_immune")
    {
        return "energy_immune belongs to shields.yml (monsters: {<id>: {energy_immune: true}})";
    }
    if (name == "stationary")
    {
        return "stationary is the monster row's Movement column (Stationary) in monster_data";
    }
    return nullptr;
}

bool parseParameters(const MonsterModifierInfo &info, const YAML::Node &params, ParsedModifier &parsed,
    std::string &error)
{
    parsed.pInfo = &info;
    parsed.numbers.resize(info.parameters.size());
    parsed.texts.resize(info.parameters.size());
    parsed.given.assign(info.parameters.size(), false);
    for (size_t parameter = 0; parameter < info.parameters.size(); ++parameter)
    {
        parsed.numbers[parameter] = info.parameters[parameter].defaultNumber;
        parsed.texts[parameter] = info.parameters[parameter].pDefaultText;
    }

    if (!params || params.IsNull())
    {
        return true;
    }
    if (!params.IsMap())
    {
        error = lineOf(params) + ": " + info.pName + " takes a map of parameters";
        return false;
    }

    for (const auto &entry : params)
    {
        const std::string key = entry.first.as<std::string>();
        const auto found = std::find_if(info.parameters.begin(), info.parameters.end(),
            [&](const MonsterModifierParameterInfo &parameter) { return key == parameter.pName; });
        if (found == info.parameters.end())
        {
            std::string known;
            for (const MonsterModifierParameterInfo &parameter : info.parameters)
            {
                known += known.empty() ? "" : ", ";
                known += parameter.pName;
            }
            error = lineOf(entry.first) + ": " + info.pName + " has no parameter '" + key + "'"
                + (known.empty() ? std::string(" (it takes none)") : " (known: " + known + ")");
            return false;
        }

        const size_t parameter = size_t(found - info.parameters.begin());
        parsed.given[parameter] = true;
        const std::string where = lineOf(entry.second) + ": " + info.pName + "." + key;
        if (!entry.second.IsScalar())
        {
            error = where + " must be a single value";
            return false;
        }

        switch (found->kind)
        {
            case Kind::Number:
            {
                float value = 0.0f;
                if (!YAML::convert<float>::decode(entry.second, value) || !std::isfinite(value)
                    || value < found->minimum || value > found->maximum)
                {
                    std::ostringstream range;
                    range << found->minimum << "-" << found->maximum;
                    error = where + " must be a number in " + range.str();
                    return false;
                }
                parsed.numbers[parameter] = value;
                break;
            }
            case Kind::Boolean:
            {
                bool value = false;
                if (!YAML::convert<bool>::decode(entry.second, value))
                {
                    error = where + " must be true or false";
                    return false;
                }
                parsed.numbers[parameter] = value ? 1.0f : 0.0f;
                break;
            }
            case Kind::Text:
            {
                const std::string value = entry.second.as<std::string>();
                if (value.empty())
                {
                    error = where + " must not be empty";
                    return false;
                }
                parsed.texts[parameter] = value;
                break;
            }
            case Kind::Choice:
            {
                const std::string value = entry.second.as<std::string>();
                std::istringstream choices(found->pChoices);
                std::string option;
                bool known = false;
                while (std::getline(choices, option, '|'))
                {
                    known = known || option == value;
                }
                if (!known)
                {
                    error = where + " must be one of " + found->pChoices;
                    return false;
                }
                parsed.texts[parameter] = value;
                break;
            }
        }
    }
    return true;
}

bool applyModifier(const ParsedModifier &parsed, const std::string &line, MonsterModifierSet &set,
    std::string &error)
{
    const std::string name = parsed.pInfo->pName;
    if (std::find(set.names.begin(), set.names.end(), name) != set.names.end())
    {
        error = line + ": " + name + " is listed twice";
        return false;
    }

    if (name == "true_aim")
    {
        TrueAimModifier trueAim;
        trueAim.turnRadiansPerSecond = parsed.numberOf("turn_degrees_per_second") * DegreesToRadians;
        trueAim.maxSeconds = parsed.numberOf("max_seconds");
        trueAim.expireAfter = parsed.textOf("then") == "expire";
        set.trueAim = trueAim;
    }
    else if (name == "never_staggers")
    {
        set.neverStaggers = true;
    }
    else if (name == "kites_party")
    {
        KitesPartyModifier kites;
        kites.minDistance = parsed.numberOf("min_distance");
        kites.maxDistance = parsed.numberOf("max_distance");
        if (kites.minDistance >= kites.maxDistance)
        {
            error = line + ": kites_party.min_distance must be below max_distance";
            return false;
        }
        set.kitesParty = kites;
    }
    else if (name == "blink")
    {
        BlinkModifier blink;
        blink.onHitChance = parsed.numberOf("on_hit_chance");
        blink.onShieldBreak = parsed.flagOf("on_shield_break");
        blink.everySeconds = parsed.numberOf("every_seconds");
        blink.cooldownSeconds = parsed.numberOf("cooldown_seconds");
        blink.minDistance = parsed.numberOf("min_distance");
        blink.maxDistance = parsed.numberOf("max_distance");
        blink.maxHeightChange = parsed.numberOf("max_height_change");
        blink.effect = parsed.textOf("effect");
        blink.sound = parsed.textOf("sound");
        blink.fallbackSound = parsed.textOf("fallback_sound");
        if (blink.minDistance > blink.maxDistance)
        {
            error = line + ": blink.min_distance must not exceed max_distance";
            return false;
        }
        if (blink.onHitChance <= 0.0f && !blink.onShieldBreak && blink.everySeconds <= 0.0f)
        {
            error = line + ": blink needs a trigger (on_hit_chance, on_shield_break or every_seconds)";
            return false;
        }
        set.blink = blink;
    }
    else if (name == "charge")
    {
        ChargeModifier charge;
        charge.triggerMinDistance = parsed.numberOf("trigger_min_distance");
        charge.triggerMaxDistance = parsed.numberOf("trigger_max_distance");
        charge.windupSeconds = parsed.numberOf("windup_seconds");
        charge.speed = parsed.numberOf("speed");
        charge.maxDistance = parsed.numberOf("max_distance");
        charge.damage = parsed.numberOf("damage");
        // A damage given without use_melee replaces the melee attack.
        charge.useMelee = parsed.givenOf("use_melee") ? parsed.flagOf("use_melee") : charge.damage <= 0.0f;
        charge.knockback = parsed.numberOf("knockback");
        charge.cooldownSeconds = parsed.numberOf("cooldown_seconds");
        charge.stumbleSeconds = parsed.numberOf("stumble_seconds");
        charge.effect = parsed.textOf("effect");
        charge.windupSound = parsed.textOf("windup_sound");
        charge.windupFallbackSound = parsed.textOf("windup_fallback_sound");
        charge.hitSound = parsed.textOf("hit_sound");
        charge.hitFallbackSound = parsed.textOf("hit_fallback_sound");
        if (charge.triggerMinDistance >= charge.triggerMaxDistance)
        {
            error = line + ": charge.trigger_min_distance must be below trigger_max_distance";
            return false;
        }
        if (charge.useMelee == (charge.damage > 0.0f))
        {
            error = line + ": charge hits with its melee attack (use_melee: true, no damage) or with a fixed damage "
                "above 0 (use_melee: false)";
            return false;
        }
        set.charge = charge;
    }
    set.names.push_back(name);
    return true;
}

// "10065" or "10050-10052".
bool parseIdKey(const std::string &key, int &first, int &last)
{
    const size_t dash = key.find('-', 1);
    try
    {
        size_t used = 0;
        first = std::stoi(key.substr(0, dash), &used);
        if (used != (dash == std::string::npos ? key.size() : dash))
        {
            return false;
        }
        last = first;
        if (dash != std::string::npos)
        {
            const std::string tail = key.substr(dash + 1);
            last = std::stoi(tail, &used);
            if (used != tail.size())
            {
                return false;
            }
        }
    }
    catch (const std::exception &)
    {
        return false;
    }
    return first <= last;
}
}

const std::vector<MonsterModifierInfo> &monsterModifierCatalogue()
{
    static const std::vector<MonsterModifierInfo> catalogue = buildCatalogue();
    return catalogue;
}

const MonsterModifierInfo *findMonsterModifierInfo(const std::string &name)
{
    for (const MonsterModifierInfo &info : monsterModifierCatalogue())
    {
        if (name == info.pName)
        {
            return &info;
        }
    }
    return nullptr;
}

bool MonsterModifierTable::loadWorldYaml(const std::string &text, const std::string &packageId, uint32_t idBegin,
    uint32_t idEnd, std::string &error)
{
    const std::string file = "worlds/" + packageId + "/modifiers.yml";
    try
    {
        const YAML::Node root = YAML::Load(text);
        if (!root.IsMap() || root["schema"].as<std::string>("") != ModifiersSchema)
        {
            error = file + ": expected schema " + ModifiersSchema;
            return false;
        }

        const YAML::Node monsters = root["monsters"];
        if (monsters && !monsters.IsMap())
        {
            error = file + ":" + lineOf(monsters) + ": monsters must map ids to lists of modifiers";
            return false;
        }

        std::unordered_map<int16_t, MonsterModifierSet> loaded;
        for (const auto &entry : monsters)
        {
            const std::string key = entry.first.as<std::string>();
            const std::string keyLine = file + ":" + lineOf(entry.first);
            int first = 0;
            int last = 0;
            if (!parseIdKey(key, first, last))
            {
                error = keyLine + ": '" + key + "' is not a monster id or an id range (first-last)";
                return false;
            }
            if (first <= 0 || uint32_t(first) < idBegin || uint32_t(last) > idEnd || last > INT16_MAX)
            {
                error = keyLine + ": " + key + " lies outside the world's monster id range";
                return false;
            }
            if (!entry.second.IsSequence())
            {
                error = keyLine + ": " + key + " must list its modifiers ([name, {name: {parameters}}])";
                return false;
            }

            MonsterModifierSet set;
            for (const YAML::Node &item : entry.second)
            {
                std::string name;
                YAML::Node params;
                if (item.IsScalar())
                {
                    name = item.as<std::string>();
                }
                else if (item.IsMap() && item.size() == 1)
                {
                    name = item.begin()->first.as<std::string>();
                    params = item.begin()->second;
                }
                else
                {
                    error = file + ":" + lineOf(item) + ": a modifier is a name or {name: {parameters}}";
                    return false;
                }

                const MonsterModifierInfo *pInfo = findMonsterModifierInfo(name);
                if (pInfo == nullptr)
                {
                    const char *pRedirect = redirectFor(name);
                    error = file + ":" + lineOf(item) + ": "
                        + (pRedirect != nullptr ? std::string(pRedirect)
                                                : "unknown modifier '" + name + "' (known: " + knownNames() + ")");
                    return false;
                }

                ParsedModifier parsed;
                std::string parameterError;
                if (!parseParameters(*pInfo, params, parsed, parameterError)
                    || !applyModifier(parsed, lineOf(item), set, parameterError))
                {
                    error = file + ":" + parameterError;
                    return false;
                }
            }

            for (int id = first; id <= last; ++id)
            {
                if (loaded.contains(int16_t(id)) || m_sets.contains(int16_t(id)))
                {
                    error = keyLine + ": monster " + std::to_string(id) + " is listed twice";
                    return false;
                }
                MonsterModifierSet copy = set;
                copy.monsterId = int16_t(id);
                loaded.emplace(int16_t(id), std::move(copy));
            }
        }

        for (auto &entry : loaded)
        {
            set(entry.second);
        }
    }
    catch (const std::exception &exception)
    {
        error = file + ": " + exception.what();
        return false;
    }
    return true;
}

void MonsterModifierTable::clear()
{
    *this = {};
}

const MonsterModifierSet *MonsterModifierTable::find(int16_t monsterId) const
{
    if (monsterId < m_minimumMonsterId || monsterId > m_maximumMonsterId)
    {
        return nullptr;
    }
    const auto found = m_sets.find(monsterId);
    return found != m_sets.end() ? &found->second : nullptr;
}

void MonsterModifierTable::set(const MonsterModifierSet &modifiers)
{
    m_sets[modifiers.monsterId] = modifiers;
    m_hasBlink = m_hasBlink || modifiers.blink.has_value();
    m_hasCharge = m_hasCharge || modifiers.charge.has_value();
    m_minimumMonsterId = std::min(m_minimumMonsterId, modifiers.monsterId);
    m_maximumMonsterId = std::max(m_maximumMonsterId, modifiers.monsterId);
}
}
