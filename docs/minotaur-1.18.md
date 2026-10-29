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

Implemented; 21 initial stack/Alchemy cases /1187 assertions passed on Linux, followed by the expanded checks below. UI compiled; visual checks pending.
Only common items typed Reagent stack, up to50 per1x1 inventory cell; quest/unique/artifact items excluded.
Compatibility compares every InventoryItem property except quantity, geometry and grid position.
Automatic grants are transactional on full inventories; compatible manual drops fill50 and leave excess held.
Sort coalesces old singles. Plain inventory click takes all; Shift takes1; Ctrl takes ceil(quantity/2).
Shift wins if both modifiers are held. Existing sort-button modifier shortcuts remain unchanged.
To sell a portion, split it into an empty inventory cell, then sell that portion. Sale/buy values count units.
Inventory, bags, held item and floor/chest/corpse payloads reuse the existing serialized quantity field.
One brewing operation consumes1 reagent, in both cursor orientations and on the bench; planner counts all units.
No reagent bags, potion/bottle/weapon/armour auto-stacking or balance changes introduced.

## Numeric inspection supplements

Implemented; original descriptions/enchantments retained. Standard's seven primary-stat descriptions append
current effective value, actual effect, next tier and every meaningful threshold (29 entries in three columns).
The threshold API reads the combat engine's original parameter table. HP/SP contributions call the same class
resource functions on copies, relative to the zero-modifier tier, respecting the class's mana mode.
Speed shows its Recovery adjustment and AC modifier. Luck explains the resistance roll (not flat mitigation)
and, where learned, the existing Lethal Strike Luck contribution. No combat formula changed.

Armor/Shield show their isolated additive Standard Recovery penalty with current mastery, including zero.
The helper is also used by the real attack calculation; Speed, Haste and other equipment do not contaminate it.
Equipped weapons show actual final Recovery for the whole attack, so both dual-wielded weapons agree.
Unequipped weapons use the normal equip plan on a Party copy; the label identifies an equip preview.
If equip validation fails (including space/restrictions), show explicitly labeled base weapon Recovery instead.
The preview chooses the ordinary default equip slot; it does not promise an off-hand arrangement the player
has not chosen. It never mutates the real Party. Ruleset combat overrides (MAW) take precedence.
MAW keeps its own existing stat/armor descriptions rather than showing Standard's incompatible thresholds
or pretending its multiplicative armor impairment is an additive tick penalty.

Long description bodies are clipped to the available viewport and scroll by mouse wheel or touch drag.
English/Russian supplements use the mod's existing minotaur_russian_text selection; original localized texts
are untouched. Actual 4:3/16:9/ultrawide/Android visual validation remains manual.

## Validation in progress

152 selected gameplay cases /35,969 assertions passed, including nine inspection cases and MAW bags/potions.
Full native executable compiled successfully (protected run 20261029T083305Z-2419c36760, peak4343MiB).
Final native/runtime/unit build passed. Three genuine indoor/outdoor/new-game Lethal Strike cases /58 assertions passed.
Windows ZIP packaging: Pending.
Manual tests, including the user's actual saves and Android/widescreen rendering, are not claimed.
