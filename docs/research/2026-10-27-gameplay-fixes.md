# Gameplay fixes after the 2026-10-27 interruption

Status: Implemented / Automatically tested on Linux. Windows packaging and manual gameplay pending.

## Alchemy

Standard PotionRules::mixingAlchemyLevel previously returned only the trained skill.
Both the alchemy bench and inventory mixing use this helper; equipment skill bonuses
were displayed by the UI but omitted from brewing. Use trained level plus skillBonus.
An unlearned skill remains unavailable. Ingredient power is still added separately.
For trained 4, combined equipment bonus 25 and Wasp Wing (217, power 10), effective
Alchemy is 29 and catalyst power is 39, rather than 14. This does not change stacking.
MAW retains its existing raw trained level + item bonus and twice-cap limit, avoiding
double-counting after the Standard fix.

## Mind Resistance

PartySpellSystem::resolvePartyBuffDurationSeconds omitted Mind Resistance from the
resistance group. Its zero-duration buff immediately disappeared, explaining both
missing icon and unchanged Stats. Add the missing case: one game hour per skill point,
with the existing mastery-dependent resistance power unchanged. The regression uses
a level-20 Cleric and checks all six resistance spells, all four masteries and expiry.

## Levitate

Casting already installed Levitate, WaterWalk and FeatherFall, but indoor and outdoor
burning-floor checks ignored Levitate. The movement driver now derives a transient
Levitate flag, suppresses lava/water ticks and resets hazard timers through existing
branches. Party damage application also rejects queued environmental hazard ticks.
The flag does not enter saves, grant Fly, or protect against general fire attacks.
Tests cover indoor lava, WaterWalk without Levitate, outdoor water/lava, queued effects,
expiry and absence of flight.

Pressure-plate filtering already exists in IndoorWorldRuntime::triggerFaceEvent:
Levitate bypasses only plates classified by isLevitateSensitivePressurePlate. Ordinary
events and chest traps are not blanket-disabled. Keep this behavior; synthetic classifier
coverage passed in LegacyLuaExportTests, including original EVT fixtures from all three worlds.
Direct scripted trap spell execution also passed: Levitate does not blanket-disable scripts.
Actual-map/manual checks remain separate.

## Safety and verification

The interrupted build used unsafe parallelism; no old binary is accepted as validation
of these changes. See /home/plane/incident-2026-10-27/REPORT.md for incident evidence.
Rebuild only through safe-heavy-run at jobs=4, with verified 16/18 GiB RAM and 1 GiB
swap cgroup limits. Persistent toolchain: /home/plane/.local/share/openyamm-toolchain.
No system installation, save schema change, Windows publication or balance change.

## Completed validation

- Protected build `20261027T160723Z-ec91891a58`: openyamm_unit_tests, jobs=4, exit 0.
- Focused run `20261027T161153Z-136714a329`: 7 cases / 404 assertions, all passed.
- Final related run `20261027T161703Z-2f7589188f`: 81 cases / 32,164 assertions, all passed.
  Filter: `*alchemy*,*resistance*,*lethal*,*minotaur*,*axe*,*levitate*,*lava*`.
  Exclusions: `*Bloodaxe*` (unrelated quest) and `MM8 lava terrain is drawn by the liquid shader`
  (full-map/render assets absent). This is selected coverage, not the full test suite.
- Earlier broader runs failed because sparse checkout omitted UI/MAW/event fixtures.
  Restore only required small files from the existing commit; no resource edits. After restoration,
  all selected tests pass. Preserve failed-run logs rather than hiding them.
- `git diff --check` passed. Lighting check reports 0 stale bakes in the available sparse checkout;
  this is not an audit of absent assets.

See [protected build measurements](safe-heavy-run-2026-10-27.md). No Windows artifact was built,
no push/release performed, and the installed-update baseline remains 1.14.
