# Minotaur v0.4 + Luck on OpenYAMM1.10

Official latest release checked2026-10-23: v1.10, commit
`3f06332001c7a841cc2c2d161f192a1252b25713` (published2026-10-22).
Exact tag merged; main unchanged. Includes upstream1.8/1.9/1.10 weather, creature,
travel, wizard-eye corpse, updater and long-dialog fixes. Mod balance unchanged.
Lethal Strike Luck bonus, Axe/DR, Stats and teachers retained.
Identify Monster/bestiary Deferred; LS-003 teacher flavour/hints Planned.

## Save compatibility

Official1.10 uses OYSAVE1/v94, adding PendingMapMove.startPointName.
The old mod1.7 also used number94 but OYMINO1 with upstream93 fields and combat Level.
Reader explicitly maps mod89 to schema88, mod94 to schema93, new mod95 to schema94.
Official OYSAVE1 through94 has no combat-level extension. Older mod85 (both known
weather layouts) and v84 stay supported with exact consumption/ambiguity rejection.
Upstream journal migrations use the mapped schema version.

Production mod1.7 fixture was written before merging by the unmodified v94 writer.
Its SHA256 is8f94c078d56758f157a0b037738684a62d0673220b60008b168ecbd0e91d562e.
Tests check3 successive migrations for old mod85, production1.5/v89 and1.7/v94:
skills, XP, inventory, bank/gold, quests, reserve and visited maps.
A separate test exercises the new named-map-start field together with Lethal Strike.
Synthetic fixtures only; the user's actual save was not supplied.

## Installation and validation

Use the new full package in a separate folder. Keep old installation/original saves.
Copy only saves, launch root OpenYAMM.exe, save into a new slot.
Do not overlay old settings/assets/hotfix ZIPs. New mod95 is not intended for older binaries.
Stock auto-update remains disabled by default to preserve mod code/content/save support.
Windows packaging, tests and manual validation statuses are recorded after completion.

## Checks before Windows dispatch

26 focused tests /30630 assertions,3 runtime /58 and226 selected regressions /7531
passed after integration, including production1.7 save migration. Groups overlap.
The additional named-map-start test and final Linux rebuild are pending while
temporary dependency sources removed by the host restart are restored.
Selected regressions retain the documented sparse MM6/MM9/model and legacy
recovery-enchant exclusions; this is not a full-suite claim.
Package-validator regression and model-profile packaging passed.
