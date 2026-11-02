[Русский](surface-materials-mm8-window-glow.md) | **English**

# Night window glow: OpenYAMM 1.20, MM8 and Minotaur Rebalance

Date: 2026-11-01. Status: research; no gameplay changes were made.
Reviewed official v1.20 SHA 1add1d374848648887c08707199921335b4ba440 and the sources of the released mod
c80ebaa9634d23faa9f389d662fcd03daa482326, package minotaur-1.20-c80ebaa9634d-25-1.

## Observation and conclusion

User: Windows 11, Intel Core Ultra X7 358H / Arc B390; a Radeon RX 9070 XT is also available.
Surface materials enabled in Settings→Video→Experimental; night-time Ravenshore, Alvar and Shadowspire were checked.
Walking between maps, the stable and Town Portal produced no glow; other graphical effects work.

The confirmed main cause for this version is incomplete coverage of the original MM8 textures by material maps
in upstream 1.20. This is not a loss of these resources during the Minotaur port and not proof that the Intel GPU is unsuitable.
The renderer does not derive lighting from a window picture alone: it needs a prepared glow mask for the specific texture.
A missing pair of maps deliberately leaves the old matte rendering, with no error. Repeated map changes or nights do not create data.

Limit of this conclusion: the user's installation and driver were not checked remotely, and there is no comparative Intel/AMD
run. An independent driver problem cannot be ruled out in general; but it is not needed to explain the missing
materials in the listed towns. A full visual run of these towns on Windows is not claimed.

## What upstream promises and contains

