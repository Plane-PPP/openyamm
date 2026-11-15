# Party creation attribute tooltips — upstream verification pending

**Status: not confirmed as a reproduced upstream bug. Do not post this as a verified upstream defect.**

The player tested a Minotaur/Medusa/Roguelike Windows 11 build based on OpenYAMM 2.3.
They report that party creation tooltips show racial point conversion, but do not explain what an
attribute does. The in-game character screen provides that explanation.

## Source evidence

Local upstream source inspected: `f4a6b8032759a218505532b2600ac56b6d80c7af` (2.3 base).
Mod source inspected: `da1bb1624580ceb7caa3626d637c020727f5aa46`.

In both, `game/ui/screens/NewGameScreenPartySheet.cpp::drawCreationAttributes` reads
`characterInspectTable().getStat(...)->description` and appends a racial-cost line.
`drawCreationHelp` wraps and renders the body as well as extra lines. The source already attempts
to display a description; there is no evidence that it simply discards the body.

In the mod, `game/ui/GameplayHudOverlaySupport.cpp`, lines 804–830, additionally calls
`rules.skills.statTooltip`, whose default implementation in `game/rules/SkillRules.cpp::statTooltip`
adds derived mechanical information. Party creation does not invoke this formatter.
This is a confirmed difference between code paths, not proof of the reported missing-description symptom
in an unmodified upstream installation.

Needed before confirmation: a clean official 2.3 Windows installation, exact package/build ID,
language and resolution/DPI, screenshots of the same attribute in creation and gameplay,
and inspection of the loaded description data. Repeat with English and the user's selected language.
Neither a clean upstream run nor the reported tooltip screenshot was available in this investigation.

## Short Discord draft — explicitly unverified

**Possible party-creation attribute tooltip inconsistency (needs a clean-upstream check)**

Windows 11, a modded build based on OpenYAMM 2.3. I have not reproduced this in the official build yet.

Steps:
1. Start a new game and open party creation.
2. Hold RMB over an attribute name, such as Might.
3. Compare its help with the same attribute on the in-game character Stats screen.

Expected: explain the attribute's gameplay effects, along with racial point costs/bonuses.

Reported result in the modded build: only racial point conversion is visible; gameplay Stats provides
the mechanical explanation.

Source note: creation already reads the base stat description, while gameplay additionally uses
the rules' statTooltip formatter. Could someone check the official 2.3 build before treating this as
an upstream issue? Build ID, language and comparison screenshots are still needed.

## After verification

If reproduced in official OpenYAMM, replace the qualification with the exact official version,
attach comparison screenshots and record language/DPI. If not reproduced, keep investigation
in the mod: loaded packs, translated stats data and preview-tooltip integration.
No Discord message was sent by this task.
