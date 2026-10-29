# Minotaur on OpenYAMM 1.18 — packaged Windows update

Implemented / Automatically tested on Linux and Windows / ZIP packaged. Manual gameplay and visual checks pending.

[Successful Actions run40863165794](https://github.com/Plane-PPP/openyamm/actions/runs/40863165794).
[Download Minotaur-Update-minotaur-1.18-c0ccf0a877bc-23-1.zip](https://github.com/Plane-PPP/openyamm/actions/runs/40863165794/artifacts/12894798568).
The artifact is143,681,237 bytes (137.03MiB), available until2026-11-05T11:57:30Z.
SHA-256: `211612350913e54b36d93a016cf6ba0fb57c82afdba138077390ab475b4ac667`.
The generated package report and GitHub artifact API agree on both size and digest; the full ZIP was not
redownloaded to the VM. Package contents were validated in CI before upload.

## Revision and merge

- Previous installed package: `minotaur-1.14-c991da37a2de-20-1`, source `c991da37a2de3b6c74a5ed838e38537fb6a5cc2b`.
- Branch before merge: `9dcf1c72684afec137e0fb65a977dd714ef713aa`; backup `backup/minotaur-before-v1.18-2026-10-29`.
- Common ancestor: `f1d7c45d8a7c1fb2edbb4542b4a144cb476b9486`.
- Official v1.18: `4ab07ed519a66feab24599d69bc0ae19971250e2`; annotated tag `207a3c9ce0ec2de6a1ce091d6268964748046b77`.
- Built source: `c0ccf0a877bc74fca0aa84a1594ed099d318cbdd`.
- New package: `minotaur-1.18-c0ccf0a877bc-23-1`.

Normal merge preserves published history. Only the fork's feature branch was pushed; neither main nor the author's
repository was modified. Core decisions in conflict resolution:

| Area | Resolution |
| --- | --- |
| Alchemy / Mind Resistance | Keep official1.18 logic; PotionRules, MawPotionRules and PartySpellSystem match upstream. No double item/hireling bonus or duplicate duration case. |
| Levitate | Keep official surface guards, floorBurn types/resistance/FireImmune; remove duplicate setter declarations/definitions. Retain additional pre-movement synchronization and queued-hazard guards from our fix. |
| Skills / teachers | Cybernetics keeps48, Lethal Strike becomes49. Saved names and3 teacher services retained; no balance change. |
| Combat API | Adapt native Lethal Strike death to ShieldHitModifiers and the new min/max damage-helper argument, preserving terminal death, XP and loot. |
| Saves | Mod140 wraps upstream139; production mod132 explicitly maps to131. Earlier mod layouts and ambiguous legacy84/85 import remain supported. Keep upstream compact blood/map storage. |
| UI / creation | Preserve the existing Stats layout fix and creation logic; adapt the runtime creation test to the upstream callback signature. |

Logical commits: `d396d29` merge; `d40a5ec` reagent stacks; `7bd8385` stat/Recovery inspection;
`79e7941` regressions/Windows packaging; `d93a0f0` equip restrictions and compact-save tests;
`c0ccf0a` exact worn-stat values and active attack Recovery.
Earlier runs40848860065 and40856630819 were cancelled for source revisions, not OOM/guard stops.

## Implemented behavior

See [implementation details](../minotaur-1.18.md). Ordinary compatible reagents stack to50 in one inventory cell,
merge during grants/manual drops/sorting, split with Shift1/Ctrl half upward, and consume1 per brewing operation.
All meaningful item properties participate in compatibility. Quantity uses the existing save field across inventory,
bags, cursor, floor/chest/corpse payloads. A split portion can be sold independently with quantity-aware pricing.

Standard primary-stat supplements retain original text and use engine thresholds and CharacterSheetSummary.
HP/SP contributions use real class resource functions on copies. Armor shows its isolated mastery-adjusted penalty;
weapons show actual attack Recovery, including both hands, Speed, Haste, minimums and Minotaur bonuses.
Unequipped previews use normal equip rules on a Party copy, including doll restrictions, or explicitly labeled base
Recovery if preview is refused. Wands/blasters retain their ordinary attack priority. Long bodies clip and scroll.
MAW keeps its own stat/armor descriptions and supplies its actual combat Recovery override.

## Automatic validation

Linux and Windows each passed156 selected unit/regression cases /36,020 assertions and3 native Lethal Strike
cases /58 assertions. The native cases cover actual indoor/outdoor death, one-time XP/loot/hooks and innate skill
creation. Twelve inspection cases cover all7 stats,29 thresholds, item bonuses counted once, HP/SP class modes,
armor mastery contributions, dual axes/Axe+Mace, two-handed previews, Speed/Haste/minimums, restricted equipment,
wands/blasters, MAW overrides, read-only behavior and bounded/touch scrolling.

Stack tests cover125-unit splitting;10+15,25+25,49+3,50+1,48+5 and42+15; metadata mismatches; full-inventory
rollback; manual remainder/splitting/transfers; old-single sorting; both brewing orientations and the last unit;
bench stock; split sales; saved inventory/cursor/floor/chest/corpse quantities. A genuine old1.14 writer fixture
loads and resaves with named skills intact. Compact saves omit2,400 baked vertices while retaining splat metadata.
Resistance tests cover every mastery, application/refresh/expiry and no duplicate bonus. Alchemy tests cover items,
hirelings, mastery and MAW caps; official once-per-character potions refuse a second drink while allowing another
character. Synthetic Levitate tests include lava/water, queued effects, expiry, trap filtering and no Fly.

Windows also passed17 actual updater/PowerShell5.1 tests,3 monitor-selection tests, package verification and the
native monitoring smoke test. Linux's corresponding run discovered20 tests, passed10 and skipped10 Windows-only
cases, with package verification passing separately. This is a focused selection, not all2,386 unit tests:
MM9 is not shipped, Bloodaxe artifact-specific tests and the full-map lava shader test were excluded; relevant
Axe and synthetic lava/Levitate behavior was tested. Local stale-bake check found0 in the sparse tree; it does not
certify every downloaded upstream lighting asset.

After packaging, a separate Linux inventory/chest/corpse selection passed83 cases /4,076 assertions
(partly overlapping the main selection). Its first broad sweep had12 missing asset-fixture failures and an
outdated debug-Lich assertion expecting every canonical skill. Small scene/legacy fixtures were restored; the test
now explicitly requires Lethal Strike to be absent from a non-Minotaur Lich. Full-map/animation suites remained
excluded locally. This assertion-only correction does not alter the built game; the ZIP's source remains c0ccf0a.

## Resource measurements

Compilation was native on GitHub's Windows runner, not Windows cross-compilation on Ubuntu.
The existing adaptive policy selected4 jobs on4vCPU /15.995GiB. Scope: sampled whole-runner RAM/CPU every second.

| Phase | Duration | Peak RAM, GiB | Mean CPU | Peak CPU |
| --- | ---: | ---: | ---: | ---: |
| Program / shaders / cooker | 18m39.19s | 7.679 | 92.37% | 100% |
| Unit / runtime compilation | 16m36.28s | 7.855 | 99.14% | 100% |
| Gameplay tests | 4.52s | 2.737 | 24.22% | 27.69% |
| Native world tests | 0.88s | 2.721 | 29.46% | 29.46% |
| Content packs | 2m06.06s | 4.438 | 26.57% | 44.92% |
| Program package | 7.20s | 4.056 | 19.13% | 28.91% |
| Incremental ZIP | 3.41s | 3.454 | 28.44% | 31% |

Windows job total47m59s; minimum observed free RAM8.140GiB. All monitored commands exited0 with reason completed;
no guard stop/retry. PSI/cgroup events/disk-swap peak are unavailable in this Windows monitor, not reported as zero.
[Machine-readable Windows evidence](windows-1.18-resources.json).

[Local protected-run evidence](v1.18-local-resources.csv): jobs4, task limits16/18GiB and1GiB swap, one operation
at a time, no pressure/OOM/limit events or swap use. Largest task peak5.49GiB occurred in a compile-error run;
final successful broad build peaked5.10GiB. No limits or system configuration were changed, no Vulkan build used.

## Update and manual verification

The ZIP replaces9 packs (121,211,935 bytes) and reuses123 (5,657,002,470 bytes,5.27GiB).
The next-update baseline is checked in under packaging/minotaur; the old baseline remains available.

1. Close the game and copy the actual save directory to a separate backup.
2. Extract the entire ZIP, keeping its payload and manifest together. Open its Minotaur-Update folder.
3. Run `Apply-MinotaurUpdate.cmd -InstallationRoot "D:\path\to\OpenYAMM"`, with the actual root containing `app/current`.
4. Wait for the target-version success message and launch the usual OpenYAMM.exe.
5. Load a copy of an old campaign save, verify party/equipment/skills, sort old reagents, then save/reload.

The installer does not replace saves/settings. The new game writes mod140; retain original1.14 saves for rollback.
Real user saves, actual installation on the user's PC, HD4:3/16:9/ultrawide/Android rendering and manual combat
remain untested. Verify50+overflow, one/half splitting, repeated brewing/sales and cursor/loot persistence;
inspect both equipped weapons and previews, change Speed/mastery/Haste/armor, and check long descriptions.
Teacher flavor/next-teacher hints, Identify Monster redesign and the bestiary remain Deferred.
