# UI & Inventory Hotfix — successful Windows update

Implemented / Automatically tested on Linux and Windows / ZIP packaged. User campaign and visual retesting pending.

[Successful Windows CI 40912044729](https://github.com/Plane-PPP/openyamm/actions/runs/40912044729).
[Download Minotaur-Update-minotaur-1.18-fc2c74c256c9-24-1.zip](https://github.com/Plane-PPP/openyamm/actions/runs/40912044729/artifacts/12917245741).
ZIP:32,784,239 bytes (31.27MiB), SHA256 `a97ee957d424ef465f757f24240d3926dff49542fce1b69aff8190d5e30acfaa`.
The generated package report and GitHub artifact API agree. CI verified package contents before upload;
the large ZIP was not redownloaded to the VM. Artifact expiry:2026-11-05T17:03:54Z.

## Revisions and files

Previous installed package: `minotaur-1.18-c0ccf0a877bc-23-1`.
Branch before hotfix: `d8e5fe5aede029c5cbcab90d1dfadf28f58e8a69`.
Built source: `fc2c74c256c9cee5c50b7cffb53c9a3e1e28e931`.
New package: `minotaur-1.18-fc2c74c256c9-24-1`.
Official base remains v1.18 `4ab07ed519a66feab24599d69bc0ae19971250e2`; no repeat upstream merge.
Only feature/minotaur-rebalance of Plane-PPP/openyamm was pushed. Main and the author's repository were untouched.

Logical commits:3ce5181 item identification/name consistency;10d5d9c shared icon geometry;
2d303a8 empty bottle stacks/atomic brewing;d927895 compact stat UI/HP layout/CI test selection;
fc2c74c completed inventory regression record. A final documentation/baseline commit follows with [skip ci];
it does not change the binary's source SHA. Pre-existing uncommitted windows-ci-optimization.md is preserved separately.

Changed files (including this release evidence and next-update baseline):

- `.github/workflows/minotaur-windows.yml`
- `BUILD-NOTES.md`
- `CHANGELOG.md`
- `assets_dev/engine/ui/gameplay/character.yml`
- `docs/ROADMAP.md`
- `docs/minotaur-1.18.md`
- `docs/research/ui-inventory-hotfix-local-resources.csv`
- `docs/research/ui-inventory-hotfix.md`
- `docs/research/windows-ui-inventory-hotfix-build.md`
- `docs/research/windows-ui-inventory-hotfix-resources.json`
- `game/gameplay/GameMechanics.cpp`
- `game/gameplay/GameplayItemService.cpp`
- `game/gameplay/GameplayOverlayInputController.cpp`
- `game/gameplay/GameplayPartyOverlayInputController.cpp`
- `game/items/AlchemyBenchRuntime.cpp`
- `game/items/InventoryBags.cpp`
- `game/items/InventoryBags.h`
- `game/items/InventoryItemMixingRuntime.cpp`
- `game/items/InventoryItemMixingRuntime.h`
- `game/items/ItemRuntime.cpp`
- `game/items/ReagentStacks.cpp`
- `game/items/ReagentStacks.h`
- `game/party/Party.cpp`
- `game/tables/ItemTable.cpp`
- `game/tables/ItemTable.h`
- `game/ui/GameplayHudOverlayRenderer.cpp`
- `game/ui/GameplayHudOverlaySupport.cpp`
- `game/ui/GameplayPartyOverlayRenderer.cpp`
- `game/ui/InventoryIconGeometry.h`
- `packaging/minotaur/bases/minotaur-1.18-fc2c74c256c9-24-1.json`
- `packaging/minotaur/latest-baseline.json`
- `tests/BottleStackHotfixTests.cpp`
- `tests/CMakeLists.txt`
- `tests/GameplayUiRuntimeTests.cpp`
- `tests/InspectSupplementTests.cpp`
- `tests/InventoryGeometryHotfixTests.cpp`
- `tests/ItemNamingHotfixTests.cpp`
- `tests/ReagentStackTests.cpp`
- `tests/fixtures/minotaur-1.18-v140-stacks.oysav`
- `tests/fixtures/minotaur-1.18-v140-stacks.txt`

## A–C. History, reagents and potions

The earliest confirmed source revision with the inconsistent title check is upstream
`e2e9601781d682471848494231320c1734a47d8f` (2026-03-29). Pure upstream 1.18 and our pre-stack1.14
retain it; the recent merge/stack feature did not introduce that title branch. This identifies a source revision,
not the first distributed retail/mod release. Original MM8 executable/reference assets were unavailable here,
so original-game runtime parity is not claimed.

Phial of Gog Blood202, Obsidian1018, Heroism229 and Cure Insanity239 all have identifying difficulty 0.
Changing only their historical raw identified bit reproduced Reagent/Layered Potion titles while inspect/Identify
already treated them as known. The title now uses that same requiresIdentification contract. All actual reagent
and prepared-potion definitions are tested in both raw flag states; genuine unknown positive-difficulty items
retain generic titles until identification. No name table, effect, price, recipe or item ID was replaced.

A separate stack comparison compared raw identification even when irrelevant. It now ignores only that irrelevant
bit for items known by the rules; ID, power, stolen/broken/enchantment and other meaningful fields still distinguish
stacks. Thus otherwise equal4 + 3 Gog Blood units combine to 7; different genuine properties remain separate.
Exact internal fields of the user's instances cannot be inferred without their save; the reported IDs reproduce
this cause. No forced identification migration or item mass conversion is used.
[Detailed investigation](ui-inventory-hotfix.md).

## D–E. Geometry, UI and empty bottles

Wasp Wing63x25 logical pixels and Gog Blood30x60 exceeded their32x32 stack cell. Drawing used native icon size
while insertion used1x1. Shared bounded aspect-fit now governs render/grab/drop in inventory, chests and bag views,
including the held icon and quantity inset. Only compact stack items are fitted; other equipment keeps its art size.
Legacy stack shape normalizes through normal merge/move/sort, without rewriting old saved positions on load.

HP's YAML right edge was40 units left of SP/AC/DR. It now shares their right edge and glyph-based right alignment;
no spaces or arithmetic changes. Seven stat descriptions keep original text, current bonuses and next changing
threshold; the long table and trailing lines are removed. Armor/weapon Recovery descriptions remain intact.

The canonical emptyBottleItemId from potion settings derives the bottle stack flag. Normal empty bottles stack 50;
ready potions do not, although their equipment type is also Bottle. Plain click takes all; Shift takes 1;
Ctrl takes half rounded upward. Overflow, manual merging, sorting, bags, transfers and unit pricing use the shared path.

Brewing spends exactly 1 bottle and 1 reagent in either cursor orientation. Candidate inventory placement is atomic:
with two surviving stocks, a separate singleton potion must fit, otherwise both remain unchanged. A newly free cursor
can hold a result that cannot fit. The bench keeps the original target bag where possible, restores remainders first,
and uses its free-cursor overflow path; an occupied cursor blocks bench brewing. New potions use their own dimensions.
Existing serialized quantity persists bottle stacks; no schema/version bump or opaque reused bits/fields.
Frozen pre-stack mod 132 and pre-hotfix mod 140 fixtures load, and new quantities resave unchanged. The reader reports
the decoded upstream 139 payload for a mod 140 envelope. Original campaign-save files were not supplied for direct testing.

## Automatic checks and limitations

Linux and Windows each passed 169 selected unit/regression cases / 39,730 assertions and 4 native world/layout cases
/123 assertions. An additional local inventory/chest/corpse selection passed 89 cases / 6,820 assertions, partly overlapping.
Coverage includes known/unknown names, metadata, old saves, geometry boundaries and scales, bottle/reagent overflow,
splitting/transfers/sorting/pricing, both brewing orientations and repeated/full-inventory bench operations.
Real Stats layout resolves consistently at 640x480,1280x720 and2560x1080. Existing Alchemy item/hireling bonuses,
black-potion limits, Mind Resistance, Levitate, Axe/Recovery, physical resistance and Lethal Strike remain covered.

The first expanded run exposed a closed-MAW-bag result placement regression, fixed before final validation.
Other intermediate failures were corrected new test setup/old removed-table assertions and the target owning the
real UI resolver. No OOM/pressure stop occurred. Windows updater safety/package/monitor jobs also passed.
The focused selection excludes unshipped MM9, Bloodaxe artifact-only tests and the full-map lava shader test;
this is not a full-suite or full-world gameplay certification. Sparse local stale-bake check reports 0;
it does not audit every upstream lighting resource. Changed-document relative links and git diff --check pass.

Manual validation remains: actual campaign saves, HP9/9–1000/1000 screenshots, last-row icons and quantity overlays,
HD4:3/16:9/ultrawide/Android, mouse/touch drag behavior and normal campaign play. No balance changes were made.
Identify Monster/bestiary and teacher flavor/next-teacher directions remain Deferred from earlier work.

## Build resources

Windows compilation was native on GitHub, not cross-compiled on Ubuntu. Existing adaptive policy selected4 jobs
for 4 vCPU /15.995GiB. Job duration34m13s; sampled peak whole-runner RAM7.704GiB, minimum free8.291GiB.

| Phase | Seconds | Peak RAM GiB | Mean CPU | Peak CPU |
| --- | ---: | ---: | ---: | ---: |
| Program/shaders/cooker |720.10|7.704|91.09%|100%|
| Unit/runtime compilation |640.95|7.651|99.07%|100%|
| Gameplay tests |2.45|2.708|25.48%|27.68%|
| Native tests |0.52|2.683|29.55%|29.55%|
| Content packs |99.46|4.221|24.57%|37.50%|
| Program archive |4.57|3.863|17.88%|28.08%|
| Incremental ZIP |2.16|3.440|27.17%|28.46%|

Every monitored command exited 0/completed. Windows PSI, cgroup events and disk-swap peak are unavailable,
not reported as zero. [Machine-readable Windows evidence](windows-ui-inventory-hotfix-resources.json).
Local compilation/tests used safe-heavy-run only, jobs 4,16/18GiB RAM and1GiB swap limits, one operation at a time.
Largest local hotfix build:400.524s,4.750GiB peak,3.761 mean CPU cores; swap0, no high/max/OOM events or
observed task memory PSI. [All protected runs, including intermediate failures](ui-inventory-hotfix-local-resources.csv).
No system/VM limits, CI concurrency or compilation parallelism policy changed.

## Installation

1. Close the game and separately back up the actual save directory.
2. This ZIP updates `minotaur-1.18-c0ccf0a877bc-23-1`; if still on1.14, install the preceding1.18 update first.
3. Extract the whole ZIP with manifest/payload together and open its Minotaur-Update folder.
4. Run `Apply-MinotaurUpdate.cmd -InstallationRoot "D:\path\to\OpenYAMM"`, replacing the path with the root
   containing the ordinary OpenYAMM.exe and app/current. From PowerShell prefix the command with `.\`.
5. Wait for success naming `minotaur-1.18-fc2c74c256c9-24-1`, then launch the usual OpenYAMM.exe.
6. Load a copy of a campaign save, inspect named reagents/potions, sort old singles, test bottle/reagent brewing,
   then save/reload. Check final-row Wasp Wing/Gog Blood and all stat descriptions. To sell part of a stack,
   split the portion into a free cell before selling it.

The ZIP changes 1 pack (10,308,774 bytes), reusing 131 packs (5,767,905,628 bytes,5.37GiB).
The installer preserves saves/settings. No mass migration is required; retain backups for rollback and do not assume
older game binaries support every future saved change. The confirmed next-update baseline is checked in separately.
