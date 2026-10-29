# Monster modifiers

Status (2026-10-23): implemented; `frenzy`, the charge's strike clip and its post leash added 2026-10-26. A world
add-on file gives monsters a list of **modifiers** from a fixed engine catalogue: small, parameterised rules over the
shared monster AI and combat (true aim, never staggers, kites party, blink, charge, frenzy). It follows the pattern of [shields.yml and relations.yml](ENERGY_SHIELDS_AND_RELATIONS.md): opt-in from data,
keyed by the world's own monster ids, so a monster no world lists (every MM6–MM9 monster) behaves, saves and renders
exactly as before. Hesper's turrets are the first users.

## Data: `worlds/<world>/modifiers.yml` (schema `openyamm.monsterModifiers.v1`)

```yaml
schema: openyamm.monsterModifiers.v1
monsters:
  10065: [never_staggers, {true_aim: {turn_degrees_per_second: 720, max_seconds: 3}}]   # a Ward Turret
  10050-10052: [never_staggers]                                                         # an id range
  10040: [{blink: {on_shield_break: true, cooldown_seconds: 8, max_height_change: 768}}]
```

- Read by `GameDataLoader::loadMonsterTable` beside `relations.yml` and `shields.yml` for every mounted world
  (`MonsterTable::modifiers()`, `findModifiers(id)`). Ids and ranges must lie in the world's `idRanges.monsters`.
