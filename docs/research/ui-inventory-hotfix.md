# UI & inventory hotfix on OpenYAMM 1.18

Status: implementation in progress. Base package minotaur-1.18-c0ccf0a877bc-23-1; branch head before work d8e5fe5.
Existing uncommitted docs/research/windows-ci-optimization.md belongs to earlier work and is left untouched.

## Naming investigation (before bottle/geometry changes)

The wrong generic title is reproducible with the SAME Item ID, data and power, changing only raw identified=false.
Rows202 Phial of Gog Blood,1018 Obsidian,229 Heroism and239 Cure Insanity have IdRepSt=0 and distinct
unidentified aliases Reagent/Layered Potion. Their identifying difficulty is zero. The existing generator treats
such items as known; raw map records may still carry a cleared identification bit, and saves preserve that bit.
The inspect body and Identify action use requiresIdentification; the old title used only raw identified.
Consequently a zero-difficulty item could show its whole effect while permanently retaining a generic title,
with Identify reporting Already identified. This is a presentation/interpretation inconsistency, not a wrong ID,
name table, effect, value or save-layout change.

Git blame locates the raw-flag-only title branch in upstream e2e9601781d682471848494231320c1734a47d8f,
2026-03-29. That same revision already has the zero-difficulty requiresIdentification rule. Clean upstream1.18
has the same logic; our1.14 and stack implementation did not create it. A native MM8 executable/reference checkout
is not present in this sparse workspace, so original-game execution/binary validation is not claimed.
Original source-data fields and the existing engine identification contract are preserved; genuine positive-
difficulty unknown items still show their generic alias until identified. No forced global renaming/migration.
The MMExtension reference separately documents item Number, Identified and GetName/GetIdentifiedName:
https://grayface.github.io/mm/ext/ref/#structs.Item . It does not prove native behavior for zero-difficulty items.

A SECOND cause prevented merging: ReagentStacks compared the full raw identified bit even where identification
cannot matter. Compatibility now ignores ONLY this irrelevant bit for zero-difficulty/already-known-by-rules items.
Meaningful unknown/known states, Item ID, power, stolen/broken flags, enchantments and all other properties remain
part of comparison. Exact user instances are unavailable; the cited IDs reproduce the reported symptom, but other
metadata differences in an actual user save cannot be inferred from screenshots alone.

Names/compatibility/save tests and existing reagent tests:14 cases /1406 assertions passed locally.
Frozen pre-hotfix1.18 writer fixture tests/fixtures/minotaur-1.18-v140-stacks.oysav was captured from the unchanged
old unit binary; SHA2561e6b1c8787e63f9364d889b96c807d8c7ce7a8f94356509cd392082eb1f73ebc,11066bytes.
Includes125 reagents split50/50/25 and named Lethal Strike. Existing v132 fixture is retained.

## Confirmed geometry and UI causes

Stack insertion forces1x1 logical cells, but renderer/input geometry used the original icon extent without fitting
it to those cells. Oversized reagent icons therefore extend over neighboring slots and the last inventory row.
Fix the shared aspect-preserving geometry only for stackable items, including held/dropped pointer geometry;
do not shrink all equipment/icons. Normalize legacy stack geometry when merging/moving/sorting.
HitPointsValue has x115.208333,width85, whereas SP/AC end at240.208333; HP ends40 units earlier. Correct the
layout's common right edge, not string padding. Threshold table removal preserves real current/next bonuses.

Bottle stacks, final tests, Windows packaging and manual verification: pending.

## Geometry implementation

Actual icons_x2 PNG dimensions: Wasp Wing126x50 (logical63x25), Gog Blood60x120 (30x60),
Obsidian80x66 (40x33), empty bottle64x120 (32x60). Inventory cells are32x32 logical.
A shared aspect-fit/bounded rectangle function now serves drawing, grab offsets and drop placement, including
MAW bag overview, chest and nested/shop inventory. Compact stack items alone fit a single cell; non-stack art
retains native extents. Counts inset from the edge and also appear in nested inventories.
New reagent definitions and merge/pickup geometry normalize to1x1; old save positions/counts are not mass-rewritten.
Full native game and unit targets compiled; focused geometry/name/stack checks completed locally.
