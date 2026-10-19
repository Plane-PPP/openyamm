# Minotaur v0.4 on OpenYAMM1.5 — compatibility review

Date:2026-10-19. Upstream release1.5 (2026-10-17):
`1414884155362e613977a3444a3c7f81ed4ee047`.
Integration is in feature/minotaur-rebalance; main is not merged or changed.

## Scope

Merge the exact1.5 tag, retain Lethal Strike v0.4 coefficients, Perception synergy,30% cap,
GIANT<60%HP, Axe/DR balance and fixed Stats/teachers (LS-001/LS-002).
Conflicts resolved by retaining both upstream melee feedback/blaster/debug changes and mod mechanics.
No author-supplied teacher flavour text was received: LS-003 remains Planned, not part of this rebase.
Identify Monster and bestiary remain Deferred.

## Save format collision and migration

Old v0.4 wrote OYSAVE1 header/version85 with an outdoor actor effectiveLevel and the old
rain override. Official upstream85 instead removed the rain override and has no actor level.
Official1.5 writes version88 with actor behaviours/explosive mixes. Merely treating every85 as
mod data would silently misalign official saves; treating every85 as upstream breaks mod saves.

The new reader recognizes both known v85 layouts, requires full byte consumption and rejects
ambiguous/truncated input. Old debug rain overrides are discarded as in upstream; battle levels
are preserved from mod85 and reconstructed by the existing bolster pipeline for official old saves.
New mod saves use a separate OYMINO1 header/version89, including1.5 state and the combat level.
Stock OYSAVE1 formats through88, v0.4 mod85 and the v84 fixture are supported.
New saves are not intended for the previous mod executable or stock1.5.

## Evidence

Before merging, the unmodified v0.4 writer produced tests/fixtures/minotaur-v04-v85.oysav.
The production reader loads and resaves it, then reloads while preserving tested class,
LethalStrike15 GM, Perception10, XP, level, gold and cached actor level87.
Separate fixtures were written by the original upstream v85 and v88 writers (see fixture README).
Both load with correct actorID/HP/maxHP, absence of a cached level, and idempotent innate skill migration.
An additional v0.4 progress fixture verifies three successive resaves preserve inventory
positions/quantity, XP, free skill points, gold/bank/food, quest bits, NPC overrides,
reserve character and visited indoor/outdoor map state. Generation details are in fixture README.
All fixtures are synthetic; the user's actual campaign save was not supplied or directly tested.

Linux release executable, unit and runtime targets built (Vulkan off).
25 focused tests/30294 assertions;3 runtime tests/58 assertions;
205 selected regression tests/7329 assertions passed. Runs overlap, not a full-suite claim.
Regression excludes sparse MM6/MM9 asset-dependent cases and fonts; see historical BUILD-NOTES scope.
Lighting stale-bake checker returned0 with the local sparse assets; this is not a full-assets lighting audit.

## Packaging and user migration

Existing Minotaur Windows build is used. Its sparse tools checkout must include1.5 texture cooker
sources; committed cooked assets are packaged with cook-on-build OFF (no source-art recook).
Do not install the old v0.4 hotfix overlay over the new full package: corrected tables/layout already
reside in engine.zip, and the old overlay would hide newer1.5 resources.

Unpack the new full build into a separate folder, keep the previous install and original saves,
copy only saves into the new saves folder, load a copy and save to a new slot.
Do not copy old assets/assets.zip, engine.zip or settings over the1.5 package.
Source save is read-only on load; migration is written only when saving separately.
Windows run/SHA/artifact must be recorded only after actual completion in BUILD-NOTES.

## Windows package validation correction

The first run39287322676 compiled and packaged successfully but failed the old verifier's
mmmerge.zip requirement. The1.5 tag has exactly three world directories:MM6/MM7/MM8;
the removed mmmerge directory was the separate experimental Breach world, not the merged
global gameplay tables (these remain in engine). The package validator now follows1.5,
checks PE x64 and compares embedded Stats/NPC resources against the source checkout.
The synthetic MM8 progress fixture also passes content-package/item validation without mmmerge.
This does not claim compatibility with removed experimental maps or unprovided custom content.
