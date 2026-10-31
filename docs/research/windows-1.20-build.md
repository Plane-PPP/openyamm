# Minotaur on official OpenYAMM1.20 — release verification

Status: Implemented / Automatically tested on Linux and Windows / ZIP packaged. Manual campaign/visual retest pending.

Source before update:b776797b71cc3835ff80bfc762d1b5366458ac2e.
Official stable target:1add1d374848648887c08707199921335b4ba440 (v1.20).
Built source:c80ebaa9634d23faa9f389d662fcd03daa482326.
[Windows run41192833570](https://github.com/Plane-PPP/openyamm/actions/runs/41192833570).

Commits:
- 95989b6ee0db9fdde5e2345a9147aef8bd5fa311: normal merge of official1.20, retained Minotaur and save envelope.
- 34a1fc667cee92116b822d8e4c358c6d7cbc317e: native multi-cell stacks, safe layout repair and fixtures.
- c80ebaa9634d23faa9f389d662fcd03daa482326:1.20 Windows metadata and regression selection/updater guard.

[Integration, conflicts, geometry audit, save migration and test scope](openyamm-1.20-stack-geometry.md).
[Definition geometry CSV](stack-item-geometry.csv).
[Protected Linux resource records](openyamm-1.20-local-resources.csv).

## Successful package

[Download Minotaur-Update-minotaur-1.20-c80ebaa9634d-25-1.zip](https://github.com/Plane-PPP/openyamm/actions/runs/41192833570/artifacts/13040197642).
267,188,188bytes (254.81MiB). SHA256 `40f9233ae3cd79071f0412f21ab51378c2f38ecc66b9863b1a64edd49017d59f`.
Expires2026-11-07T11:07:49Z. Generated package report and artifact API size/digest match.
CI verified launcher/game, all content hashes, MM6/MM7/MM8 resources and embedded hotfix. The large ZIP was not
redownloaded to the VM; the small metadata/resources archives were downloaded and their API digests verified.
47 changed packs:244,416,672bytes;119 reused:5,569,455,595bytes (5.19GiB). No full resource redownload required.
The confirmed next-update baseline now points at this actual successful package.

## Tests and resources

Windows:189 selected cases /48,755 assertions and7 native cases /159 assertions passed. The baseline169/39,730
and4/123 coverage is retained and expanded. All17 offline updater tests passed, including actual PowerShell5.1
filesystem scenarios; full/thin package-validator regression and3 resource-monitor unit tests passed.
Linux matches189/48,755 and7/159; additional save/inventory selection165/21,008 passed (partly overlapping).
These are focused regressions, not all2467 unit or59 native cases. No full-game/Android manual claim.

[Windows phase metrics and package evidence](windows-1.20-build-resources.json).
Windows job10:28:17–11:08:06UTC=39min49sec, including checkout/toolchain/setup/upload.
4vCPU,17,174,360,064bytes RAM, adaptive jobs4. Program+shaders/cooker:845.67sec, runner peak8,153,419,776bytes
(7.59GiB), meanCPU90.33%, peak100%. Test compilation:800.02sec, peak8,493,744,128bytes (7.91GiB),
meanCPU99.06%, peak100%. Content115.23sec, archive3.37sec. No monitor stop or automatic build retry.
These RAM/CPU measurements cover the whole dedicated runner, not compiler RSS. Windows disk swap, PSI and
cgroup counters are unavailable and recorded null, not zero. Windows commit usage is not disk swap.
Linux full successful build380.5sec,4.27GiB peak,swap0,3.734 average busy cores (4.0 peak sampled),
no sampled memory pressure/high/max/OOM event. Both environments keep their own existing safety rules.

A final documentation/baseline commit follows the built source with [skip ci]; it changes no executable code.
The pre-existing windows-ci-optimization.md remains uncommitted and unchanged.

## Installation and save precautions

The update targets installed minotaur-1.18-fc2c74c256c9-24-1, the last confirmed package.
Close the game. Back up the whole install folder and saves. Extract the update ZIP beside OpenYAMM.exe,
open its Minotaur-Update-* folder and run Apply-MinotaurUpdate.cmd. If selecting the folder manually,
pass -InstallationRoot with the absolute existing game directory. Launch the ordinary root OpenYAMM.exe.
Do not overlay an official author's program on this mod or reinstall old resource hotfix ZIPs.
The installer verifies hashes and changes the application pointer only after successful verification;
unchanged large resource packs are reused. It does not replace saves/settings.

Load a COPY of the old campaign save and immediately inspect every member's inventory. Native larger stacks can
force overflow to the party's feet, with a status notice. Collect overflow before leaving the map. Check stored
bags/recruits and chest contents. Save to a new slot, exit and reload that slot. Retain the untouched old slot.
New143 saves are for this binary; backward reading by1.18 is not promised. User campaign saves were not supplied,
so actual private campaign compatibility requires this check even though frozen old-writer fixtures round-trip.

## Manual visual checklist

All reagent/bottle quantities must retain native scale; Wasp Wing21 is2x1, empty bottle1/10/50 is1x2.
Gog Blood7 is1x2; MM8 Obsidian216 stays1x1; larger MM7 Obsidian1018 is2x2 with native scale preserved.
Check last row/right column, splitting, held cursor, sort, transfers, neighboring scrolls and brewing one unit
from each input stack. Prepared potions remain single items. Check HP alignment and Current/Next stat tooltips,
weapon/armor Recovery, dual Axes/Axe+Mace, Lethal Strike message and physical Damage Resistance.
Compare4:3,16:9 and ultrawide. The VM's partial terrain assets prevented an actual rendered comparison;
automatic placement/dimension checks do not replace visual testing.
