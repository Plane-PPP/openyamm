# Minotaur v0.4 + Luck on OpenYAMM 1.14 — Windows ZIP packaged

Latest official release rechecked 2026-10-26: [v1.14](https://github.com/pjasicek/openyamm/releases/tag/v1.14),
published 09:33:38 UTC, commit `f1d7c45d8a7c1fb2edbb4542b4a144cb476b9486`.
It appeared while the 1.13 port was being validated; exact tag1.14 is now merged instead.
No 1.13 mod package was published. [Earlier port/validation](openyamm-1.13-compatibility.md)
remains the technical history, including29 applicable Windows tests passed on schema129.

The patch base remains the user's installed `minotaur-1.10-b7e80f0583df-14-1`.
Root/nested ZIP extraction detection and its17 Windows tests are retained. No Minotaur
balance changes: Lethal Strike v0.4 + Luck, Axe, passive resistance, teachers and Stats remain.
LethalStrike stays ID48; upstream classic, technology and MAW IDs0–47 remain stable.
Skills are saved by name. LS-003 teacher flavour/hints Planned; Identify Monster/bestiary Deferred.

## Save compatibility

Official1.14 uses OYSAVE1/schema131, adding per-character spellbook page130 and school shelves131.
Mod now writes OYMINO1/v132 with schema131 plus the existing combat-level field.
Explicit mappings: mod89→88, mod94→93, production mod95→94, intermediate mod130→129,
new mod132→131. Legacy84/85 handling remains. Official saves through131 have no mod field.
Campfire/journal/XP migrations and guild-stock reads use the mapped schema.
The upstream change clears an old guild's mixed spell shelf so it can restock by school;
this is an inventory-of-the-guild migration, not deletion of party inventory or quests.

The production-writer1.10 fixture and three repeated progress migrations stay in the tests.
The latest-format test keeps EnergyGuns, LethalStrike and tech points independent, verifies
old saves default the spellbook page to -1, and checks a chosen page survives a new save/load.
Actual user save and manual Windows gameplay remain untested until the user checks them.
Back up saves and save into a new slot after updating; old executables cannot read new saves.

## Validation and packaging

Windows Minotaur/Axe cases exclude MM9* because this package ships MM6/MM7/MM8;
this prevents the unrelated Keith Bloodaxe quest being selected by the Axe wildcard.
The29 applicable cases passed again on1.14 (30,791 assertions).
Windows link dependencies winhttp/bcrypt for standalone unit tests are retained.
Actual run, artifact/hash/size and resource reuse are recorded below.

## Windows 1.14 — Packaged / manual retest pending

[Successful run40325494796](https://github.com/Plane-PPP/openyamm/actions/runs/40325494796), source `0b4fc6589a5003142c2a0a1d3dd82da6d5542d43`.
[Download ZIP patch](https://github.com/Plane-PPP/openyamm/actions/runs/40325494796/artifacts/12654334120): **910,392,473 bytes (910.4 MB)**.
Base `minotaur-1.10-b7e80f0583df-14-1` → `minotaur-1.14-0b4fc6589a50-19-1`.
Changed23 resource packs (888,146,682 bytes); reused109 packs (4,889,109,186 bytes).
The delta includes updated shared icons/models/world items, core/maps and the new ruleset pack.
84.17% less download than the previous5,751,530,172-byte full1.10 archive; no new full1.14 archive was built.
SHA256: `8bc8d0182468ee147f4d50090653156b3e99e0f4b100cc62c4c0ee42c70c4407`.

Automatically tested on Windows:29 selected gameplay cases /30,791 assertions;17 updater tests,
including10 actual PowerShell5.1 filesystem scenarios; full/thin validator regressions.
Native x64 game/launcher, shaders, all resource hashes and embedded teacher/Stats resources verified.
This is selected coverage; MM9 data is outside this package and its Bloodaxe quest is excluded.
The user's actual save and manual gameplay/installation of this1.14 patch remain untested.
Back up saves, run the **new patch folder's** Apply-MinotaurUpdate.cmd and save into a new slot.
Installer supports both nested extraction and scripts placed directly in the game root.
The next build defaults to this newly confirmed baseline; old baselines remain available.

[Structured build report](windows-v0.4-openyamm-1.14-build.json).
