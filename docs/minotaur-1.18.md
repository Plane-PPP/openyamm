# Minotaur Rebalance on OpenYAMM 1.18

Work in progress; no Windows package claimed yet. Official v1.18: 4ab07ed519a66feab24599d69bc0ae19971250e2.
Previous package/base: minotaur-1.14-c991da37a2de-20-1. Backup branch: backup/minotaur-before-v1.18-2026-10-29.

## Merge

Official Alchemy (items and hirelings), Mind Resistance and black-potion restrictions retained.
Official Levitate guards retained once; keep our pre-movement sync and queued-environmental-tick protection.
New floorBurn/resistance/FireImmune behavior preserved. Lethal Strike uses the new shield API and native death.
Cybernetics remains ID48; Lethal Strike is49. Saved skills are named; teacher topics updated.
Mod save140 maps to upstream139; old mod132 maps to131. Legacy89/94/95/130 and84/85 paths retained.
Upstream139 omits blood geometry and reconstructs it at map load; keep its original migration.
Original old binary produced frozen tests/fixtures/minotaur-1.14-v132.oysav, verified by the new reader.

## Reagent stacks

Implemented; 21 selected stack/Alchemy cases /1187 assertions passed on Linux. UI compile/visual checks pending.
Only common items typed Reagent stack, up to50 per1x1 inventory cell; quest/unique/artifact items excluded.
Compatibility compares every InventoryItem property except quantity, geometry and grid position.
Automatic grants are transactional on full inventories; compatible manual drops fill50 and leave excess held.
Sort coalesces old singles. Plain inventory click takes all; Shift takes1; Ctrl takes ceil(quantity/2).
Shift wins if both modifiers are held. Existing sort-button modifier shortcuts remain unchanged.
To sell a portion, split it into an empty inventory cell, then sell that portion. Sale/buy values count units.
Inventory, bags, held item and floor/chest/corpse payloads reuse the existing serialized quantity field.
One brewing operation consumes1 reagent, in both cursor orientations and on the bench; planner counts all units.
No reagent bags, potion/bottle/weapon/armour auto-stacking or balance changes introduced.

## Remaining stages

Numeric primary-stat thresholds and Recovery supplements: Pending.
Broader gameplay/save/UI checks, full native compilation and Windows ZIP packaging: Pending.
Manual tests, including the user's actual saves and Android/widescreen rendering, are not claimed.
