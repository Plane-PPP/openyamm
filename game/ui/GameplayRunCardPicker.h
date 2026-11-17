#pragma once

#include "game/events/RunCardDialog.h"

#include <cstddef>
#include <optional>

namespace OpenYAMM::Game
{
class GameplayScreenRuntime;
struct GameplayInputFrame;

// The card screen of a dungeon run: a level-up's cards side by side with their pictures, in place of the text choice
// box (the approved mockup: docs/research/roguelike-next-prep/card-picker). A 800 x 450 canvas scaled to fit the
// window and centred, drawn with the Carved Obsidian painter like the Talents page of a solo run. The screen only
// marks a card and confirms it: confirming runs the dialog's own EventChoice action of that answer, the same one the
// text box, the debug console's `run pick` and a headless scenario use, so the reward is given in one place.
//
// Mouse: a click marks a card, the button confirms. Keys: 1-5 and the arrows mark, Enter confirms. Esc closes the
// screen and the card stays owed (GameplayOverlayInputController).
class GameplayRunCardPicker
{
public:
    static constexpr float CanvasWidth = 800.0f;
    static constexpr float CanvasHeight = 450.0f;

    // The canvas's rectangle in the window, in pixels, and its scale.
    struct Canvas
    {
        float x = 0.0f;
        float y = 0.0f;
        float scale = 1.0f;
    };

    // A rectangle in canvas units.
    struct Box
    {
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
    };

    static Canvas canvas(int width, int height);
    // Where the card of this place stands among `count` cards, and the confirm button.
    static Box cardBox(size_t index, size_t count);
    static Box confirmBox();
    // The card under a point of the window (pixels), if any; whether the point is on the confirm button.
    static std::optional<size_t> cardAt(const Canvas &canvas, size_t count, float pointerX, float pointerY);
    static bool confirmAt(const Canvas &canvas, float pointerX, float pointerY);

    // What a frame of input does to the screen: marks a card, or returns the card the player confirmed. A key acts
    // once per press, and a key held when the screen opened does nothing until it is released.
    static std::optional<size_t> updateSelection(RunCardDialog &dialog, const GameplayInputFrame &input);

    static void render(GameplayScreenRuntime &context, int width, int height);
    static void handleInput(GameplayScreenRuntime &context, const GameplayInputFrame &input);
};
}
