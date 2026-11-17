#include "game/ui/GameplayRunCardPicker.h"

#include "engine/Localization.h"
#include "game/events/EventDialogContent.h"
#include "game/gameplay/GameplayInputFrame.h"
#include "game/gameplay/GameplayScreenRuntime.h"
#include "game/party/Party.h"
#include "game/party/SkillData.h"
#include "game/ui/GameplayObsidianPainter.h"

#include <algorithm>
#include <string>
#include <vector>

namespace OpenYAMM::Game
{
namespace
{
using GameplayObsidianColors::abgr;
using Box = GameplayRunCardPicker::Box;
using Canvas = GameplayRunCardPicker::Canvas;

// The layout, in canvas units (the mockup's 1600 x 900 pixels halved).
constexpr float CardsLeft = 30.0f;
constexpr float CardsWidth = GameplayRunCardPicker::CanvasWidth - CardsLeft * 2.0f;
constexpr float CardsTop = 125.0f;
constexpr float CardHeight = 220.0f;
constexpr float CardGap = 10.0f;
constexpr float IconFrame = 58.0f;
constexpr float ConfirmWidth = 170.0f;
constexpr float ConfirmHeight = 26.0f;
constexpr float ConfirmTop = 388.0f;

constexpr uint32_t Ivory = abgr(0xefe0bc);
constexpr uint32_t Gold = abgr(0xf2d27a);
constexpr uint32_t Brass = abgr(0x8c815f);
constexpr uint32_t DimBrass = abgr(0x514535);
constexpr uint32_t Body = abgr(0xc8c7a7);
constexpr uint32_t Muted = abgr(0x9fa594);
constexpr uint32_t Teal = abgr(0x86d2c4);
constexpr uint32_t Backdrop = abgr(0x070b0f, 0xf4);
constexpr uint32_t Panel = abgr(0x0c1418, 0xe6);
constexpr uint32_t CardFill = abgr(0x11181d);
constexpr uint32_t CardHoverFill = abgr(0x1a2328);
constexpr uint32_t CardMarkedFill = abgr(0x2c2719);
constexpr uint32_t IconBack = abgr(0x080e12);
constexpr uint32_t PipOff = abgr(0x323c41);

// The keys the screen reads, in the order of RunCardDialog::keyLatches.
constexpr SDL_Scancode Keys[] = {SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5,
    SDL_SCANCODE_LEFT, SDL_SCANCODE_RIGHT, SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER};
constexpr size_t LeftKey = 5;
constexpr size_t RightKey = 6;
constexpr size_t EnterKey = 7;
constexpr size_t KeypadEnterKey = 8;

bool contains(const Canvas &canvas, const Box &box, float pointerX, float pointerY)
{
    const float x = canvas.x + box.x * canvas.scale;
    const float y = canvas.y + box.y * canvas.scale;
    return pointerX >= x && pointerX < x + box.width * canvas.scale && pointerY >= y
        && pointerY < y + box.height * canvas.scale;
}

// Lines of one paragraph, centred on x, no lower than `bottom`. Returns the y after the last line.
float centredParagraph(const GameplayObsidianPainter &painter, const std::string &font, float height, uint32_t colour,
    const std::string &text, float centreX, float y, float width, float bottom)
{
    for (const std::string &line : painter.wrapped(font, height, text, width))
    {
        if (y + height > bottom)
        {
            break;
        }

        painter.text(font, height, colour, line, centreX, y, 1);
        y += height + 2.5f;
    }

    return y;
}

void drawIcon(GameplayScreenRuntime &context, const GameplayObsidianPainter &painter, const RunCardDisplay &card,
    const GameplayObsidianRect &box)
{
    GameplayUiRuntime &ui = context.gameplayUiRuntime();
    std::optional<GameplayHudTextureHandle> texture =
        card.icon.empty() ? std::nullopt : ui.ensureItemIconTextureLoaded(card.icon);

    if (!texture && !card.icon.empty())
    {
        texture = ui.ensureHudTextureLoaded(card.icon);
    }

    if (texture && texture->width > 0 && texture->height > 0)
    {
        // Fitted whole, keeping its proportions: the weapons' pictures are tall and narrow.
        const float fit = std::min(box.width / float(texture->width), box.height / float(texture->height));
        ui.submitHudTexturedQuad(texture->textureHandle, box.x + (box.width - float(texture->width) * fit) / 2,
            box.y + (box.height - float(texture->height) * fit) / 2, float(texture->width) * fit,
            float(texture->height) * fit);
    }
    else if (card.iconSpell != 0)
    {
        painter.spellIcon(card.iconSpell, box, false);
    }
}

// An arrow pointing right, drawn of rectangles (the fonts need not have the glyph): its left end at (x, y).
void drawArrow(const GameplayObsidianPainter &painter, float x, float y, uint32_t colour)
{
    painter.fill(painter.rect(x, y, 7.0f, 1.0f), colour);
    painter.fill(painter.rect(x + 5.0f, y - 1.0f, 1.0f, 3.0f), colour);
    painter.fill(painter.rect(x + 4.0f, y - 2.0f, 1.0f, 5.0f), colour);
}
}

Canvas GameplayRunCardPicker::canvas(int width, int height)
{
    const float scale = std::max(0.01f, std::min(float(width) / CanvasWidth, float(height) / CanvasHeight));
    return {(float(width) - CanvasWidth * scale) / 2.0f, (float(height) - CanvasHeight * scale) / 2.0f, scale};
}

Box GameplayRunCardPicker::cardBox(size_t index, size_t count)
{
    const float cards = float(std::max<size_t>(count, 1));
    // Fewer than four cards keep the width of four, centred.
    const float width = (CardsWidth - CardGap * (std::max(cards, 4.0f) - 1.0f)) / std::max(cards, 4.0f);
    const float row = width * cards + CardGap * (cards - 1.0f);
    return {CardsLeft + (CardsWidth - row) / 2.0f + float(index) * (width + CardGap), CardsTop, width, CardHeight};
}

Box GameplayRunCardPicker::confirmBox()
{
    return {(CanvasWidth - ConfirmWidth) / 2.0f, ConfirmTop, ConfirmWidth, ConfirmHeight};
}

std::optional<size_t> GameplayRunCardPicker::cardAt(
    const Canvas &canvas, size_t count, float pointerX, float pointerY)
{
    for (size_t index = 0; index < count; ++index)
    {
        if (contains(canvas, cardBox(index, count), pointerX, pointerY))
        {
            return index;
        }
    }

    return std::nullopt;
}

bool GameplayRunCardPicker::confirmAt(const Canvas &canvas, float pointerX, float pointerY)
{
    return contains(canvas, confirmBox(), pointerX, pointerY);
}

std::optional<size_t> GameplayRunCardPicker::updateSelection(RunCardDialog &dialog, const GameplayInputFrame &input)
{
    const size_t count = dialog.cards.size();

    if (count == 0)
    {
        return std::nullopt;
    }

    if (dialog.selected && *dialog.selected >= count)
    {
        dialog.selected.reset();
    }

    // A key acts on the frame it goes down, once.
    std::array<bool, std::size(Keys)> pressed = {};

    for (size_t key = 0; key < std::size(Keys); ++key)
    {
        const bool held = input.isScancodeHeld(Keys[key]);
        pressed[key] = held && !dialog.keyLatches[key];
        dialog.keyLatches[key] = held;
    }

    for (size_t number = 0; number < 5 && number < count; ++number)
    {
        if (pressed[number])
        {
            dialog.selected = number;
        }
    }

    if (pressed[LeftKey])
    {
        dialog.selected = dialog.selected ? (*dialog.selected + count - 1) % count : count - 1;
    }

    if (pressed[RightKey])
    {
        dialog.selected = dialog.selected ? (*dialog.selected + 1) % count : 0;
    }

    const Canvas area = canvas(input.screenWidth, input.screenHeight);
    bool confirmed = (pressed[EnterKey] || pressed[KeypadEnterKey]) && dialog.selected.has_value();

    if (input.leftMouseButton.pressed)
    {
        if (const std::optional<size_t> card = cardAt(area, count, input.pointerX, input.pointerY))
        {
            dialog.selected = card;
        }
        else if (confirmAt(area, input.pointerX, input.pointerY) && dialog.selected.has_value())
        {
            confirmed = true;
        }
    }

    return confirmed ? dialog.selected : std::nullopt;
}

void GameplayRunCardPicker::handleInput(GameplayScreenRuntime &context, const GameplayInputFrame &input)
{
    EventDialogContent &dialog = context.activeEventDialog();

    if (!dialog.runCards)
    {
        return;
    }

    const std::optional<size_t> confirmed = updateSelection(*dialog.runCards, input);

    if (!confirmed)
    {
        return;
    }

    // The same action the text choice runs: the run's own code alone gives the card.
    const auto action = std::find_if(dialog.actions.begin(), dialog.actions.end(),
        [&confirmed](const EventDialogAction &candidate)
        {
            return candidate.kind == EventDialogActionKind::EventChoice && candidate.id == *confirmed;
        });

    if (action != dialog.actions.end())
    {
        const EventDialogAction chosen = *action;
        context.executeDialogAction(chosen);
    }
}

void GameplayRunCardPicker::render(GameplayScreenRuntime &context, int width, int height)
{
    const EventDialogContent &content = context.activeEventDialog();

    if (!content.runCards || content.runCards->cards.empty())
    {
        return;
    }

    const RunCardDialog &dialog = *content.runCards;
    const Party *pParty = context.partyReadOnly();
    const Character *pHero = pParty != nullptr ? pParty->member(dialog.memberIndex) : nullptr;
    const Canvas area = canvas(width, height);
    const GameplayObsidianPainter backdrop(context, {0, 0, float(width), float(height), 1});
    backdrop.fill(backdrop.rect(0, 0, float(width), float(height)), Backdrop);
    const GameplayObsidianPainter painter(
        context, {area.x, area.y, CanvasWidth * area.scale, CanvasHeight * area.scale, area.scale});
    painter.texture("obsidian_skills_surface", painter.rect(0, 0, CanvasWidth, CanvasHeight));
    painter.fill(painter.rect(0, 0, CanvasWidth, CanvasHeight), Panel);
    painter.frame(painter.rect(8, 8, CanvasWidth - 16, CanvasHeight - 16), Brass);
    painter.frame(painter.rect(11, 11, CanvasWidth - 22, CanvasHeight - 22), DimBrass);

    // The header: the hero at the left, the run's place at the right.
    if (pHero != nullptr)
    {
        painter.face(*pHero, painter.rect(32, 18, 46, 58), false);
        const std::string who = Engine::tr("{class} · Level {level}",
            {{"class", Engine::localize(displayClassName(pHero->className), "class")},
                {"level", int64_t(pHero->level)}});
        const float nameWidth =
            painter.text("Lucida", 12, Ivory, painter.fitted("Lucida", 12, pHero->name, 200), 92, 58);
        painter.text("Lucida", 12, Body, painter.fitted("Lucida", 12, "· " + who, 380 - nameWidth),
            92 + nameWidth + 6, 58);
    }

    painter.text("Fondamento", 22, Ivory, Engine::tr("Level up"), 91, 24);
    painter.text("Lucida", 9, Muted,
        Engine::tr("Dungeon {stop} / {stops}", {{"stop", int64_t(dialog.stop)}, {"stops", int64_t(dialog.stops)}}),
        770, 28, 2);
    painter.text("Fondamento", 14, Gold,
        Engine::tr("Card {number} of {total}",
            {{"number", int64_t(dialog.cardNumber)}, {"total", int64_t(dialog.totalCards)}}),
        770, 41, 2);

    // A mark for each hero: the one who chooses now, those still owed a card, the rest.
    const float marks = float(dialog.membersOwed.size());

    for (size_t member = 0; member < dialog.membersOwed.size(); ++member)
    {
        const uint32_t colour = member == dialog.memberIndex ? Gold : dialog.membersOwed[member] ? Brass : PipOff;
        painter.fill(painter.rect(770 - marks * 17 + 3 + float(member) * 17, 63, 14, 2), colour);
    }

    painter.fill(painter.rect(CardsLeft, 88, CardsWidth, 1), DimBrass);
    painter.fill(painter.rect(CanvasWidth / 2 - 2, 86, 5, 5), Brass);
    painter.text("Lucida", 12, Body, Engine::tr("Choose one reward for the hero"), CanvasWidth / 2, 100, 1);

    const GameplayInputFrame *pInput = context.currentGameplayInputFrame();
    const float pointerX = pInput != nullptr ? pInput->pointerX : -1.0f;
    const float pointerY = pInput != nullptr ? pInput->pointerY : -1.0f;
    const size_t count = dialog.cards.size();
    const std::optional<size_t> hovered = cardAt(area, count, pointerX, pointerY);
    // Five cards are narrower: their texts a little smaller.
    const bool narrow = count > 4;

    for (size_t index = 0; index < count; ++index)
    {
        const RunCardDisplay &card = dialog.cards[index];
        const Box box = cardBox(index, count);
        const bool marked = dialog.selected == index;
        const bool hover = hovered == index;
        const float centre = box.x + box.width / 2;
        const float textWidth = box.width - 16;
        painter.fill(painter.rect(box.x, box.y, box.width, box.height),
            marked ? CardMarkedFill : hover ? CardHoverFill : CardFill);
        painter.frame(painter.rect(box.x, box.y, box.width, box.height), marked ? Gold : hover ? Ivory : Brass);
        painter.frame(painter.rect(box.x + 3, box.y + 3, box.width - 6, box.height - 6),
            marked ? abgr(0x79643b) : DimBrass);

        // The key, the kind, the mark of the chosen card.
        painter.frame(painter.rect(box.x + 8, box.y + 8, 13, 13), marked ? Gold : Brass);
        painter.text("Lucida", 9, marked ? Gold : Muted, std::to_string(index + 1), box.x + 14.5f, box.y + 10, 1);
        painter.text("Lucida", 9, Muted, card.kind, centre, box.y + 11, 1);

        if (marked)
        {
            painter.fill(painter.rect(box.x + box.width - 18, box.y + 11, 7, 7), Gold);
        }

        const float iconLeft = centre - IconFrame / 2;
        painter.fill(painter.rect(iconLeft, box.y + 27, IconFrame, IconFrame), IconBack);
        painter.frame(painter.rect(iconLeft, box.y + 27, IconFrame, IconFrame), marked ? Gold : Brass);
        drawIcon(context, painter, card, painter.rect(iconLeft + 4, box.y + 31, IconFrame - 8, IconFrame - 8));

        const float titleHeight = narrow ? 13.0f : 15.0f;
        float y = centredParagraph(painter, "Fondamento", titleHeight, marked ? Gold : Ivory, card.title, centre,
            box.y + 96, textWidth, box.y + 96 + (titleHeight + 2.5f) * 2);
        y = std::max(y + 4, box.y + 130);

        // The step: what the hero has, an arrow, what the card gives.
        const float stepHeight = narrow ? 9.5f : 10.5f;
        const float fromWidth = painter.width("Lucida", stepHeight, card.stepFrom);
        const float toWidth = painter.width("Lucida", stepHeight, card.stepTo);
        const float stepLeft = centre - (fromWidth + toWidth + 19) / 2;
        painter.text("Lucida", stepHeight, Muted, card.stepFrom, stepLeft, y);
        drawArrow(painter, stepLeft + fromWidth + 6, y + stepHeight * 0.55f, Muted);
        painter.text("Lucida", stepHeight, Teal, card.stepTo, stepLeft + fromWidth + 19, y);
        y += stepHeight + 6;

        const float pipsWidth = float(card.pips) * 12 - 3;

        for (int pip = 0; pip < card.pips; ++pip)
        {
            const uint32_t colour = pip < card.pipsBefore ? Brass : pip < card.pipsAfter ? Gold : PipOff;
            painter.fill(painter.rect(centre - pipsWidth / 2 + float(pip) * 12, y, 9, 2.5f), colour);
        }

        y += 9;
        const float effectHeight = narrow ? 9.0f : 10.0f;

        for (const std::string &line : card.effect)
        {
            y = centredParagraph(painter, "Lucida", effectHeight, Body, line, centre, y, textWidth,
                box.y + box.height - 8);
        }
    }

    // The full sentence of the card under the pointer, or of the marked one.
    const std::optional<size_t> shown = hovered ? hovered : dialog.selected;

    if (shown && *shown < count)
    {
        centredParagraph(painter, "Lucida", 10.5f, Body, dialog.cards[*shown].detail, CanvasWidth / 2, 353,
            CardsWidth, 384);
    }

    const Box confirm = confirmBox();
    const bool confirmHover = confirmAt(area, pointerX, pointerY);
    const bool pressed = confirmHover && dialog.selected.has_value() && pInput != nullptr
        && pInput->leftMouseButton.held;
    painter.button(painter.rect(confirm.x, confirm.y, confirm.width, confirm.height), Engine::tr("Confirm choice"),
        false, dialog.selected.has_value(), confirmHover, pressed);
    painter.text("Lucida", 9, Muted,
        Engine::tr("1-{count} or arrows: choose  ·  Enter: confirm  ·  Esc: close", {{"count", int64_t(count)}}),
        CanvasWidth / 2, 424, 1);
}
}
