# OpenYAMM 1.20 and definition-sized inventory stacks

Status: merged and implemented; Linux/Windows automatic tests passed; Windows ZIP packaged; manual retest pending.
Starting fork HEAD b776797b71cc3835ff80bfc762d1b5366458ac2e; installed package minotaur-1.18-fc2c74c256c9-24-1.
The existing uncommitted windows-ci-optimization.md is preserved separately (SHA256
99f2c6467a07c13f3fb07b30d16a7cfae1ea0f33a84cf34511467a43ce6ba6d9). Its intent-to-add marker was temporarily
removed to permit merge; the file and a permanent backup are intact and the marker was restored.

## A. Upstream selection and merge

Latest stable release verified on 2026-10-31: [v1.20](https://github.com/pjasicek/openyamm/releases/tag/v1.20),
published 2026-10-30T21:18:37Z, full SHA 1add1d374848648887c08707199921335b4ba440. It is also current upstream/main.
[Official release CI](https://github.com/pjasicek/openyamm/actions/runs/41124211990) passed Windows/Linux/macOS/Android
builds, platform scenarios and stable publication. Common ancestor is v1.18 4ab07ed519a66feab24599d69bc0ae19971250e2.
Backup branch backup/minotaur-before-v1.20-2026-10-31 retains the original fork HEAD.

New changes include inventory search and MAW bag handling, tagged chest/corpse loot, negative-stat equip checks,
ore smith services, correct history-page numbering and faster save summaries, monster death explosions/chasing,
Wizard Eye visibility, indoor lighting/fog/translucency/movers/ladders, and platform/render/audio fixes.

Eight textual conflicts: game/CMakeLists.txt, GameplayActionController.cpp, SaveGame.cpp, Party.cpp/Party.h,
GameplayHudOverlayRenderer.cpp, GameplayPartyOverlayRenderer.cpp, tests/CMakeLists.txt.
Keep BOTH ReagentStacks and upstream ItemSearch source registrations/includes. Preserve upstream's new search and
multi-bag rendering, adding quantity overlays inside its new common bag-render loop. Retain quantity-aware take API
and pass the ItemTable through upstream's any-bag placement helper. Keep the new native monster-death path and its
explosion processing, rather than reverting world-runtime changes. Combat formulas are taken from the existing fork.
Upstream did not replace our identification fix, bottle/reagent stacking or Minotaur mechanics.

Save changes: upstream142 adds summary-first140, history-numbering141 and outdoor Nearby142. The mod now writes143
with its separate OYMINO1 magic and upstream142 payload plus the existing combat-level field. Explicit old mod140
maps to139, mod132 to131, earlier mappings remain. Both full-load and fast-summary use the same schema mapping;
old mod140 must NOT be mistaken for official summary-first140. Official OYSAVE1 headers remain supported.
History migration uses decoded schema versions, so reloading new saves will not shift history repeatedly.

## B. Definition geometry and native rendering

Root cause: the previous compact policy overwrote dimensions to1 in ItemTable::applyPotionSettings,
ReagentStacks::consolidate, Character add/place/manual merge and Party pickup. Render/grab/drop then aspect-fitted
all eligible items into one cell. This was a deliberate policy in the previous hotfix, now superseded.

Quantity is now independent from definition geometry. All placement, sorting, manual merge, split, transfer,
held-cursor and HUD paths use the original item definition dimensions and native image extent/authored scale.
No source images are changed. Native drawing only adjusts position inside the footprint; it never shrinks a stack.
Full multi-cell occupied rectangles are checked, including the bottom/right edge. Merging into an existing stack
needs no new free rectangle; excess remains on the cursor, and failed automatic insertion is atomic.

The upstream footprint rounding itself under-allocates some HD images. The user explicitly approved retaining
native scale and enlarging only insufficient footprints. The minimum is ceil(native logical extent * authored scale
/32), bounded by the native14x9 grid, and never smaller than the original definition. This applies identically to
singles and stacks, only to ordinary reagent/empty bottle definitions. Quantity is never an input.
[CSV audit](stack-item-geometry.csv):41 base definitions,12 insufficient footprints corrected.
Examples: Gog Blood202=1x2; Wasp Wing217=2x1; empty bottle220=1x2; MM8 Obsidian216=1x1;
MM7 Obsidian1018=2x2 (40x33 logical pixels). Some MM7 tall reagent art requires1x3. Other items retain their geometry.
Prepared potions remain unstackable; old potion instances accidentally saved with compact ingredient dimensions
are restored to their own potion definition, without changing power, effect, price or identification.

## C. Saves and migration

No new quantity fields or borrowed bits. The writer advances to mod143 only for the upstream142 integration
explained above, not for geometry. Existing per-instance quantities and identification flags remain untouched.
Party restore and item-table attachment normalize active members and Adventurers Inn recruits. Held items normalize
when loaded; old grab offsets reset only if dimensions changed. Floor items normalize on pickup. Chest/corpse
snapshots normalize when their world loads; visited maps retain their serialized contents until then.

Valid bag layouts retain coordinates/order. Invalid bags reserve unchanged equipment/quest coordinates first,
then repack alchemy items by native dimensions, without combining or deleting instances. A full14x9 bag can hold
56 vertical1x2 bottle stacks, not63: the ninth row cannot fit another vertical pair. In the126-stack full-inventory fixture,
70 stacks remain in serialized overflow bag255; all6300 bottles survive. Existing world overflow handling puts
items at the party's feet and posts a notice. A failed world drop keeps the item serialized for a later attempt.
Pick these items up before leaving the map. The load itself never writes over the source save.
Chest overflow uses existing serialized hiddenItems and reveals it as space becomes available.
Repeated restoration is idempotent. Inventory-space shortages may require the player to collect overflow.

A genuine old1.18 bottle save was captured with the unchanged previous unit binary before rebuilding:
9340bytes, SHA256 882ad46e748624caf4defeed0ba078aef0373218144c81cf10a3e530d3ca4003.
Genuine production mod132 and previous mod140 reagent fixtures are loaded, summarized, restored, saved as143,
and loaded again. Named skills including LethalStrike, tech points, class, level, XP and quest flags are checked.
Overflow serialization is tested before a world can accept items. Actual user campaign saves were not supplied;
these fixture checks do not establish complete compatibility of every campaign state or backward writing to1.18.
Keep the original saves and install-folder backup; do not open newly written143 saves in the old binary.

## D. Regression and visual scope

Additional save/inventory/chest/corpse selection:165 cases /21,008 assertions passed, partly overlapping the189-case set.
Unrelated large model/portal/terrain map tests are outside this selected run.

Linux protected builds use safe-heavy-run, jobs4, common MemoryHigh16GiB/MemoryMax18GiB/SwapMax1GiB.
Selected regression:189 cases /48,755 assertions passed. Native runtime:7 cases /159 assertions passed.
Coverage includes all previously selected Minotaur/Alchemy/Mind/Levitate cases, old save summary/full load,
new bag/search/stealing behavior, native geometry across quantities/scales, edge/full-inventory merge and splits,
heterogeneous sorting, chest hidden overflow, serialized bag overflow and upstream death explosions exactly once
through Lethal Strike's real terminal paths. The synthetic explosion fixture uses the real comma-separated Special
format; an initial malformed fixture failed before that correction. Missing sparse source assets were restored,
not hidden by weakening expected geometry or skipping requested gameplay cases.

Original Alchemy/item/hireling, black-potion and Mind Resistance fixes remain official once; no new duplicate fix.
Levitate's prior movement/queued-effect guards remain with official surface handling. No balance formula changed.
HP alignment, compact Current/Next stat descriptions, weapon/armor Recovery and naming/identification are retained.
The online updater rejects the nonnumeric minotaur-* channel before downloading official application/packs;
a signed-manifest regression verifies this even with update checks enabled. ZIP patching remains the supported path.

Visual capture preparation on a private old save was attempted with the frozen1.18 game. It stopped first at
missing desktop x4 surfaces, then missing sky/weather definitions; after restoring those, native map loading
requires missing liquid-edge terrain masks. No rendered comparison is claimed. Numeric native
texture/placement assertions are not screenshots. The Windows/user game still needs native-scale visual review,
including last row/right column, Wasp Wing21, Gog Blood7, Obsidian, bottle50, held/split items and neighboring gear
at4:3,16:9 and ultrawide. Full MAW/world/Android visual compatibility is not claimed from focused Linux tests.

## E. Windows release

[Successful Windows package, resources and installation](windows-1.20-build.md).
Run41192833570 built c80ebaa9634d23faa9f389d662fcd03daa482326, package minotaur-1.20-c80ebaa9634d-25-1.
Windows repeated189/48,755 and7/159 successfully. ZIP254.81MiB,47 changed/119 reused packs.
The next-update baseline is advanced only after this success. Metadata records official1.20 and full upstreamSHA.
Existing adaptive jobs4 on4vCPU/16GiB and monitoring are retained; no Ubuntu limits transferred to Windows.

[Local protected-run measurements](openyamm-1.20-local-resources.csv) retain failed setup attempts too.
The full successful native build took380.5seconds, peak4,579,549,184bytes (4.27GiB),swap0,
no cgroup OOM/high/max events or sampled memory pressure. CPU seconds/average busy cores are in the CSV.
No main/upstream writes, force-push, system-limit changes or simultaneous local heavy operations.
