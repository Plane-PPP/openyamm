#pragma once

namespace OpenYAMM::Game
{
// MAW's experience curve (devdocs/maw/data/exp_curve.yml): the vanilla 1000 x L per level to 100, then steeper; every
// MAW conversion between experience and level goes through it. Levels are fractional inside a step.
double mawExperienceForLevel(double level);
double mawLevelForExperience(double experience);

// Climbs `levels` vanilla levels' worth of experience (500 L (L - 1)) on top of the MAW curve at `floorLevel`, so above
// level 100 a monster family gains fewer levels than it is offset by (zzMaw-Monsters.lua bolsteredLevel).
double mawBolsteredLevel(double floorLevel, double levels);
}