[Release 1.20](https://github.com/pjasicek/openyamm/releases/tag/v1.20) announces experimental Surface Materials
(relief/glass/metal/glowing windows), off by default.
The [implementation document at the exact release](https://github.com/pjasicek/openyamm/blob/1add1d374848648887c08707199921335b4ba440/devdocs/SURFACE_MATERIALS_PLAN.md)
lists ready, visually verified MM6 New Sorpigal at its start and leaves material production
for the other MM6–MM8 maps to further work in section 8. So the settings item enables the technology and does not guarantee
night lighting of every window in every town. The UI wording is too easily read as full coverage.
This is a feature of new OpenYAMM, not a comparison with a mandatory effect of the original 2000 MM8.

The release's Git tree contains only 11 normal/detail pairs for MM8:
bdoorb, btrima1, btrimb1, btrimb2, bwooda1, bwooda2, bwoodb1, bwoodb3, bwoodb4, solid01, t66a12d.
Stack quantity, the Minotaur class and other gameplay balance changes have no connection to this.

## Check of the three real maps

[Full CSV of BModel face textures](mm8-surface-material-coverage.csv), with data taken from the standard ODMs.
The light read-only analysis repeats the OutdoorMapDataLoader structure: MM8 terrain offset 0xb4, three 0x4000 blocks,
normal-count/CMap1/CMap2/normals, BModel headers 0xbc, vertices 12 bytes, faces 0x134, flags2 and texture names of 10 bytes.
Faces with 2–20 vertices are counted; empty names are excluded. This is a data audit, not a GPU test.

| Map | BModels | Distinct non-empty textures | Has a Surface Materials pair | Examples of missing families |
|---|---:|---:|---:|---|
|Ravenshore/out02.odm|121|131|11|t06*: 26 textures without maps|
|Alvar/out03.odm|99|89|1|t05*: 34 textures without maps|
|Shadowspire/out06.odm|84|103|11|t19*: 28 textures without maps|

In Alvar the only match is solid01. The 11 covered textures of Ravenshore/Shadowspire are the small set
listed above; most town textures in use have no maps. The match count is NOT the count of glowing windows:
having a pair still requires a non-zero glow mask, a suitable BModel path and night time.
A separate visual audit of each BC7 glow channel's contents was not done in this research; the existence
of a pair must not be presented as a positive check of glow.

## Setting, data and render path

- assets_dev/engine/ui/menu/settings_catalog.yml:421: Surface materials, action "from the next map".
- MenuSettingsModel.cpp:575 and GameApplication::applyMenuSettings:10637 apply the choice;
  applyCurrentSettingsToActiveRuntime:7782 passes the flag to GameDataLoader::setSurfaceMaterials.
  So the claim "the setting is not connected anywhere" is not supported by the sources.
- GameDataLoader.cpp:4986 passes surfaceMaterials to MapCompanionLoadOptions.
- MapAssetLoader.cpp:1829–1884 reads `<world>/surface_materials_x4/<stem>_n.oytex` and `_d.oytex` next to the color
  texture in textures_x4. Both missing→nullptr→old path; one missing/invalid format→error.
- Surface normal.b stores glow: the lower half is night, the upper half is permanent. surface_material.sh:133–136
  multiplies nightStrength by nightGlow; a white/yellow area in ordinary albedo does not become a light source by itself.
- OutdoorRenderer.cpp:1369–1371 calculates nightGlow from the game sun:
  clamp((0.05−sunHeight)/0.2,0,1). The day cycle is supported and not tied to restarting the program.
- bindSurfaceMaterial:4121 requires the flag, a valid normal map and baked direct-sun pages. So maps are needed
  not only in the index: they must belong to a specific surface and a supported path.
- Glow is an addition to surface color, not automatic placement of a dynamic lamp in a room or street.
  Bloom does not create a missing mask; additional Surface parallax/reflections are not a condition for glow.
  Animated textures, moving mechanisms and separate non-baked paths do not get this treatment in 1.20.

## Check of the fork and Windows packaging

Comparison of upstream 1add1d3 with built source c80ebaa: no differences in game/shaders, game/render,
MapAssetLoader.cpp, OutdoorRenderer.cpp, IndoorRenderer.cpp, tools/package_runtime_assets.py,
tools/packaging/packs.yml. The mod did not replace the algorithm or shaders with an older version here.

The confirmed 1.20 baseline contains 12 MM8 surface_materials_x4 packs, 3,389,212 bytes in total;
all 12 are new relative to the confirmed 1.18 base and so enter the delta rather than being wrongly counted as reused.
The number of buckets/ZIP packs does not equal the number of texture pairs. The desktop packager keeps surface_materials_x4;
their exclusion applies to Android, not Windows. CI packs the committed cook and creates no new class maps.

The log of successful Windows job 124513067108/run 41192833570 contains GLSL generation of:
fs_outdoor_bmodel_baked_materials, baked_shadow_materials, baked_array_materials,
baked_array_shadow_materials and the corresponding indoor materials. So the theory "the new shaders did not
build at all" is not supported by this run. The whole shader/runtime archive was not downloaded again or run
on the user's GPU; the CMake sources, generation log and metadata/index of the released package were checked.

## Windows and hardware

engine/BgfxContext.cpp:30–47 selects OpenGL for the ordinary Windows build; DirectX 12/RTX are not a condition
for this particular effect. Standard 1.20 has no separate switch disabling glow by vendor Intel/AMD.
[The official Intel comparison](https://www.intel.com/content/www/us/en/products/compare.html?productIds=245523%2C245526%2C245527%2C245529%2C245530%2C245531%2C245532%2C245533%2C245713%2C245720%2C245722%2C245727.html)
confirms Arc B390 and OpenGL 4.6 for the 358H. Integrated graphics alone do not explain the missing effect.
API support does not prove the absence of a driver bug; the driver version, actual renderer and its errors
from the user's openyamm.log were not provided. A specific Intel/AMD failure is not established from this report.
Switching to the RX 9070 XT will not create the missing MM8 material maps.

## What to do and what not to do yet

For a control check use the already covered MM6 New Sorpigal/oute3.odm, town hall and tavern at night,
with Surface materials, after a fresh map load. This is a documented upstream example, not a test
performed here. If glow is also missing there, then useful data are openyamm.log after start/load,
the exact package revision, the saved video.surface_materials and the driver version; then an identical Intel/AMD frame.
Do not start by reinstalling Windows or drivers or downloading the full 5 GiB of resources.

Spreading the effect to MM8 needs separate work: authored height/class/glass-glow maps for the
missing textures, cooking, checking links with BModels/bakes, visual day/night checks and a new ZIP patch.
Just enabling the flag or rebuilding without new materials will not fix this. Image production, changes to
resources/code/UI description, builds, publishing and push are not done in this research task.
