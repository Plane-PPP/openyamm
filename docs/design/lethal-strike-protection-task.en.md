[Русский](lethal-strike-protection-task.md) | **English**

# Task: Lethal Strike and protected targets

Status: **Deferred / Low Priority**. These are materials for possible future work, not a task for the next stage. No code changed.
Skill contract: [Lethal Strike v0.4](death-strike.en.md).

## Goal

If the task is resumed: instant death should not bypass a story energy shield or a monster's real invulnerability.
The rest of the skill's balance stays. The fix is postponed until there is a practical need.

## Intended behaviour

| Case | Assumption | Status |
|---|---|---|
| Raised energy shield | Lethal Strike does not kill. The rules must be clarified against current code and published content | Deferred |
| `self:setInvulnerable` | Lethal Strike does not bypass real invulnerability. First verify the flag's semantics and how other attacks behave | Deferred |
| MAW ruleset | Out of the mod's scope, compatibility not required | Closed |

## Current behaviour

Confirmed by reading the code, not reproduced in game.

- `game/outdoor/OutdoorWorldRuntime.cpp:17921` and `game/indoor/IndoorWorldRuntime.cpp:16877` zero health under
  `lethalStrike` regardless of the `applyActorShieldHit` result.
- `applyActorShieldHit` returns zero damage for a target with `self:setInvulnerable`
  (`game/outdoor/OutdoorWorldRuntime.cpp:7091`, `game/indoor/IndoorWorldRuntime.cpp:11513`), but that result is ignored.
- Already working as a model: `canReceiveDamage=false` and `deflectsPartyHit` refuse Lethal Strike, and the trace writes `death_rejected`.
- As of 2026-11-02 this repository's `assets_dev` tracked no `shields.yml`, so a test would need its own
  fixture. The publication status of energy shield content was not checked here.

## Questions to answer if the task is resumed

1. Semantics of `self:setInvulnerable`: `game/gameplay/ActorBehaviorRuntime.cpp:877` describes it as "no damage reaches it"
   (the Bulwark). Which scripts use it, and does any standard attack bypass it?
2. Energy shield: what counts as protection — only a raised shield, or also a target whose shield is down?
   `devdocs/ENERGY_SHIELDS_AND_RELATIONS.md` sets shares by attack type; in its example a physical blow removes 0.5 of its damage, so
   "ordinary weapons do nothing" must be compared with Hesper's real data.
3. Should the trial be spent: is an RNG roll consumed, and what does `lethal_strike_trace` show for a protected target?
4. Should Lethal Strike remove a shield: by the current plan, no.

## Fix requirements

- A protected target does not die from Lethal Strike, and its health and shield do not change.
- The same blow's ordinary damage is handled as before.
- The trace names the skip reason, for example `shield_raised` and `invulnerable`.
- Without a shield or invulnerability, behaviour and formulas do not change.
- Shared logic does not diverge between indoor and outdoor. MAW rules are not touched.

## Regression tests

Names start with `Lethal Strike` so they match the CI filter.

| Scenario | Expectation |
|---|---|
| Raised shield, forced successful trial, indoor and outdoor | Target alive, HP and shield unchanged |
| Shield down | Per the decision on question 2 |
| `self:setInvulnerable(true)`, indoor and outdoor | Target alive |
| `self:setInvulnerable(false)` after enabling | Lethal Strike works again |
| Ordinary target | Unchanged: death, XP, loot, message |
| Trace | Skip reason recorded; a disabled trace does not change RNG |

## Workflow if the task is resumed

1. Research the questions above and record the findings here.
2. Implement, and update the contract, mod description, `CHANGELOG.md` and ROADMAP in both languages.
3. Build and run tests through `safe-heavy-run`: the new tests and the CI set.
4. Hand over for **independent review by Claude Code**, since this is a combat mechanic.
5. Manual verification is recorded separately.
