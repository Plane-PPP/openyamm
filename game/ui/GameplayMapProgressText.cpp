#include "game/ui/GameplayMapProgressBar.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

// The map progress bar's texts, apart from its drawing so the unit tests need no HUD.
namespace OpenYAMM::Game
{
std::string GameplayMapProgressBar::percentText(double percent)
{
    const double tenths = std::round(std::clamp(percent, 0.0, 100.0) * 10);
    char text[16] = {};

    if (std::fmod(tenths, 10.0) == 0)
    {
        std::snprintf(text, sizeof(text), "%d %%", int(tenths / 10));
    }
    else
    {
        std::snprintf(text, sizeof(text), "%.1f %%", tenths / 10);
    }

    return text;
}

std::string GameplayMapProgressBar::tabText(const RulesetMapProgress &progress, bool percentOnly)
{
    // The HUD fonts are single-byte: the middle dot is 0xB7.
    const std::string dot = " \xB7 ";
    const std::string percent = percentText(progress.percent);

    if (percentOnly)
    {
        return percent;
    }

    if (progress.kind == RulesetMapProgress::Kind::Arena)
    {
        return "Endless Arena" + dot + "Level " + std::to_string(progress.arenaLevel) + dot + percent;
    }

    if (progress.complete)
    {
        return "Cleared" + dot + std::to_string(progress.slain) + " / " + std::to_string(progress.counted);
    }

    return percent + dot + std::to_string(progress.slain) + " / " + std::to_string(progress.needed);
}

std::string GameplayMapProgressBar::popupBody(const RulesetMapProgress &progress)
{
    std::vector<std::string> lines;

    if (progress.kind == RulesetMapProgress::Kind::Arena)
    {
        lines.push_back("Wave level: " + std::to_string(progress.arenaLevel));
        lines.push_back("This wave: " + percentText(progress.percent));
        lines.push_back("The Arena is never completed");
    }
    else
    {
        if (progress.kind == RulesetMapProgress::Kind::DimensionRun)
        {
            lines.push_back("Dimension run: power " + std::to_string(progress.dimensionPower) + ", "
                + std::to_string(progress.dimensionAffixes)
                + (progress.dimensionAffixes == 1 ? " affix" : " affixes"));
        }

        lines.push_back("Slain: " + std::to_string(progress.slain) + " of " + std::to_string(progress.counted));
        lines.push_back("Needed to complete: " + std::to_string(progress.needed) + " ("
            + percentText(progress.requiredShare * 100) + ")");
        std::string alive = "Alive: " + std::to_string(std::max(0, progress.counted - progress.slain));

        if (progress.hidden > 0)
        {
            alive += ", " + std::to_string(progress.hidden) + " hidden, not counted";
        }

        lines.push_back(alive);

        if (progress.revival)
        {
            lines.push_back("Dead rise again on leaving: " + (progress.revivalPassed ? std::string("no longer")
                : "until " + std::to_string(progress.revivalSlain) + " slain"));
        }

        lines.push_back("Reward: " + (progress.rewardPaid ? std::string("paid") : progress.reward));
    }

    std::string body;

    for (const std::string &line : lines)
    {
        body += (body.empty() ? "" : "\n") + line;
    }

    return body;
}
}
