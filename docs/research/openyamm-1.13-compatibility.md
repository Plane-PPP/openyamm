> Historical intermediate port, not published. Superseded by [OpenYAMM1.14](openyamm-1.14-compatibility.md).

# Minotaur v0.4 + Luck on OpenYAMM 1.13 — validation in progress

Latest official release checked 2026-10-26: [v1.13](https://github.com/pjasicek/openyamm/releases/tag/v1.13),
published 2026-10-25. Exact release tag merged into feature/minotaur-rebalance; no merge to main.
Prior patch/base: minotaur-1.10-b7e80f0583df-14-1, source b7e80f0583df4e594718cb3de63f87c4a0793fc7.

## Port and retained mod behavior

Upstream 1.11–1.13 adds technology skills, Standard/MAW rules and damage pipelines,
UI/quest/travel fixes, and native shield/death handling. The mod keeps Lethal Strike v0.4 + Luck,
its NORMAL/GIANT registry, innate acquisition, class caps, teachers, Stats Damage Resistance,
Axe balance and trace settings. No new Minotaur balance is intended by this port.
The existing passive remains in Standard's resistance/armor stage; the Axe damage helper
is called by Standard's monster-resistance stage, with no extra RNG draws.
MAW's independent balance is not claimed as a tested Minotaur rebalance mode.
LS-003 thematic teacher texts/hints remain Planned. Identify Monster/bestiary remain Deferred.

Upstream now uses skill IDs 40–47. Lethal Strike moves from 40 to 48; classic skills and all
upstream technology/MAW IDs remain unchanged. Skills are saved by canonical name, so saved
LethalStrike remains itself; teacher_topics is updated to 48 as well. It is neither a tech
skill nor a MAW-only skill and still spends ordinary skill points.
The duplicate skill-console registration is consolidated: old numeric mastery 1–4 and
new B/E/M/G spelling work, while upstream techpoints stays available. Lethal Strike caps remain enforced.

## Saves

Official 1.13 is OYSAVE1/schema129. New mod writes OYMINO1/v130, schema129 plus the existing
combat-level extension. Explicit old mappings: mod89→schema88, mod94→schema93,
mod95→schema94, mod130→schema129. Official saves through129 have no mod extension.
Legacy mod84/85 behavior and exact-consumption checks remain.
Campfire/journal/XP migrations receive the mapped source schema, not the mod header number.
Production-writer fixture minotaur-1.10-v95.oysav was captured using the unchanged old
1.10 test executable before recompilation. The existing progress test now includes it,
checking three load/save cycles alongside older formats. Fixture provenance/hash:
[structured review](openyamm-1.13-compatibility.json).
The user's actual save has not been supplied or tested. Back up saves and use a new slot;
new saves are not intended for old executables.

## Installer fix

User confirmed first patch installed with -InstallationRoot "$PWD" on 2026-10-26.
Actual gameplay/save loading was not yet reported. Previously autodetection examined only
parent-of-script. It now examines script directory first, then parent, then offers folder selection.
Both ZIP layouts have real PowerShell 5.1 tests: extracted in root or in a child folder.
Missing app/current error now includes the selected path, without suggesting another 5 GB download.

## Delivery and checks

ZIP patch targets the installed 1.10 patch above; unchanged logical resource packs are reused.
Windows workflow also builds/runs focused Minotaur/Axe/save unit tests, separately from actual
PowerShell installer tests. Build status, artifact and actual size are recorded after completion.
No manual 1.13 gameplay or real user save compatibility is claimed before user testing.

## First Windows attempt

Run40294728955 compiled/linked the game successfully and passed all17 installer tests.
The unit target stopped compiling on AxeTestWorld's old three-argument melee override;
updated to the 1.13 shield-tier signature. The tests were not run and no patch was uploaded.
A separate save test now checks that schema129 retains both EnergyGuns and LethalStrike
with independent levels/masteries and the new tech point pool. Subsequent attempts are recorded below.

Second Windows run40312194498 compiled every game and unit-test source, then failed linking
the unit executable: WinHTTP and FFmpeg's BCrypt dependencies were absent from its link list.
Added winhttp/bcrypt for this Windows test target, matching dependencies already used by
the game/engine. No tests ran and no patch was uploaded. The subsequent run is recorded below.

Final sparse-tree audit: the official packages.yml had retained its 1.10 copy during
sparse conflict restoration. Restored it exactly from v1.13 (including upstream macOS job).
This does not change the Minotaur Windows workflow or the game currently building.

Third run40318306385 linked both executables and ran the tests: 29/30 cases passed,
30790/30791 assertions passed. The sole failure was MM9 Keith Bloodaxe quest acceptance:
its name accidentally matched *Axe*, while this package deliberately has no MM9 data.
All selected Minotaur/Lethal Strike/Axe and other MM6/MM7/MM8 cases passed, including the
old1.10 fixture and separate EnergyGuns/LethalStrike save test. The selector now explicitly
excludes MM9* cases; no gameplay code or test expectations were weakened. Packaging was superseded by the1.14 release before publication.