- A modifier is a name, or `{name: {parameters}}`; parameters left out take their defaults.
- **Load errors name the file and line** and stop the load: an unknown modifier (the message lists the known ones), an
  unknown parameter (lists the modifier's parameters), a value outside its range or choices, a modifier listed twice for
  a monster, a monster listed twice (by id and by a range), an id outside the world's range, a blink without a trigger,
  `kites_party` with `min_distance >= max_distance`, a charge with `trigger_min_distance >= trigger_max_distance` or
  with both `use_melee: true` and a `damage` (or neither), a frenzy without a trigger.

## Catalogue

`game/tables/MonsterModifiers.{h,cpp}` (`monsterModifierCatalogue()`).

| Modifier | Parameter (default, range) | What it does |
|---|---|---|
| `true_aim` | `turn_degrees_per_second` (180, 1–3600); `max_seconds` (4, 0–60; 0: the whole flight); `then` (`straight` or `expire`) | The monster's projectiles (missiles and spell projectiles) home onto the target they were fired at, the party or an actor: each step they turn toward its current position by at most the turn rate and keep their speed. They still collide with everything on their way (terrain, BModels, decorations, actors, indoor faces), so only cover stops them. After `max_seconds`, or when the target dies or is removed, they fly straight on (`then: expire` ends them instead). |
| `never_staggers` | — | Hits never interrupt it: no hit reaction (the flinch and, outdoors, its knock-back). Stuns and paralysis from spells, skills and the EMP grenade still apply. |
| `indoor_meteor_shower` | — | It casts its Meteor Shower indoors too, each meteor from as high as the room's ceiling allows (`IndoorWorldRuntime::castIndoorMeteorShower`). The original game drops a monster's Meteor Shower indoors; a monster without the modifier never picks it there (`applyIndoorMeteorShowerRule` marks the spell uncastable before the AI), so it fights on with its other attacks. Outdoors every monster casts it as before. A Lua behaviour that casts Meteor Shower indoors needs the modifier too. |
| `kites_party` | `min_distance` (768, 0–20000); `max_distance` (2304, 1–20000) | The tactics ranged band (`ActorTacticsFacts::rangedMinDistance/MaxDistance`): commits to its ranged attack, backs off inside `min_distance`, closes in beyond `max_distance`, circles between shots. A monster whose `behaviors.yml` tactics set `ranged_band` may not also have it (the behaviour catalogue fails to load and names the monster). |
| `charge` | `trigger_min_distance` (384, 0–8192); `trigger_max_distance` (1280, 32–8192); `windup_seconds` (0.6, 0–5); `speed` (1400, 100–8000); `max_distance` (1536, 64–8192); `damage` (0, 0–10000; given without `use_melee`, it replaces the melee attack); `use_melee` (true); `knockback` (256, 0–2048); `cooldown_seconds` (8, 0–600); `stumble_seconds` (1.2, 0–10); `strike_seconds` (0, 0–5); `strike_impact_seconds` (0, 0–5); `effect` (`openyamm:fx/ember_impact`); `windup_sound` (`monster_charge_windup`); `windup_fallback_sound` (`roar 1`); `hit_sound` (`monster_charge_hit`); `hit_fallback_sound` (`swing with blunt weapon01`) | A readable dash onto the party. With the party within the trigger band (edge to edge), in sight, while it fights the party (hostile, aware of it, not party-controlled) and its cooldown ready, it winds up: it holds and faces the party, a flash (`effect`) and `windup_sound`. Then it dashes straight along the line fixed when the wind-up began, at `speed` for up to `max_distance`, following the floor and stopping at walls, ledges, decorations and other actors (the blink's spot checks, step by step). Reaching the party it hits once (its melee attack, or `damage`; the hit always lands, the defence is stepping aside) and throws the party back `knockback` units, with `hit_sound`; a miss (blocked, or the full distance run) ends in a stumble of `stumble_seconds`. With `strike_seconds` a hit is followed by a hold of that long playing the model's `charge_strike` script clip from `strike_impact_seconds` (the clip's blow), so the blow meets the contact. A post (placed `immobile`, tactics with a `post` leash) never charges a party beyond its leash, and its dash stops at the leash like at a wall. The cooldown runs from the end of the charge. |
| `frenzy` | `health_below` (0, 0–0.95; 0: off); `every_seconds` (0, 0–600; 0: never); `on_shield_break` (false); `windup_seconds` (0.8, 0–5); `seconds` (6, 0.5–60); `speed_multiplier` (1.4, 1–3); `attack_rate_multiplier` (1.5, 1–3); `spent_seconds` (0, 0–10); `cooldown_seconds` (20, 0–600); `glow_multiplier` (2, 1–5); `effect` (`openyamm:fx/ember_impact`); `sound` (`monster_frenzy`); `fallback_sound` (`roar 1`) | A haste power-up. Triggers (at least one): its health falls below `health_below` (once), every `every_seconds` roar to roar while it fights (with `health_below`, only while below it), its energy shield breaks. It roars for `windup_seconds` (holds and faces the party, `effect` and `sound` at its body; the model's `frenzy` script clip, else standing); a stun, paralysis or hit reaction in the roar cancels it (the trigger is used). Then for `seconds` it moves `speed_multiplier` faster, its attack recovery runs `attack_rate_multiplier` faster and its model's own emission is `glow_multiplier` brighter; then it holds `spent_seconds` (the `spent` clip, else standing): the punish window. `cooldown_seconds` runs from the end of one frenzy to the next. A frenzy and a charge never overlap. |
| `blink` | `on_hit_chance` (0, 0–1); `on_shield_break` (false); `every_seconds` (0, 0–600; 0: never); `cooldown_seconds` (6, 0–600); `min_distance` (384, 32–8192); `max_distance` (1024, 32–8192); `max_height_change` (256, 0–4096); `effect` (`openyamm:fx/arcane_pulse`); `sound` (`monster_blink`); `fallback_sound` (`30teleport03`) | A short teleport to a free spot it can see, on a damaging hit (by chance), when its energy shield breaks and/or periodically while it fights (a hit taken or a shot fired in the last 5 s), at most once per cooldown. Needs at least one trigger. The effect plays where it leaves and where it arrives, the sound at the arrival; it then faces the threat. |

Not modifiers, on purpose (the name in `modifiers.yml` is an error that says where the data lives):

- **`energy_immune`** stays in `shields.yml` (`monsters: {<id>: {energy_immune: true}}`): it is a rule of the shield hit
  resolution (`resolveMonsterShieldHit`) and needs the monster's shield profile, so moving it would split one rule over
  two files. The same goes for the shield `link`.
- **`stationary`** is the monster row's `Move` column (`Stationary`) in `monster_data`, which the AI already reads;
  Hesper's generator checks that its `Stationary` rule flag and the column agree.

### Choosing a turn rate

A homing shot turns on a circle of radius `speed / turn rate` (radians). Ener lasers fly 6 000 units a second: at
180 degrees a second the radius is about 1 900, so a shot fired at a party that steps aside or behind cover circles past
it; Hesper's turrets use 720 degrees a second (a radius of about 480). Slow missiles (fire bolts, arrows) home well at
the default.

## Behaviour

| Piece | Where |
|---|---|
| Catalogue, parsing, table | `game/tables/MonsterModifiers.{h,cpp}`; `MonsterTable::modifiers()` |
| Homing state and steering | `GameplayProjectileService::ProjectileHoming`, `attachHoming`, `steerVelocityToward`, `steerHomingProjectile`, `recordHomingEnd` / `recentHomingEnds` |
| A true_aim shot's homing | `trueAimProjectileHoming` (`GameplayActorModifiers.h`); the world runtimes' `attachTrueAim` after an actor's projectile request (`applyOutdoorActorProjectileRequest`, `applyIndoorActorProjectileRequest`), with the target from `ActorProjectileRequest::targetKind/targetActorIndex` |
| Steering each projectile step | `steerOutdoorHomingProjectile` / `steerIndoorHomingProjectile` before the step's collision query, so the predicted and the applied motion agree |
| never_staggers | `beginMapActorHitReaction` (both runtimes) |
| kites_party | `ActorBehaviorCatalog::applyMonsterModifiers` (from `GameApplication::reloadActorBehaviorCatalog`) |
| Blink decisions | `game/gameplay/GameplayActorModifiers.{h,cpp}`: triggers from `applyActorShieldHit` (every HP reduction and shield hit in both runtimes) and the actor's shots, cooldowns, candidate spots (`blinkCandidates`), presentation |
| Blink placement | `OutdoorWorldRuntime::blinkMapActorTo`, `IndoorWorldRuntime::blinkMapActorTo`; `blinkActor` (interface, scenarios) |
| Charge state machine | `GameplayActorModifiers::updateCharges` (wind-up, dash in steps of 24 units, recovery, cooldown), `applyChargeHolds` (the AI hold while charging), `chargeHit`, `chargeKnockbackImpulse`, the presentation |
| Frenzy | `GameplayActorModifiers::updateFrenzies` (triggers, roar, frenzy, spent hold; shield breaks from `observeHit`), `applyFrenzies` (the holds, the move speed and `ActorBehaviorFacts::attackRateMultiplier` before the AI), `moveSpeedMultiplier` (the indoor movement integration's own speed), `emissionMultiplier` → `GameplayRuntimeActorState::emissionMultiplier` → `ModelInstanceSystem::setEmissionMultiplier` (single skinned draws scale their emission; a boosted creature leaves the instanced batch), `scriptedClip` (the `frenzy`, `spent` and `charge_strike` roles into `GameplayRuntimeActorState::scriptedClip`); `frenzyActor` (interface, scenarios, console) |
| Charge in the worlds | `updateActorCharges` per fixed actor step after the AI (both runtimes; frenzies first), `chargeWorld` (sight, one step through `blinkMapActorTo` with `pBlockedByParty`, the events), `handleChargeEvent` (the melee impact through `GameplayCombatController::recordMonsterMeleeImpact`, the party's knock-back: a hop up through `requestSpecialJump` and its horizontal speed as a dash for the whole flight through `requestDash`, so the party lands `knockback` units back; fixed 2026-10-24, when a jump's horizontal speed lasted one frame and moved the party only a few dozen units); `chargeActor`, `chargeCounts` (interface, scenarios, console) |

**True aim.**

- The homing is attached when the shot spawns: every live projectile of that actor spawned during the request (a
  spell's several projectiles included), at its spawn speed. The party target point is the one the AI aims at (feet +
  96); an actor's is its body (feet + 70–75 % of its height).
- Each projectile step first turns the velocity (Rodrigues rotation toward the target, at most turn rate × step), then
  runs the unchanged collision and frame logic. Gravity still applies to missiles without NoGravity; the next step
  restores the speed. Trails and the 3D projectile models follow the position and velocity as before.
- The party's spells, event projectiles and every monster without the modifier never get a homing state, so their
  flight is the same code path with one extra `if`.
- The lead of a turret's tactics (`lead_seconds`) still aims the launch; the homing then steers to the current
  position.
- **Not saved:** a projectile in flight at a save loads without its homing and flies straight on. No save field, no
  `SaveVersion` change (still 95).

**Charge.**

- Phases: ready (the cooldown runs; the trigger is checked each fixed actor step), wind-up, dash, recovery (0.35 s after a
  hit, `stumble_seconds` after a miss), then ready with the cooldown. A charging actor holds through the AI
  (`ActorBehaviorFacts::holdActive`: it neither walks nor attacks; animations: standing in the wind-up, walking in the
  dash, the hit reaction in a stumble). Death, a stun or paralysis ends a charge where it is.
- The dash moves in steps of at most 24 units through the blink's spot check (`blinkMapActorTo` with a height change of
  at most 48 per step): a floor (no water or steep slope for walkers outdoors; walkable, in a sector, with headroom
  indoors), no blocking face or wall, decoration or other actor in its cylinder and a clear line from where it stands.
  The party's cylinder in the way is the contact; anything else ends the dash as a miss. A flyer keeps its height above
  the floor. The actor faces along the dash.
- The hit is one monster melee impact (`recordMonsterMeleeImpact`) with an attack bonus of 1000 (so the armour roll
  passes; the party's own evasion still applies): Attack1's dice, or Attack2's when Attack1 fires a missile and Attack2
  does not (a Lancer's lance thrust), or `damage` (physical). The knock-back is a short hop of the party
  (`requestSpecialJump`: 320 up, the horizontal speed that covers `knockback` under the party's gravity of 1280).
- Presentation: `effect` at its body and `windup_sound` (with `windup_fallback_sound`) at the wind-up, `hit_sound` (with
  `hit_fallback_sound`) at the hit. Drop `monster_charge_windup.wav` and `monster_charge_hit.wav` into a world's or the
  engine's audio to replace the fallbacks.
- Runtime only, like the blink: phases and cooldowns restart on every map load and after a load; no save field.
- The post leash (`ChargeActorFacts::leashRadius`, from `ActorBehaviorRuntime::postLeashRadius`) applies only to actors
  the map placed `immobile` whose tactics hold a post, as the tactics' own leash does.
- The strike clip (`chargeStrikeClip`, through `scriptedClip`) is presentation only: the hit is applied at the contact as
  before, and a behaviour script's own clip wins over it.

**Frenzy.**

- Phases: ready (cooldown, triggers), roar, frenzy, spent, then ready with the cooldown. The roar and the spent hold
  hold the actor through the AI (`ActorBehaviorFacts::holdActive`), as a charge does. A health trigger fires once, when
  the share first falls below `health_below`, and waits (for the cooldown, a charge or a hit reaction to end); a shield
  break waits up to 1 s. The period runs from roar to roar.
- The health and shield triggers need a hostile actor (not party-controlled) that can act; the periodic one also needs
  it to be aware of the party.
- Speed: the AI facts' `effectiveMoveSpeed` (its pursuit and wander intents) and, indoors, the movement integration's
  own speed; the charge's dash keeps its own speed. Attack rate: the attack cooldown ages `attack_rate_multiplier`
  times faster (`ageActorFrameTimers`).
- The glow scales the emission of the creature's own skinned materials; rigid attachments (held blades) draw through
  the static instancing path and keep their emission. Effect and sound: `queueFrenzyPresentation`.
- Runtime only: no save field.

**Blink.**

- Triggers: `on_hit_chance` rolls per damaging hit (a deterministic per-actor sequence); `on_shield_break`; and
  `every_seconds` while the actor fights. A request waits up to 1 s for a cooldown or a stun to end; an actor that is
  dying, stunned, paralysed, invisible or mid-attack does not blink.
- Candidates: 16 random spots on a ring of `min_distance`–`max_distance` around it (seeded per actor and blink), the
  ones sideways or away from the threat (the last attacker, or the target of its last shot) first. Spots that keep the
  threat in sight are tried first, then any free one. A failed blink retries after 0.75 s.
- **Outdoors** a spot needs: inside the map (and its `outdoorBounds`); the highest walkable floor (terrain or a BModel
  top) no more than `max_height_change` above the actor's own floor, and within that range of it; for walkers no water
  and no steep terrain slope; no blocking BModel face (`isOutdoorCylinderBlockedByFace`), decoration, other living actor
  or the party in its cylinder; a clear line of sight from where it stands (`hasClearOutdoorLineOfSight`). A flyer keeps
  its height above the floor.
- **Indoors**: the floor nearest its own height within `max_height_change` (walkable for walkers), headroom for its
  height (`sampleIndoorCeiling`), a sector (`findIndoorSectorForPoint`), no wall within its radius at three heights
  (`findIndoorWallContact`), no decoration, other actor or the party in its cylinder, a clear combat line of sight
  (`hasIndoorCombatLineOfSight`, sector-aware); the move state comes from the movement controller
  (`initializeStateFromEyePosition`), the collider is moved and its path is reset.
- The move keeps the actor's home (posts and leashes are unchanged); outdoors it updates the map delta position.
- Presentation: `effect` at both ends (`EventRuntimeState::worldEffectRequests`), `sound` at the arrival with
  `fallback_sound` while the placeholder has no file (`pendingSounds`). Drop `monster_blink.wav` into a world's or the
  engine's audio to replace the fallback.
- Runtime only: cooldowns restart on every map load and after a load; positions are saved as always.

## Hesper

- The design's `design/kreegan_world/data/make_monster_tables.py` maps rule flags to modifiers (`MODIFIER_RULES`):
  `TrueAim` → `true_aim` (720 degrees a second, 3 s), `NeverStaggers`/`NoStagger` → `never_staggers`, `KitesParty` →
  `kites_party` (1 024–3 072), `Blink` (and a "Blinks…" shield-break reaction) → `blink` (on a shield break, cooldown
  8 s, 512–1 280, height change 768), `Charges` → `charge` (from 384–1 280; per monster in `MODIFIER_OVERRIDES`: the Drudge
  Brute 10001 a quick short rush, the Lancers 10020–10022 a slow, long-telegraphed lance rush, the Glassback Bull 10082
  a heavy gallop from up to 1 536 with a knock-back of 512). It writes `data/monster_modifiers.yml` in the engine format and the "Engine
  modifiers" table of `MONSTERS.md`.
- `level_generation/projects/hesper_stage/stage.py` installs the staged monsters' rows as `worlds/hesper/modifiers.yml`
  (today the Starport Turret 10054 and Landfall's Ward Turret 10065 and Gate Bastion 10066: `true_aim` and
  `never_staggers`).
- Left to Lua behaviours (`devdocs/ACTOR_SCRIPTING_INVESTIGATION.md`) or later engine work: phases (`Phases3`,
  `LinesAt75_50_25`, `LeavesAt50`, `FleesAt30`), summons (`SummonDrudges20s`, `SpawnsHatchlings`, `SpawnsDrudgeWaves`),
  telegraphed attacks (`TelegraphedBeam1.5s`, `TelegraphedSweep2s`, `LockOn3s` with `Strike…`/`Range…`), shields fed or
  refilled by objects (`ShieldFedByEggSacs3`, `ShieldRefilledByColumns3`, `PhaseShieldByConduits4`,
  `RotatingShieldArcs`, `InvulnerableWhileCouplingsLive`), `ArmouredFront`, `Turn40DegPerSecond` (a tactics traverse),
  `DigWindow40s`, `SlagSprayFlanks`, `Mines`, `FireWaves`, `FireTeam`, `Taunts`, `Collared`, `ConsoleLinked`,
  `CourtOfNobles`, `HonourTerms`, `GuardsGridControl` and the relation flags (`relations.yml`).

## Checks

- Unit tests (`tests/MonsterModifierTests.cpp`):
  `--test-case='*monster modifier*,*catalogue lists*,*homing*,*true aim*,*blink*,*kites_party*'`: parsing, defaults,
  ranges, every error with its line; the steering math (turn limit, speed, target straight behind); a projectile
  following a moving target, flying straight after its time, expiring, losing its target; homing given only to the
  firing actor's new projectiles; blink triggers, cooldowns, periodic blinks and the candidate ring; kites_party as a
  ranged band and its conflict.
- Runtime tests (`openyamm_runtime_tests`, `tests/MonsterModifierWorldTests.cpp`) on MM6 maps with a modifier set in a
  copy of the monster table: a homing Fire Bolt in New Sorpigal turns after a party gone behind a building and the
  building stops it (a straight one flies on); outdoor blink spots refused inside a building, behind it, onto another
  monster and outside the map, accepted on open ground; indoor blink spots in the Hive accepted only with a floor, a
  sector, headroom, no wall and a clear line, some refused.
- Charge: unit tests `--test-case='*charge*'` (parsing, defaults and errors; the state machine with a stub world:
  wind-up hold, dash, hit, cooldown, a wall, the longest dash, the band, sight, a stun; the melee or Lancer's thrust
  roll; the knock-back impulse); runtime tests in the MM6 Hive (`openyamm_runtime_tests --test-case='*charge*'`): a
  spawned charger holds through its wind-up, dashes along the floor onto the party and records one melee impact, and a
  charge at a party beyond a wall stops short of the wall and stumbles. Headless (Hesper stage):
  `level_generation/projects/hesper_out1/charge.yml`: a Glassback Bull charges the party on its own and hits it (the
  party hurt); a second bull's charge misses a party that steps aside in the wind-up and ends in a stumble. Captures:
  `level_generation/projects/hesper_out1/review/charge_20261023/` (`tour_charge.yml`, an input tour). Console:
  `actor charge <index|last>`. Scenario: `charge_actor: {actor_index}`, `assert: {charges: {started, hits, misses,
  min_started, min_hits, min_misses}}`. MM6–MM9 monsters have no charge, so their AI, saves and timing are unchanged
  (the hooks return at once without a charging monster). The leash and the strike clip: unit tests `*leash*`,
  `*strike*`.
- Frenzy: unit tests `--test-case='*frenzy*'` (parsing, defaults and errors; the health trigger once, the roar's hold
  and clip, the speed, attack rate and glow, the spent hold; the period below the health share; a stun in the roar; a
  shield break; no overlap with a charge); runtime tests in the MM6 Hive (`openyamm_runtime_tests
  --test-case='*frenzy*'`): a roar that holds and plays its clip, then twice the pursuit speed and the glow, then the
  spent clip; a frenzy that starts by itself below half health. Headless (Hesper stage):
  `level_generation/projects/hesper_out1/frenzy.yml` (a Harrower started by hand; a second one that loses its shield
  and half its health roars on its own). Console: `actor frenzy <index|last>`. Scenario: `frenzy_actor: {actor_index}`,
  `assert: {charges: {frenzies, min_frenzies}}`.
- Headless (Hesper stage): `level_generation/projects/hesper_out1/true_aim.yml`: a Starport Turret west of the crew
  ramp; its shot at the party, which steps behind the ramp, turns after it and ends against the ramp
  (`homing_ends: {hit: world}`), the party unhurt; a fresh turret's shot at the party strafing in the open turns onto it
  and hits it (`homing_ends: {hit: party}`).
- Console: `actor blink <index|last>` blinks an actor with the modifier now; `projectile actor <index|last> attack1
  at_party` fires the actor's shot at the party as its AI would (a true_aim shot homes).
- Captures: `level_generation/projects/hesper_out1/review/true_aim_20261023/` (the turret's laser bending onto a
  strafing party; the MM6 before/after set, within the run-to-run variance of the spawned creatures' motion).
- Scenario steps and checks added for this: `fire_actor_projectile: {actor_index, ability, damage}` (an AI shot at the
  party now), `blink_actor: {actor_index}`, `assert: {projectiles: {source_monster_id, min_count, max_count, homing,
  toward_party_within_degrees, report}}`, `assert: {homing_ends: {source_monster_id, hit, min_count, max_count,
  report}}`, `assert: {party_health: {full}}`.

## Not done

- A Lua `self:blink()` command and per-placement modifiers (a map's actor list cannot name modifiers yet).
- True aim of the party's own homing weapons (no such item yet), and of traps and events.
- A homing projectile's homing after loading a save (it flies straight).
- The stagger rules proper (a hit over a share of the health staggers, with a cooldown: work item 2); `never_staggers`
  only switches the existing hit reaction off.
