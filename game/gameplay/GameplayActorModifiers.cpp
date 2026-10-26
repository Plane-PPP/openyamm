#include "game/gameplay/GameplayActorModifiers.h"

#include "game/events/EventRuntime.h"
#include "game/gameplay/GameMechanics.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace OpenYAMM::Game
{
namespace
{
constexpr float Pi = 3.14159265f;
// A hit or a shield break asks for a blink for this long (a cooldown or a stun may delay it).
constexpr float PendingSeconds = 1.0f;
// Without a hit or an attack for this long the actor no longer fights: periodic blinks pause.
constexpr float CombatMemorySeconds = 5.0f;
// After a blink found no free spot.
constexpr float RetrySeconds = 0.75f;

uint32_t mix(uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

float unitFloat(uint32_t value)
{
    return float(mix(value) >> 8) / float(1u << 24);
}
}

std::optional<GameplayProjectileService::ProjectileHoming> trueAimProjectileHoming(
    const MonsterModifierSet *pModifiers,
    GameplayProjectileService::ProjectileHoming::TargetKind targetKind,
    size_t targetActorIndex,
    uint32_t targetActorId)
{
    using TargetKind = GameplayProjectileService::ProjectileHoming::TargetKind;

    if (pModifiers == nullptr || !pModifiers->trueAim
        || (targetKind == TargetKind::Actor && targetActorIndex == static_cast<size_t>(-1)))
    {
        return std::nullopt;
    }

    GameplayProjectileService::ProjectileHoming homing = {};
    homing.targetKind = targetKind;
    homing.targetActorIndex = targetKind == TargetKind::Actor ? targetActorIndex : static_cast<size_t>(-1);
    homing.targetActorId = targetKind == TargetKind::Actor ? targetActorId : 0;
    homing.turnRadiansPerSecond = pModifiers->trueAim->turnRadiansPerSecond;
    homing.maxSeconds = pModifiers->trueAim->maxSeconds;
    homing.expireAfter = pModifiers->trueAim->expireAfter;
    return homing;
}

void GameplayActorModifiers::bind(const MonsterModifierTable *pTable)
{
    m_pTable = pTable;
    m_entries.clear();
    m_charges.clear();
}

void GameplayActorModifiers::clear()
{
    m_entries.clear();
    m_charges.clear();
}

GameplayActorModifiers::Entry *GameplayActorModifiers::ensure(size_t actorIndex, int16_t monsterId, uint32_t actorId)
{
    if (!active())
    {
        return nullptr;
    }

    const MonsterModifierSet *pModifiers = m_pTable->find(monsterId);

    if (pModifiers == nullptr || !pModifiers->blink)
    {
        return nullptr;
    }

    if (actorIndex >= m_entries.size())
    {
        m_entries.resize(actorIndex + 1);
    }

    Entry &entry = m_entries[actorIndex];

    if (!entry.used || entry.actorId != actorId || entry.monsterId != monsterId)
    {
        entry = {};
        entry.used = true;
        entry.actorId = actorId;
        entry.monsterId = monsterId;
        entry.pBlink = &*pModifiers->blink;
    }

    return &entry;
}

void GameplayActorModifiers::observeHit(size_t actorIndex, int16_t monsterId, uint32_t actorId,
    const std::array<float, 3> &source, bool damaged, bool shieldBroke)
{
    Entry *pEntry = ensure(actorIndex, monsterId, actorId);

    if (pEntry == nullptr || (!damaged && !shieldBroke))
    {
        return;
    }

    pEntry->threat = source;
    pEntry->sinceCombatSeconds = 0.0f;
    ++pEntry->hitCount;

    if (shieldBroke && pEntry->pBlink->onShieldBreak)
    {
        pEntry->pendingSeconds = PendingSeconds;
    }
    else if (damaged && pEntry->pBlink->onHitChance > 0.0f
        && unitFloat(actorId * 0x9e3779b9u ^ pEntry->hitCount * 0x85ebca6bu) < pEntry->pBlink->onHitChance)
    {
        pEntry->pendingSeconds = PendingSeconds;
    }
}

void GameplayActorModifiers::observeAttack(size_t actorIndex, int16_t monsterId, uint32_t actorId,
    const std::array<float, 3> &target)
{
    Entry *pEntry = ensure(actorIndex, monsterId, actorId);

    if (pEntry != nullptr)
    {
        pEntry->threat = target;
        pEntry->sinceCombatSeconds = 0.0f;
    }
}

std::vector<GameplayActorModifiers::BlinkRequest> GameplayActorModifiers::update(float deltaSeconds,
    size_t actorCount, const std::function<ActorFacts(size_t)> &actorFacts)
{
    std::vector<BlinkRequest> requests;

    if (!active() || deltaSeconds <= 0.0f)
    {
        return requests;
    }

    const size_t count = std::min(actorCount, m_entries.size());

    for (size_t actorIndex = 0; actorIndex < count; ++actorIndex)
    {
        Entry &entry = m_entries[actorIndex];

        if (!entry.used)
        {
            continue;
        }

        const ActorFacts facts = actorFacts(actorIndex);

        if (facts.actorId != entry.actorId || facts.monsterId != entry.monsterId)
        {
            entry = {};
            continue;
        }

        const BlinkModifier &blink = *entry.pBlink;
        entry.cooldownSeconds = std::max(0.0f, entry.cooldownSeconds - deltaSeconds);
        entry.sinceCombatSeconds += deltaSeconds;

        if (entry.sinceCombatSeconds < CombatMemorySeconds)
        {
            entry.sincePeriodicSeconds += deltaSeconds;
        }

        const bool periodicDue = blink.everySeconds > 0.0f && entry.sincePeriodicSeconds >= blink.everySeconds
            && entry.sinceCombatSeconds < CombatMemorySeconds;
        const bool due = entry.pendingSeconds > 0.0f || periodicDue;
        entry.pendingSeconds = std::max(0.0f, entry.pendingSeconds - deltaSeconds);

        if (!facts.canAct || !due || entry.cooldownSeconds > 0.0f)
        {
            continue;
        }

        BlinkRequest request = {};
        request.actorIndex = actorIndex;
        request.actorId = entry.actorId;
        request.pBlink = entry.pBlink;
        request.origin = facts.position;
        request.threat = entry.threat;
        request.seed = mix(entry.actorId * 0x27d4eb2du ^ (entry.blinkCount + 1) * 0x165667b1u);
        requests.push_back(request);
    }

    return requests;
}

void GameplayActorModifiers::finishBlink(const BlinkRequest &request, bool blinked)
{
    if (request.actorIndex >= m_entries.size() || m_entries[request.actorIndex].actorId != request.actorId)
    {
        return;
    }

    Entry &entry = m_entries[request.actorIndex];

    if (!blinked)
    {
        entry.cooldownSeconds = RetrySeconds;
        return;
    }

    entry.cooldownSeconds = entry.pBlink->cooldownSeconds;
    entry.sincePeriodicSeconds = 0.0f;
    entry.pendingSeconds = 0.0f;
    ++entry.blinkCount;
    ++m_blinkCount;
}

std::vector<std::array<float, 3>> GameplayActorModifiers::blinkCandidates(const BlinkRequest &request, size_t count)
{
    struct Candidate
    {
        std::array<float, 3> position = {};
        float score = 0.0f;
    };

    std::vector<Candidate> candidates;

    if (request.pBlink == nullptr)
    {
        return {};
    }

    const float minimum = request.pBlink->minDistance;
    const float maximum = std::max(request.pBlink->maxDistance, minimum);
    float awayX = 0.0f;
    float awayY = 0.0f;

    if (request.threat)
    {
        awayX = request.origin[0] - (*request.threat)[0];
        awayY = request.origin[1] - (*request.threat)[1];
        const float length = std::sqrt(awayX * awayX + awayY * awayY);
        awayX = length > 0.001f ? awayX / length : 0.0f;
        awayY = length > 0.001f ? awayY / length : 0.0f;
    }

    for (size_t index = 0; index < count; ++index)
    {
        const uint32_t base = request.seed ^ uint32_t(index * 0x9e3779b9u);
        const float angle = unitFloat(base) * 2.0f * Pi;
        const float distance = minimum + (maximum - minimum) * unitFloat(base ^ 0x5bd1e995u);
        const float directionX = std::cos(angle);
        const float directionY = std::sin(angle);
        Candidate candidate = {};
        candidate.position = {
            request.origin[0] + directionX * distance,
            request.origin[1] + directionY * distance,
            request.origin[2]};
        // Sideways or away from the threat first, with some randomness so it does not always pick the same side.
        candidate.score = (directionX * awayX + directionY * awayY) + unitFloat(base ^ 0x68e31da4u) * 0.75f;
        candidates.push_back(candidate);
    }

    std::stable_sort(candidates.begin(), candidates.end(),
        [](const Candidate &left, const Candidate &right) { return left.score > right.score; });
    std::vector<std::array<float, 3>> positions;
    positions.reserve(candidates.size());

    for (const Candidate &candidate : candidates)
    {
        positions.push_back(candidate.position);
    }

    return positions;
}

void GameplayActorModifiers::queueBlinkPresentation(EventRuntimeState &eventRuntimeState, const BlinkModifier &blink,
    const std::array<float, 3> &fromCenter, const std::array<float, 3> &toCenter)
{
    for (const std::array<float, 3> &point : {fromCenter, toCenter})
    {
        EventRuntimeState::WorldEffectRequest effect = {};
        effect.id = blink.effect;
        effect.position = {point[0], point[1], point[2]};
        effect.scale = 1.0f;
        eventRuntimeState.worldEffectRequests.push_back(std::move(effect));
    }

    EventRuntimeState::PendingSound sound = {};
    sound.soundScope = SoundScope::World;
    sound.soundName = blink.sound;
    sound.fallbackSoundName = blink.fallbackSound;
    sound.x = int32_t(std::lround(toCenter[0]));
    sound.y = int32_t(std::lround(toCenter[1]));
    sound.z = int32_t(std::lround(toCenter[2]));
    sound.positional = true;
    sound.hasExplicitZ = true;
    eventRuntimeState.pendingSounds.push_back(std::move(sound));
}

GameplayActorModifiers::ChargeEntry *GameplayActorModifiers::ensureCharge(size_t actorIndex, int16_t monsterId,
    uint32_t actorId)
{
    if (!chargeActive())
    {
        return nullptr;
    }

    const MonsterModifierSet *pModifiers = m_pTable->find(monsterId);

    if (pModifiers == nullptr || !pModifiers->charge)
    {
        return nullptr;
    }

    if (actorIndex >= m_charges.size())
    {
        m_charges.resize(actorIndex + 1);
    }

    ChargeEntry &entry = m_charges[actorIndex];

    if (!entry.used || entry.actorId != actorId || entry.monsterId != monsterId)
    {
        entry = {};
        entry.used = true;
        entry.actorId = actorId;
        entry.monsterId = monsterId;
        entry.pCharge = &*pModifiers->charge;
    }

    return &entry;
}

void GameplayActorModifiers::beginWindup(ChargeEntry &entry, size_t actorIndex, const std::array<float, 3> &position,
    const std::array<float, 3> &partyPosition, const ChargeWorld &world)
{
    // The line is fixed when the wind-up starts: a party that reads the telegraph can step aside.
    float directionX = partyPosition[0] - position[0];
    float directionY = partyPosition[1] - position[1];
    const float length = std::sqrt(directionX * directionX + directionY * directionY);
    directionX = length > 0.001f ? directionX / length : 1.0f;
    directionY = length > 0.001f ? directionY / length : 0.0f;
    entry.phase = ChargePhase::Windup;
    entry.timerSeconds = entry.pCharge->windupSeconds;
    entry.directionX = directionX;
    entry.directionY = directionY;
    entry.travelled = 0.0f;
    entry.x = position[0];
    entry.y = position[1];
    entry.stumbling = false;
    ++m_chargeCount;

    if (world.handle)
    {
        world.handle(ChargeEvent{ChargeEventKind::WindupStarted, actorIndex, entry.pCharge, directionX, directionY});
    }
}

void GameplayActorModifiers::endDash(ChargeEntry &entry, size_t actorIndex, bool hit, const ChargeWorld &world)
{
    entry.phase = ChargePhase::Recover;
    entry.stumbling = !hit;
    entry.timerSeconds = hit ? ChargeHitRecoverSeconds : entry.pCharge->stumbleSeconds;
    ++(hit ? m_chargeHitCount : m_chargeMissCount);

    if (world.handle)
    {
        world.handle(ChargeEvent{hit ? ChargeEventKind::Hit : ChargeEventKind::Missed, actorIndex, entry.pCharge,
            entry.directionX, entry.directionY});
    }
}

void GameplayActorModifiers::updateCharges(float deltaSeconds, size_t actorCount,
    const std::function<ChargeActorFacts(size_t)> &actorFacts, const ChargeWorld &world)
{
    if (!chargeActive() || deltaSeconds <= 0.0f)
    {
        return;
    }

    for (size_t actorIndex = 0; actorIndex < actorCount; ++actorIndex)
    {
        const ChargeActorFacts facts = actorFacts(actorIndex);
        ChargeEntry *pEntry = ensureCharge(actorIndex, facts.monsterId, facts.actorId);

        if (pEntry == nullptr)
        {
            continue;
        }

        ChargeEntry &entry = *pEntry;
        const ChargeModifier &charge = *entry.pCharge;

        // Death, a stun or paralysis ends a charge where it is; the cooldown then runs as after a miss.
        if (!facts.canAct && entry.phase != ChargePhase::Ready)
        {
            entry.phase = ChargePhase::Ready;
            entry.cooldownSeconds = charge.cooldownSeconds;
            continue;
        }

        switch (entry.phase)
        {
            case ChargePhase::Ready:
            {
                entry.cooldownSeconds = std::max(0.0f, entry.cooldownSeconds - deltaSeconds);

                if (facts.canAct && facts.engagesParty && entry.cooldownSeconds <= 0.0f
                    && facts.partyEdgeDistance >= charge.triggerMinDistance
                    && facts.partyEdgeDistance <= charge.triggerMaxDistance
                    && (!world.hasLineOfSightToParty || world.hasLineOfSightToParty(actorIndex)))
                {
                    beginWindup(entry, actorIndex, facts.position, facts.partyPosition, world);
                }
                break;
            }
            case ChargePhase::Windup:
            {
                entry.timerSeconds -= deltaSeconds;

                if (entry.timerSeconds <= 0.0f)
                {
                    entry.phase = ChargePhase::Dash;
                    entry.x = facts.position[0];
                    entry.y = facts.position[1];
                }
                break;
            }
            case ChargePhase::Dash:
            {
                const float yaw = std::atan2(entry.directionY, entry.directionX);
                float remaining = std::min(charge.speed * deltaSeconds, charge.maxDistance - entry.travelled);
                bool ended = false;

                while (remaining > 0.001f && !ended)
                {
                    const float stepLength = std::min(remaining, ChargeStepLength);
                    const float nextX = entry.x + entry.directionX * stepLength;
                    const float nextY = entry.y + entry.directionY * stepLength;
                    const ChargeStepResult result = world.step
                        ? world.step(actorIndex, nextX, nextY, ChargeStepHeightChange, yaw)
                        : ChargeStepResult::Blocked;

                    if (result == ChargeStepResult::Contact)
                    {
                        endDash(entry, actorIndex, true, world);
                        ended = true;
                    }
                    else if (result == ChargeStepResult::Blocked)
                    {
                        endDash(entry, actorIndex, false, world);
                        ended = true;
                    }
                    else
                    {
                        entry.x = nextX;
                        entry.y = nextY;
                        entry.travelled += stepLength;
                        remaining -= stepLength;
                    }
                }

                if (!ended && entry.travelled >= charge.maxDistance - 0.001f)
                {
                    endDash(entry, actorIndex, false, world);
                }
                break;
            }
            case ChargePhase::Recover:
            {
                entry.timerSeconds -= deltaSeconds;

                if (entry.timerSeconds <= 0.0f)
                {
                    entry.phase = ChargePhase::Ready;
                    entry.stumbling = false;
                    entry.cooldownSeconds = charge.cooldownSeconds;
                }
                break;
            }
        }
    }
}

void GameplayActorModifiers::applyChargeHolds(ActorAiFrameFacts &facts) const
{
    if (m_charges.empty())
    {
        return;
    }

    const auto applyToActor = [this](ActorAiFacts &actor)
    {
        if (actor.actorIndex >= m_charges.size())
        {
            return;
        }

        const ChargeEntry &entry = m_charges[actor.actorIndex];

        if (!entry.used || entry.actorId != actor.actorId || entry.phase == ChargePhase::Ready)
        {
            return;
        }

        actor.behavior.holdActive = true;
        actor.behavior.holdAnimation = entry.phase == ChargePhase::Dash ? ActorAiAnimationState::Walking
            : entry.phase == ChargePhase::Recover && entry.stumbling ? ActorAiAnimationState::GotHit
            : ActorAiAnimationState::Standing;
    };

    for (ActorAiFacts &actor : facts.activeActors)
    {
        applyToActor(actor);
    }

    for (ActorAiFacts &actor : facts.backgroundActors)
    {
        applyToActor(actor);
    }
}

GameplayActorModifiers::ChargePhase GameplayActorModifiers::chargePhase(size_t actorIndex) const
{
    return actorIndex < m_charges.size() && m_charges[actorIndex].used ? m_charges[actorIndex].phase
                                                                          : ChargePhase::Ready;
}

bool GameplayActorModifiers::startCharge(size_t actorIndex, int16_t monsterId, uint32_t actorId,
    const std::array<float, 3> &position, const std::array<float, 3> &partyPosition, const ChargeWorld &world)
{
    ChargeEntry *pEntry = ensureCharge(actorIndex, monsterId, actorId);

    if (pEntry == nullptr || pEntry->phase != ChargePhase::Ready)
    {
        return false;
    }

    beginWindup(*pEntry, actorIndex, position, partyPosition, world);
    return true;
}

GameplayActorModifiers::ChargeHit GameplayActorModifiers::chargeHit(const ChargeModifier &charge,
    const MonsterTable::MonsterStatsEntry *pStats, uint32_t seed)
{
    ChargeHit hit = {};

    if (!charge.useMelee || pStats == nullptr)
    {
        hit.damage = std::max(1, int(std::lround(charge.damage)));
        return hit;
    }

    const bool useAttack2 = pStats->attack1HasMissile && !pStats->attack2HasMissile
        && pStats->attack2Damage.diceRolls > 0 && pStats->attack2Damage.diceSides > 0;
    const MonsterTable::MonsterStatsEntry::DamageProfile &profile =
        useAttack2 ? pStats->attack2Damage : pStats->attack1Damage;
    std::mt19937 rng(mix(seed));
    hit.damage = std::max(1,
        GameMechanics::rollMonsterAttackDamage(profile.diceRolls, profile.diceSides, profile.bonus, rng));
    hit.damageType = GameMechanics::parseCombatDamageType(useAttack2 ? pStats->attack2Type : pStats->attack1Type);
    hit.ability = useAttack2 ? GameplayActorAttackAbility::Attack2 : GameplayActorAttackAbility::Attack1;
    return hit;
}

std::array<float, 3> GameplayActorModifiers::chargeKnockbackImpulse(float directionX, float directionY,
    float distance, float gravity)
{
    if (distance <= 0.0f || gravity <= 0.0f)
    {
        return {0.0f, 0.0f, 0.0f};
    }

    // A short hop: up at a fixed speed, so the throw lasts 2 * up / gravity and covers distance in that time.
    constexpr float UpSpeed = 320.0f;
    const float airSeconds = 2.0f * UpSpeed / gravity;
    const float horizontalSpeed = distance / airSeconds;
    return {directionX * horizontalSpeed, directionY * horizontalSpeed, UpSpeed};
}

void GameplayActorModifiers::queueChargeWindupPresentation(EventRuntimeState &eventRuntimeState,
    const ChargeModifier &charge, const std::array<float, 3> &center)
{
    EventRuntimeState::WorldEffectRequest effect = {};
    effect.id = charge.effect;
    effect.position = {center[0], center[1], center[2]};
    effect.scale = 1.0f;
    eventRuntimeState.worldEffectRequests.push_back(std::move(effect));

    EventRuntimeState::PendingSound sound = {};
    sound.soundScope = SoundScope::World;
    sound.soundName = charge.windupSound;
    sound.fallbackSoundName = charge.windupFallbackSound;
    sound.x = int32_t(std::lround(center[0]));
    sound.y = int32_t(std::lround(center[1]));
    sound.z = int32_t(std::lround(center[2]));
    sound.positional = true;
    sound.hasExplicitZ = true;
    eventRuntimeState.pendingSounds.push_back(std::move(sound));
}

void GameplayActorModifiers::queueChargeHitPresentation(EventRuntimeState &eventRuntimeState,
    const ChargeModifier &charge, const std::array<float, 3> &center)
{
    EventRuntimeState::PendingSound sound = {};
    sound.soundScope = SoundScope::World;
    sound.soundName = charge.hitSound;
    sound.fallbackSoundName = charge.hitFallbackSound;
    sound.x = int32_t(std::lround(center[0]));
    sound.y = int32_t(std::lround(center[1]));
    sound.z = int32_t(std::lround(center[2]));
    sound.positional = true;
    sound.hasExplicitZ = true;
    eventRuntimeState.pendingSounds.push_back(std::move(sound));
}
}
