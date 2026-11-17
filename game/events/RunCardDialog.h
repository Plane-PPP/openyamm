#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace OpenYAMM::Game
{
// What the card screen of a dungeon run shows (game/ui/GameplayRunCardPicker.h): the offered cards as texts and
// pictures, already in the interface language. Presentation only and never saved: the cards themselves, the deal and
// the reward stay with the run's rules (RoguelikeRules.h), and a loaded run deals the same cards again.
struct RunCardDisplay
{
    // The caption above the picture: Skill, Talent or Books.
    std::string kind;
    std::string title;
    // The step the card makes, as two texts around an arrow ("Expert", "Master"; "Rank 1", "2 / 3").
    std::string stepFrom;
    std::string stepTo;
    // The pips under the step: how many there are, how many the hero has and how many the card leaves them with.
    int pips = 0;
    int pipsBefore = 0;
    int pipsAfter = 0;
    // The card's effect in a few words (one or two lines), and the full sentence shown under the cards.
    std::vector<std::string> effect;
    std::string detail;
    // The picture: an item icon, a HUD texture, or a spell's own icon when neither loads (0: none).
    std::string icon;
    uint32_t iconSpell = 0;
};

struct RunCardDialog
{
    size_t memberIndex = 0;
    // The hero's card this is and how many a run gives; the dungeon the run is in and how many it has.
    int cardNumber = 1;
    int totalCards = 1;
    int stop = 1;
    int stops = 1;
    // The party's heroes who are still owed a card (the header's marks).
    std::vector<bool> membersOwed;
    std::vector<RunCardDisplay> cards;
    // The card the player marked; only a marked card can be confirmed.
    std::optional<size_t> selected;
    // Keys held when the screen opened or since their last use: a key acts once per press, so the Enter that
    // confirmed one hero's card does not confirm the next hero's.
    std::array<bool, 9> keyLatches = {true, true, true, true, true, true, true, true, true};
};
}
