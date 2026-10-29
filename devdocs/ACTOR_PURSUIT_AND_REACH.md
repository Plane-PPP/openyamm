# Monster pursuit around ledges and melee reach

Status (2026-10-26): implemented. Two changes that let a melee monster find the way up to a party on a ledge
instead of standing under it.

## Melee vertical reach (`world.yml` `actorCombat`)

```yaml
actorCombat:
  meleeVerticalReach: 0.5
```

- A monster's melee reaches the party only across a height difference (feet to feet) of at most this share of the
  lower body's height, and never less than 48 (`GameplayActorService::MinimumMeleeVerticalReach`), so a step never
  stops a blow. With 0.5, a 179-high Drudge cannot strike a party standing 96 above it and pursues it instead.
- Without the block (every MM6–MM9 world) the share is 0 and height never matters, as before.
- Read by `loadWorldManifest` (`WorldActorCombatDefinition`), set on the session's `GameplayActorService`
  (`setMeleeVerticalReach`) when a map's runtime starts, copied into each AI frame (`ActorAiFrameFacts::
  meleeVerticalReach`, since the AI builds its own default services).
- Applied where a monster decides it is in melee range of the party (`resolveActorEngagement` in the AI, the indoor
  facts' `inMeleeRange`, the outdoor engagement) and where its blow lands
  (`IndoorWorldRuntime`/`OutdoorWorldRuntime` `...CanApplyPartyMeleeImpact`). Monster-against-monster melee is
  unchanged.
- Hesper sets 0.5 (`level_generation/projects/hesper_stage/stage.py`).

## Path plans in headless runs

Monster routes are planned on worker threads (`ActorPathRuntime`, two per world). A headless run simulates many seconds
in a few wall-clock milliseconds, so a plan that took 30 ms landed only after the run had moved on, often after its
end: headless pursuits never detoured (`path_deferred=1` for a whole scenario). Every headless command now sets
`OPENYAMM_SYNCHRONOUS_PATH_PLANS` (`game/app/main.cpp`), and `ActorPathRuntime::setWorkerCount` then keeps no workers,
so plans run on the calling thread and land in simulated time. The game itself keeps its workers.

## Checks

- Unit tests: `--test-case='*melee vertical reach*,*actor combat*'` (the rule; parsing and errors).
- Headless (Hesper stage): `level_generation/projects/hesper_walk_test/walk_test.yml` in the walkability test room (a
  96-high platform with a ramp at one end): a Drudge on the floor plans a route, takes the ramp and reaches the party
  on the platform; a Drudge just below the edge, within the flat melee distance, does not strike up and goes round by
  the ramp (`actor_visible` `near` with `min_z`).
