# OpenYAMM 1.20 and definition-sized inventory stacks

Status: integration and implementation in progress; no new Windows package yet.
Starting fork HEAD b776797b71cc3835ff80bfc762d1b5366458ac2e; installed package minotaur-1.18-fc2c74c256c9-24-1.
The existing uncommitted windows-ci-optimization.md is preserved separately (SHA256
99f2c6467a07c13f3fb07b30d16a7cfae1ea0f33a84cf34511467a43ce6ba6d9). Its intent-to-add marker was temporarily
removed to permit merge; the file and a permanent backup are intact and the marker will be restored.

## A. Upstream selection and merge

Latest stable release verified on 2026-10-31: [v1.20](https://github.com/pjasicek/openyamm/releases/tag/v1.20),
published 2026-10-30T21:18:37Z, full SHA1add1d374848648887c08707199921335b4ba440. It is also current upstream/main.
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

## B–E. Geometry, safe migration, validation and Windows package

Pending. The old code overwrote width/height to1 in ItemTable::applyPotionSettings, ReagentStacks::consolidate,
Character add/place/manual merge and Party pickup. Render/grab/drop functions then fitted icons to one cell.
Quantity will remain independent from definition geometry; normal placement must honor the restored dimensions.
A genuine old1.18 bottle save was captured with the unchanged previous unit binary before rebuilding:
9340 bytes, SHA256882ad46e748624caf4defeed0ba078aef0373218144c81cf10a3e530d3ca4003.
The existing old reagent/Minotaur fixtures are retained. Actual user campaign saves were not supplied.
