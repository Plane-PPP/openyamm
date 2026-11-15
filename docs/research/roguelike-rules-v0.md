# Roguelike rules v0 on Minotaur / OpenYAMM 2.3

Status: implemented and checked headless on Linux (2026-11-14). Nothing was played on a screen; Windows is not built.
The rules follow the user's answers in `openyamm-roguelike-notes/codex-review-2026-11-12/claude-answer.md`; the
player's guide is `openyamm-roguelike-notes/как-играть.md`.

## Where it lives

- `assets_dev/engine/rulesets/roguelike/run.yml`: the dungeons (tier, groupScale, entrance) and the `rules` block.
  Every number there is a test value.
- `game/content/DungeonRun.{h,cpp}`: the definition and the route (one dungeon per tier of `route`, none twice).
- `game/rules/roguelike/RoguelikeRules.{h,cpp}`: the rules object the session creates for a ruleset package whose
  run.yml has `rules` (GameSession::setRuleset). It derives ExperienceRules (level-ups and the guardian's call in
  rewardMapProgress, the start's gold and food) and MonsterRules (group sizes, no refill, the HUD bar); every other
  rule is Standard's. Pure functions deal and apply the cards.
- `game/app/GameApplicationDungeonRun.cpp`: what needs the application, once a frame (updateDungeonRunRules): the
  guardian's placement, the choice boxes, owed books, the promotion, the camp; the sealed exit and the camp hold the
  map move in processPendingMapMove.

## Shared code touched

- `MapKillProgress` (ExperienceRules.h) and `IndoorWorldRuntime::mapKillProgress`: countedHealth, slainHealth, the
  strongest kind and the slain late actors. Monsters spawned after the first fill and NPCs carry no weight.
- `EventRuntimeState::RulesetChoiceEventId` and `rulesetChoiceAnswer`; `GameplayDialogController` leaves the answer of
  such a choice box there instead of running an event.
- `RunExplosiveArrowsPlayerBit` (CharacterState.h), read by `GameMechanics::applyExplosiveArrows` beside the item flag.
- `GameSession`: the rules object, its loader, the state read from the save's named variables.
- The debug console's `run`: `run pick <n>`, `run leave`, `run close`; a scenario's console line also runs the frame
  update.

## State and saves

No serialized field was added (OYMINO1/v180 stays). RoguelikeRunState is mirrored into named global variables
(`dungeon_run.entered`, `run.level_ups`, `run.guardian`, `run.guardian_actor`, `run.door_offered`, `run.promoted`,
owed books in `run.books` / `run.book.<n>`). Each hero's owed cards, taken cards and perk levels are
Character::eventVariables 0xF0F0, 0xF0F1 and 0xF100 + the perk's index; the arrows' perk is a player bit. The cards
offered are a function of the seed, the hero's place and the cards taken, so a reload offers the same three.

## Known limits

- The count of hit points is taken from the live actors each time. Monsters a map hides until a script reveals them
  join the total when they turn hostile, so the total may grow during a visit; granted level-ups and a called guardian
  are never taken back.
- A headless scenario has no frames: the first fill of a map closes on the first world update, so the scenario
  advances the runtime a moment after each arrival. Without it the guardian would join the roster it guards.
- The guardian carries its kind's name; nothing marks it but its place and health.
- The camp moves the game time to 9:00 when it opens outside 9:00-16:00, because the houses keep their hours.
- The Medusa heroine has no class row of her own: her cards ignore class caps (`unrestrictedCharacters`), her hit and
  spell points and her starting skills are those of the class chosen at creation.
- Inherited from the skeleton: the dungeon counter advances before the next map loads and is not rolled back when
  the load fails.

## Checks

```sh
safe-heavy-run run -- cmake --build build --target openyamm openyamm_unit_tests openyamm_runtime_tests
safe-heavy-run run -- ./build/tests/openyamm_unit_tests --source-file='*DungeonRunTests.cpp,*RoguelikeRulesTests.cpp'
safe-heavy-run run -- ./build/game/openyamm --asset-scale 1 --user-data <new folder> --asset-root "$PWD/assets_dev" \
  --headless-run-scenario "$PWD/tests/scenarios/rulesets/roguelike_run.yml"
```

Results of 2026-11-14: 15 run cases passed; the scenario's 117 steps passed (start, sealed exit, level-ups, the
dialog's own click, twelve cards, a book in the pack, save and load with a living guardian, the camp and two shops,
the way to the entrance, four dungeons, the promotion, level 41, victory); the selected regression of the 2.3 port
passed (199 + 82 + 3 unit cases, 26 native cases). The scenario warns of the sparse checkout's missing music and
three decoration models, as before.
