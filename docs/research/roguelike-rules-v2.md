# Roguelike rules v2 on Minotaur / OpenYAMM 2.7

Status: built 2026-11-17/18 on `integration/minotaur-v2.7-20261117` (OpenYAMM 2.7 merged onto patch #51, `51ec30f4`),
checked headless on Linux. What v1 was: [roguelike-rules-v1.md](roguelike-rules-v1.md). The pool and its stages come
from the research [roguelike-dungeon-pool-mm6-mm9.md](roguelike-dungeon-pool-mm6-mm9.md) and the user's decisions of
2026-11-17 recorded there (section 11.1). Every number in `run.yml` is a test value.

## What changed from v1

| Area | v1 (2.4, patch #51) | v2 (2.7) |
| --- | --- | --- |
| Dungeons | 8 MM6 maps, `tier` 1-3; route `[1, 2, 3, 3]`: Goblinwatch always first, stops 3 and 4 alike | 94 maps of MM6 (27), MM7 (31), MM8 (22) and MM9 (14), each with the `stages` it may stand at; route `[1, 2, 3, 4]`: every stop random |
| Route | recomputed from the seed at every move | drawn once when the run starts and kept in the save (`run.route.count`, `run.route.<n>`) |
| Guardian | the map's strongest kind, hit points x4 | x4, or x2 when that kind stood alone on the map at its first fill (`uniqueGuardianHealthScale: 2`) |
| Stop's clearing | every monster dead on the map counted as slain | monsters already dead when the party arrived (corpses of a first fill) are not the stop's |
| A map without monsters | the way out stayed sealed for good | after 3 s with nothing hostile to clear the stop's level-ups come and the way opens |
| Camp alchemy shop | herbs and MM6's bottle row 1765, which brews nothing | herbs and the table's empty alchemy bottle (220); a 2.4 save's 1765 bottles become 220 on load |
| Piercing Arrows card | arrows fly on through 1/2/3 monsters | OpenYAMM 2.7 removed the piercing flight: the card (same id) now leaps on to the nearest monster within 10 m, 1/2/3 times, 50 % damage ("Chaining Arrows") |
| `run buy` (debug) | "Bought 3 of item ..." (the stack's size) | "Bought 1 of item ... (3 on the shelf) ..." (a shop sells one of a stack at a time) |

Group sizes stay halved for testing (the user, 2026-11-17): `groupScale: 0.5` on the 19 maps of about 145 monsters and
more (the five of v1 plus 6d04, 6t1, sewer, beethoven, 6d06, 6d10, 6d11, 6t7, 7d35, 6d17, 6t6, cd1, cd2,
chasmofthedead). Back to the original groups after the tests.

## The pool by stage

Stage 1 (heroes 1 → 13): 6d01, 6d02, 7d06, 7d22, mdk01, mdk02, mdr02, d05, anskramkeep.

Stage 2 (13 → 25): 6d02, 6d03, 6d04, 6t1, 6t2, 6t3, sewer, 7d17, 7d21, 7d22, 7d24, 7d37, d01, mdk01-05, mdr02-04,
d07, d08, d13, d45, d46, anskramkeep, beethoven, drangheimprison, thjoradmine.

Stage 3 (25 → 33, promoted): 6d03, 6d06, 6d07, 6d09, 6d10, 6d11, 6d12, 6d13, 6d14, 6d15, 6t5, 6t7, 7d08, 7d16, 7d17,
7d19, 7d24, 7d31, 7d32, 7d35, 7d37, t03, t04, d11, d12, d13, d18, d22, d25, d41, d44, d46, tasaracademy, thjoradmine.

Stage 4 (33 → 41): 6d09, 6d12, 6d13, 6d15, 6d16, 6d17, 6d19, 6t4, 6t5, 6t6, 6t7, cd1, cd2, zdwj02, 7d11, 7d12, 7d14,
7d15, 7d16, 7d20, 7d33, d03, d04, d12, d16, d20, d21, d24, d25, d30, d31, d33, d40, d41, d43, chasmofthedead,
dookscastle, inventastorca, klusoshouse, ruinedtemple, traininghall, verhoffinruins, wizardslab, yanmirsfort.

Left out (the research's classes and the user's decisions): class C (a part of the map needs a key or a quest of
another map: 6d05, 6d18, cd3, 7d13, 7d23, 7d27, 7d34, d14, d19, d26, d34, darkpassageway, mdt10, mdt12), class D
(towns, the player's castle, the Hive, maps with no hostile monster at the start), maps past stop 4 (pyramid, 6t8,
6d20, the dragon caves and elemental planes, lichlab, 1000terrors, ...), maps of fewer than about 15 monsters, and
t01/t02 (no entrance event to take the arrival from).

Entrances: the destination of the event that leads into the map (MM6-MM8 `evt.MoveToMap`, any events file of its
world) or, for MM9, the arrival of `worlds/mm9/data_tables/map_transitions.txt`. The unit test checks every one.

## Where it lives

- `assets_dev/engine/rulesets/roguelike/run.yml`: `dungeons[].stages`, `rules.route`, `rules.clear.uniqueGuardianHealthScale`,
  the camp stock, the Chaining Arrows card.
- `game/content/DungeonRun.{h,cpp}`: `DungeonRunStop::stages`, `dungeonRunStopAtStage`, `dungeonRunMapKey` (FNV-1a of
  the lower-case map name, 31 bits), `drawDungeonRunRoute` (one dungeon of each stop's stage from the seed's shuffle, no
  repeats, pinned stops kept), `legacyDungeonRunMaps` (the route 2.3-2.4 drew for a seed). The loader refuses a route
  stage with too few dungeons.
- `game/rules/roguelike/RoguelikeRules.{h,cpp}`: `RoguelikeRunState::route` and the arrival (`arrivalStop`,
  `arrivalSlainMonsters`, `arrivalSlainHealth`), `drawRunRoute`, `ensureRunRoute`, `runRouteStop`,
  `runStopProgress`, `runGuardianHealthScale`, `migrateRunBottles`.
- `game/rules/ExperienceRules.h` / `game/indoor/IndoorWorldRuntime.cpp`: `MapKillProgress::strongestMonsterCount`.
- `game/app/GameApplicationDungeonRun.cpp`: the route at the start, at every move and in `run`; the arrival; the
  empty map's way out; the guardian's multiplier; `run new <seed> [map...]` pins the first stops (tests).
- `game/app/GameSession.cpp`: on load, a run without a kept route gets 2.4's, and 2.4's camp bottles are converted.

## Saves

The writer is OYMINO1/v194 (upstream schema 193, see the integration record). The run's new state lives in named
globals: `run.route.count`, `run.route.<n>`, `run.arrival_stop`, `run.arrival_slain`, `run.arrival_slain_health`.

- A run saved by 2.3/2.4 (no route) keeps the route 2.4 drew for its seed: a run under way ends as it began; the
  random pool is for new runs. Its stop under way counts its clearing as it always did (no arrival taken).
- A kept route that names a map a later pool lost draws that stop anew and keeps the others.
- Talent ids are the run's own: OpenYAMM 2.7's MAW solo migrations (Pinning Shot to Power Shot, Attunement dropped)
  are deferred for mod saves and skipped for a Roguelike run.
- Frozen fixture of the last 2.4 production writer: `tests/fixtures/minotaur-2.4-v184-roguelike-v1.oysav` (patch #51,
  in its second dungeon; see its `.txt`).

## Known limits

- MM7-MM9 maps were each loaded headless with a run started on them (entrance, counted monsters, guardian), not
  played; their scripts, quests and fights in a real run are untested, as are MM9's own world rules (all-round sight,
  formations, MM9 items and books) inside a run.
- The camp's shops are MM6 houses whatever the world of the dungeon left; a guardian's artifact or relic comes from the
  world of its dungeon first.
- `mapKillProgress` still counts dead-at-arrival actors for MAW and Standard; only the run subtracts them.
- The group halving is a test value; with the original groups the large MM6 maps of stage 4 take 20-40 minutes each to
  the guardian (an estimate of the research).

## Checks (Linux, `safe-heavy-run`, 2026-11-18)

- Unit: the `roguelike` group (`DungeonRunTests`, `RoguelikeRulesTests`, `RoguelikeTalentTests`): 47 cases pass, also
  under AddressSanitizer. New: stages and the route (200 seeds: a stage per stop, no repeats, the first stop varies),
  pinned stops, 2.4's route for seed 4, a lost map redrawn, the arrival's subtraction, the guardian's multiplier, the
  frozen patch #51 save (talents by id after 2.7's migrations, route, bottle, round trip to v194), every camp stock
  item brews.
- Scenarios: `rulesets/roguelike_run.yml` 170 steps to victory (route pinned to 2.4's), `roguelike_v184_save.yml`,
  `roguelike_v1_save.yml` (the #51 save plays on), `roguelike_worlds.yml` (MM7 → camp → MM9 → camp on MM9's map with an
  MM6 shop and a bottle bought → MM6, the route through a save); a start on each of the 94 maps (entrance, counted
  monsters, guardian).
- The whole record, with the other CI groups: [openyamm-2.7-integration.md](openyamm-2.7-integration.md).
