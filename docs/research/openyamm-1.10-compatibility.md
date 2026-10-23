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
Windows packaging and selected automatic checks passed; manual validation remains pending.

## Checks before Windows dispatch

26 focused tests /30630 assertions,3 runtime /58 and226 selected regressions /7531
passed after integration, including production1.7 save migration. Groups overlap.
At dispatch the additional named-map-start test was pending; it later passed as
recorded below. The fresh full Linux rebuild was replaced by an incremental test build.
Selected regressions retain the documented sparse MM6/MM9/model and legacy
recovery-enchant exclusions; this is not a full-suite claim.
Package-validator regression and model-profile packaging passed.

## Final local checks

27 focused cases /30639 assertions,3 runtime /58 and227 selected regressions /7540 passed.
This includes the new named-map-start case and3 cycles for each production save format.
The changed test object was recompiled with the generated CMake compiler flags and
the unit executable relinked against the existing1.10 objects/libraries. This bypassed
a full configure after the host deleted temporary dependency sources and archive
downloads were interrupted. No fresh complete Linux game rebuild is claimed.
The release executable is built separately by the clean Windows CI job.
Selected exclusions and overlapping groups remain as stated above.

## Windows1.10 — Packaged / manual retest pending

[Run39941527807](https://github.com/Plane-PPP/openyamm/actions/runs/39941527807) completed successfully at 2026-10-23T11:18:35Z.
Source `13d2cf3bc1e1b0c005a15f4a253bc0dba875522b`; previous confirmed package was1.7/a1b8deb.
[Download OpenYAMM-windows-x64-Release.zip](https://github.com/Plane-PPP/openyamm/actions/runs/39941527807/artifacts/12498430051), 5,751,530,172 bytes.
Compile/link, launcher/game x64 PE, new model-emissive shader, indexed content hashes,
MM6/MM7/MM8 presence and exact embedded teacher/Stats resource comparison passed.
Artifact uploaded. Manual Windows gameplay and the user's actual save remain untested.
Use a separate install, copy only saves, keep originals and save in a new slot.
