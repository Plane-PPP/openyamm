# Imported party portraits

Players can add their own party portrait from the party creation sheet: **Choose Portrait → Add Portrait...**, then
pick the folder of a portrait package. The portrait joins the picker next to the built-in faces, plays every face
animation in the HUD, survives restarts and is kept by saves. Desktop only (Windows, Linux, macOS): the system folder
dialog of Android and iOS returns content URIs, not paths, the same reason the save import is desktop only.

Code: `game/party/CustomPortraits.{h,cpp}` (package format, check and install), `AssetFileSystem::setUserIconDirectory`
(serving the frames), `NewGameScreen` (button, picker, creation), `GameplayUiRuntime::availablePortraitTextureName`
(missing portraits). Packer: `tools/make_portrait_package.py`. Tests: `tests/CustomPortraitTests.cpp` and the user icon
folder case in `tests/AssetFileSystemTests.cpp`.

## What a party portrait is (confirmed in code)

| Item | Value | Where |
|---|---|---|
| Frames | 56 textures, numbered 01-56; the engine replaces the last two digits of a member's `portraitTextureName` with the frame of the current expression | `portraitTextureNameForTextureFrame`, `GameplayUiRuntime.cpp` |
| Expressions | `PortraitId` states and their frame sequences (`portrait_frame_data.txt`): conditions 02-12, idle 13-20, talking 21-24, no 25-29, yes 30-33, dodge 34-36, damage 37-39, reactions 40-56. Death and eradication use the shared `DEAD` / `ERADCATE` pictures | `PortraitEnums.h`, `PortraitFrameTable` |
| Size | 118x158 at the icons' x2 scale (the original 59x79 doubled), 8-bit RGBA, transparent around the face | restored `engine/icons_x2/pcNN-NN.png` |
| Transparency | real alpha; pure `#FF00FF` and `#00FFFF` also become transparent (legacy colour key) | `ImageAssetLoader`, HUD decode options |
| Lookup | by name in `Data/icons` through `AssetFileSystem`; a cooked set `faces_<prefix>.yml/.oytex` is optional, plain PNG files work | `GameplayHudCommon::loadHudTexture` |
| Character row | `character_data.txt`: doll type and body art, default class, voice and sex, race, `Face pics prefix` (column 19, e.g. `pc02-`) | `CharacterDollTable` |
| Save | a member stores `characterDataId`, `portraitTextureName` (frame 01) and `portraitPictureId` | `SaveGame.cpp` |

The face is therefore separable from everything else: the doll, race, sex, class and voice come from the character
row, the face only from the texture name. A ruleset package that added a row with its own prefix and 56 plain PNG
frames (no cooked set) animated correctly in the HUD, which is what the import relies on.

## Design

- **A portrait is a face on an existing character row.** The package names a `base_character` (a `character_data.txt`
  row id); the portrait reuses that row's doll, race, sex, default class and voice. No table row is added, so no id
  can collide with game or mod rows, and a saved member keeps the base row's `characterDataId`.
- **Frames are ordinary icons.** The import writes `user_<id>-01.png ... -56.png` into the user data folder
  `portraits/icons_x2/`, which `AssetFileSystem::setUserIconDirectory` mounts as an extra icon root: searched after
  every icon package for unqualified `Data/icons` requests, never for package-qualified ones, so it cannot replace a
  game icon. Its `icons_x2` folder name gives the frames the x2 scale. The mount survives `switchActiveWorld` (a map
  of another world re-initialises the file system). HUD, menu and inventory loaders need no change.
- **The `user_` prefix** marks an imported face (`isCustomPortraitTextureName`). The member's `portraitTextureName` is
  `user_<id>-01`; the engine animates it like `pc02-01`.
- **A folder, not an archive.** The save import already picks a folder with `SDL_ShowOpenFolderDialog`; the same
  dialog and threading are reused, no dependency is added, and there is no archive extraction to secure. A shared ZIP
  is extracted by the player first (`tools/make_portrait_package.py --zip` makes one).
- **Missing portraits.** When a save names a `user_` face that is not installed (deleted, or a save from another
  computer), `availablePortraitTextureName` shows the base row's own face. The save keeps the imported name, so
  importing the package again brings the face back.

## Package format

A folder with:

```yaml
# portrait.yml
schema: 1
id: medusa              # 1-32 of a-z 0-9 _ ; file names derive from it
name: "Medusa"          # optional, shown in messages; at most 48 characters
base_character: 2       # character_data.txt row: doll, race, sex, default class and voice
```

and `01.png ... 56.png`: 118x158, 8-bit RGBA PNG. Other files in the folder are ignored.

## Import checks

`importCustomPortrait` refuses the package, writes nothing and tells the player why, when:

- `portrait.yml` is missing, larger than 64 KB, not YAML, not `schema: 1`, or a field has the wrong type;
- the id is not 1-32 of `a-z 0-9 _` (no paths);
- `base_character` is unknown or a row without a race (an NPC-only face);
- a frame is missing, a link, empty or over 1 MB, not a PNG, not 118x158, not 8-bit RGBA, or does not decode.

Files are read by fixed names only and written under names derived from the checked id. The frames and manifest are
written as `*.import` files first and renamed over the old ones only when all are written; the manifest goes last, so
an interrupted import is not offered. Importing an id again replaces that portrait. `loadCustomPortraits` skips a
manifest whose id does not match its file name or whose frames are not all present.

## In the game

- The picker offers each portrait wherever its base character is offered: the same race, classes and continent
  rules. After an import the new portrait is selected for the current adventurer; the other adventurers keep theirs.
- The created member gets `portraitTextureName = user_<id>-01`; everything else comes from the base row.
- Removing a portrait: delete `portraits/<id>.yml` (it is no longer offered) and its `portraits/icons_x2/user_<id>-*`
  files. There is no in-game delete in this first version.

## Not in this version

- Android and iOS import (content URIs).
- A cooked atlas for imported faces (56 small PNG files load fast enough; the HUD keeps loaded textures).
- A custom doll or voice: the base row provides them.
