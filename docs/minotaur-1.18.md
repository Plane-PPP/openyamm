# Minotaur Rebalance on OpenYAMM 1.18

Implemented / Automatically tested on Linux and Windows / ZIP packaged; manual gameplay pending. Official v1.18: 4ab07ed519a66feab24599d69bc0ae19971250e2.
Original1.18 package base: minotaur-1.14-c991da37a2de-20-1.
Current [UI/inventory hotfix ZIP](research/windows-ui-inventory-hotfix-build.md) updates minotaur-1.18-c0ccf0a877bc-23-1
to minotaur-1.18-fc2c74c256c9-24-1;169 unit and 4 native cases passed on Linux and Windows. Backup branch: backup/minotaur-before-v1.18-2026-10-29.

## Merge

Official Alchemy (items and hirelings), Mind Resistance and black-potion restrictions retained from
[4ab07ed](https://github.com/pjasicek/openyamm/commit/4ab07ed519a66feab24599d69bc0ae19971250e2).
Our local7450b20 Alchemy helper and 46e71d0 Mind fix are superseded by the official equivalent logic;
there is one effective-level addition and one Mind duration case. Original learned skill/mastery stay unchanged.
Levitate upstream fix is [8a6d10f](https://github.com/pjasicek/openyamm/commit/8a6d10f9d21a2e8ac8601a63b7efbfeb9786bc44);
our b1907da contains additional first-update and queued-hazard protections, so those guards remain.
Official Levitate guards retained once; keep our pre-movement sync and queued-environmental-tick protection.
New floorBurn/resistance/FireImmune behavior preserved. Lethal Strike uses the new shield API and native death.
Cybernetics remains ID48; Lethal Strike is49. Saved skills are named; teacher topics updated.
Mod save140 maps to upstream 139; old mod 132 maps to131. Legacy89/94/95/130 and84/85 paths retained.
Upstream139 omits blood geometry and reconstructs it at map load; keep its original migration.
A compact-save regression verifies that 2,400 baked vertices do not increase the file size, while splat metadata survives.
Original old binary produced frozen tests/fixtures/minotaur-1.14-v132.oysav, verified by the new reader.

## Reagent stacks

Implemented; 21 initial stack/Alchemy cases /1187 assertions passed on Linux, followed by the expanded checks below. UI compiled; visual checks pending.
Common items typed Reagent and the canonical empty alchemy bottle stack, up to 50 per1x1 cell;
quest/unique/artifact items excluded. Bottle identity comes from potion settings, not a localized name.
Compatibility compares every InventoryItem property except quantity, geometry and grid position;
raw identification flags are irrelevant only for items already known by the identification rules.
Automatic grants are transactional on full inventories; compatible manual drops fill50 and leave excess held.
Sort coalesces old singles. Plain inventory click takes all; Shift takes 1; Ctrl takes ceil(quantity/2).
Shift wins if both modifiers are held. Existing sort-button modifier shortcuts remain unchanged.
To sell a portion, split it into an empty inventory cell, then sell that portion. Sale/buy values count units.
Inventory, bags, held item and floor/chest/corpse payloads reuse the existing serialized quantity field.
One brewing operation consumes 1 reagent and1 bottle, in both cursor orientations and on the bench.
Remaining stocks survive; direct cursor brewing refuses atomically if its result cannot fit. The bench returns
an overflow potion through its existing free-cursor path and refuses to brew while the cursor is occupied.
Prepared potions, weapons and armour do not stack. No reagent bags or balance changes introduced.

## Numeric inspection supplements

Implemented; original descriptions/enchantments retained. Standard's seven primary-stat descriptions append
current effective value, actual effect and the next threshold that changes the effect.
The UI hotfix removes the long threshold table; original descriptions and real bonus calculations remain.
The threshold API reads the combat engine's original parameter table. Current values come from the actual
CharacterSheetSummary, avoiding the legacy raw-stat API that would double-count worn bonuses. HP/SP contributions call the same class
resource functions on copies, relative to the zero-modifier tier, respecting the class's mana mode.
Speed shows its Recovery adjustment and AC modifier. Luck explains the resistance roll (not flat mitigation)
and, where learned, the existing Lethal Strike Luck contribution. No combat formula changed.

Armor/Shield show their isolated additive Standard Recovery penalty with current mastery, including zero.
The helper is also used by the real attack calculation; Speed, Haste and other equipment do not contaminate it.
Equipped weapons show actual final Recovery for the whole attack, so both dual-wielded weapons agree.
A usable main-hand blaster/wand keeps its normal attack priority, including when inspecting the other hand.
Unequipped weapons use the normal equip plan on a Party copy; the label identifies an equip preview.
If equip validation fails (including space/restrictions), show explicitly labeled base weapon Recovery instead.
The preview applies the inspecting character’s doll/type equipment restrictions and includes wands’ spell Recovery.
It chooses the ordinary default equip slot; it does not promise an off-hand arrangement the player
has not chosen. It never mutates the real Party. Ruleset combat overrides (MAW) take precedence.
MAW keeps its own existing stat/armor descriptions rather than showing Standard's incompatible thresholds
or pretending its multiplicative armor impairment is an additive tick penalty.

Long description bodies are clipped to the available viewport and scroll by mouse wheel or touch drag.
English/Russian supplements use the mod's existing minotaur_russian_text selection; original localized texts
are untouched. Actual 4:3/16:9/ultrawide/Android visual validation remains manual.

## Automatic validation

156 selected gameplay cases /36,020 assertions passed, including twelve inspection cases and MAW bags/potions.
Full native executable compiled successfully (protected run 20261029T083305Z-2419c36760, peak4343MiB).
Final native/runtime/unit build passed. Three genuine indoor/outdoor/new-game Lethal Strike cases /58 assertions passed.
Windows: the same 156 cases /36,020 assertions and3 native cases /58 assertions passed.
[Successful run and package report](research/windows-1.18-build.md).
[Download 137MiB ZIP](https://github.com/Plane-PPP/openyamm/actions/runs/40863165794/artifacts/12894798568).
Version minotaur-1.18-c0ccf0a877bc-23-1;9 changed packs,123 reused (5,657,002,470 bytes).
A separate local inventory/loot selection passed83 cases /4,076 assertions (overlaps the main selection).
Manual tests, including the user's actual saves and Android/widescreen rendering, are not claimed.

## Installation and manual checks

The published incremental ZIP updates installed minotaur-1.14-c991da37a2de-20-1.
Back up the actual save directory before the first launch; retain the old files separately for rollback.
Extract the whole update ZIP, not its inner payload alone. Run Apply-MinotaurUpdate.cmd with
`-InstallationRoot "D:\path\to\OpenYAMM"` pointing at the directory containing `app/current`, then launch the
usual OpenYAMM.exe. The installer validates baseline/packs, writes a versioned program directory and switches
the pointer only after validation; unchanged heavy packs are reused. It does not migrate saves itself.
The new game loads old saves and writes mod 140; do not open new saves in the old1.14 executable.

Manual checklist: load a copy of an actual1.14 campaign save, verify party/equipment/skills and save/reload.
Sort old reagent singles; merge49+3, split1/half, transfer between characters, brew repeatedly and sell a split
portion. Check counts after dropping/picking up, chests/corpses and saving. Compare Recovery on both dual weapons,
then change Axe mastery/Speed/Haste/armor. Check preserved descriptions, tier tables, wheel/touch scrolling and
layout at4:3/16:9/ultrawide; Android native visual checks remain unperformed. Retest Lethal Strike notifications,
GIANT threshold, Axe combinations, physical resistance, and retained teacher services.
Teacher flavor/next-teacher dialogue, Identify Monster and expanded bestiary remain Deferred.

## Local resource evidence

[Recorded runs](research/v1.18-local-resources.csv): all compilation/tests under safe-heavy-run, jobs 4,
16/18GiB RAM and1GiB swap budget. No limit events, OOM, swap use or measurable memory PSI.
The largest task peak (5,619MiB) occurred in an otherwise successful native build followed by a stale runtime-test
constructor compile error; that test API was corrected, not a memory retry. Final native/runtime/unit build passed.
Linux packaging tests:20 discovered,10 passed and10 Windows-only skipped; actual PowerShell tests run on Windows CI.
Sparse local stale-bake check returned0; this is not a complete downloaded-assets lighting audit.
