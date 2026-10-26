#include "game/gameplay/GameplaySpeechRules.h"

#include "game/gameplay/GameMechanics.h"
#include "game/party/Party.h"

namespace OpenYAMM::Game
{
namespace
{
bool bypassSpeechCooldown(SpeechId speechId)
{
    switch (speechId)
    {
        case SpeechId::DamageMinor:
        case SpeechId::DamageMajor:
        case SpeechId::IdentifyWeakItem:
        case SpeechId::IdentifyGreatItem:
        case SpeechId::IdentifyFailItem:
        case SpeechId::RepairSuccess:
        case SpeechId::RepairFail:
        case SpeechId::LeaveDungeon:
        case SpeechId::EnterDungeon:
            return true;

        default:
            return false;
    }
}
}

bool canMemberSpeak(const Character &member, SpeechId speechId)
{
    switch (speechId)
    {
        case SpeechId::Dying:
        case SpeechId::CheatedDeath:
            return true;

        case SpeechId::Awaken:
            return !member.conditions.test(size_t(CharacterCondition::Dead))
                && !member.conditions.test(size_t(CharacterCondition::Petrified))
                && !member.conditions.test(size_t(CharacterCondition::Eradicated));

        default:
            return GameMechanics::canAct(member);
    }
}

bool canPlaySpeechReactionForPresentationState(
    const GameplayPortraitPresentationState &presentationState,
    size_t memberIndex,
    SpeechId speechId,
    uint32_t nowTicks)
{
    if (memberIndex >= presentationState.memberSpeechCooldownUntilTicks.size())
    {
        return true;
    }

    if (bypassSpeechCooldown(speechId))
    {
        return true;
    }

    if (nowTicks < presentationState.memberSpeechCooldownUntilTicks[memberIndex])
    {
        return false;
    }

    switch (speechId)
    {
        case SpeechId::KillWeakEnemy:
        case SpeechId::KillStrongEnemy:
            return true;

        case SpeechId::AttackHit:
        case SpeechId::AttackMiss:
        case SpeechId::Shoot:
        case SpeechId::CastSpell:
        case SpeechId::DamagedParty:
            return nowTicks >= presentationState.memberCombatSpeechCooldownUntilTicks[memberIndex];

        default:
            return true;
    }
}
}
