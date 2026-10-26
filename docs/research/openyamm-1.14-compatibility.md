# Minotaur v0.4 + Luck on OpenYAMM 1.14 — Windows validation pending

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
The29 applicable cases passed in the prior1.13 run;1.14 is rerun before publication.
Windows link dependencies winhttp/bcrypt for standalone unit tests are retained.
Actual run, artifact/hash/size and resource reuse are recorded after success.
