# UI & inventory hotfix on OpenYAMM 1.18

Status: implemented and automatically tested locally; Windows packaging pending. Base package minotaur-1.18-c0ccf0a877bc-23-1; branch head before work d8e5fe5.
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

Bottle stacks and compact stat UI are implemented below. Windows packaging and manual verification: pending.

## Geometry implementation

Actual icons_x2 PNG dimensions: Wasp Wing126x50 (logical63x25), Gog Blood60x120 (30x60),
Obsidian80x66 (40x33), empty bottle64x120 (32x60). Inventory cells are32x32 logical.
A shared aspect-fit/bounded rectangle function now serves drawing, grab offsets and drop placement, including
MAW bag overview, chest and nested/shop inventory. Compact stack items alone fit a single cell; non-stack art
retains native extents. Counts inset from the edge and also appear in nested inventories.
New reagent definitions and merge/pickup geometry normalize to1x1; old save positions/counts are not mass-rewritten.
Full native game and unit targets compiled; focused geometry/name/stack checks completed locally.

## Bottle stacks and transactional brewing

Implemented, automatically tested locally: the canonical emptyBottleItemId from merged potion settings derives a
runtime-only ItemDefinition flag. Only that ordinary Bottle participates in the existing50-unit stack policy;
prepared potions also use the Bottle equipment type, so that type alone is deliberately insufficient.
Existing serialized InventoryItem.quantity persists bottles without a save version/field change.
Plain click takes all, Shift1, Ctrl half upward, including the MAW bag overview. Sorting and stock/trade operations
reuse the shared stack and per-unit pricing paths. Identification compatibility keeps the rules described above.

Cursor brewing plans on copies, consumes exactly1 of each input, then commits the complete candidate inventory.
If both remainders occupy the original inventory cell and cursor, a separate singleton potion must fit elsewhere.
If it cannot, the operation refuses without consuming either input. When both inputs end, a too-tall potion may
use the newly free cursor. New potions use their own definition dimensions, not the ingredient's1x1 geometry.
The bench restores input remainders before placing a produced potion, returning an overflow product through the
existing cursor path. Both bench entry points refuse an occupied cursor, preventing replacement of a held item.
No potion powers, mastery restrictions or Alchemy bonus formula changed.

## Compact stat tooltips and HP layout

Implemented, automatically tested locally: original descriptions, current/formula values, actual effects and next
changing effect remain; the full threshold table and its trailing lines are removed in both languages.
The threshold API remains shared with combat. Recovery item supplements remain intact.
HP now uses the same240.208333 right edge and right text alignment as SP, AC and Damage Resistance.
This is a layout constraint correction, independent of digit count; no padding spaces or HP arithmetic change.

## Compatibility and manual validation

Read compatibility is checked with genuine frozen pre-stack mod132 and pre-hotfix stack-enabled mod140 fixtures.
No mass identification migration or inventory position rewrite is performed. Back up campaign saves before testing;
loading a user campaign save and visual checks on4:3,16:9,ultrawide and Android remain manual until actually run.
Check last-row/right-edge Wasp Wing, Gog Blood and bottles; all seven tooltip descriptions; HP9/9,129/129,1000/1000;
stacked bottle/reagent brewing, full inventory refusal and save/reload. The actual user save was not supplied,
and original MM8 execution was unavailable, so neither is represented as verified.

## Final local validation

169 selected unit/regression cases /39,730 assertions passed, covering all new features plus Alchemy item/hireling
bonuses, once-per-character potions, Mind Resistance, Levitate, Axe, physical resistance and Lethal Strike.
4 native runtime cases /123 assertions passed: actor death/XP/loot/creation and actual Stats layout resolution at
640x480,1280x720,2560x1080. Right alignment shares one measured edge; digit-count screenshots remain manual.
The first expanded run exposed a closed-MAW-bag result regression, fixed by preferring the original target bag.
New test setup was corrected for both visual-scale columns and the reader's decoded upstream139 version
inside a mod140 envelope. The HP test lives in the native runtime target, which owns the real layout resolver.
The existing genuine old-writer fixtures remain intact. No save schema change or Minotaur balance change.
