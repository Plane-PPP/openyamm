#!/usr/bin/env python3
"""Makes a party portrait package for the game's Add Portrait... button (devdocs/CUSTOM_PORTRAITS.md).

Input: a folder with the 56 face frames 01.png ... 56.png in any size with the portraits' 59:79 proportion (944x1264
from an image generator, or 118x158 already), RGBA with a transparent background.
Output: a package folder with portrait.yml and 01.png ... 56.png at 118x158 8-bit RGBA, which the game imports as is.
With --doll the package also carries the doll images of the base character it replaces: body.png (the body with the
portrait's head painted on) and hand_<name>.png, copied as doll/<file>; each must be the size of the base row's image
at x2 (pc02bod is 166x594), which the game checks on import.
With --zip the folder is also packed into <output>.zip (fixed timestamps: the same frames give the same bytes) for
sharing; players extract it and pick the folder.

    python3 tools/make_portrait_package.py frames/ --id medusa --name "Medusa" --base-character 2 --output medusa/

Needs Pillow (python3 -m pip install pillow). The base character is the character_data.txt row whose doll, race, sex
and default voice the portrait uses (2 is a human woman, for example); the game offers the portrait wherever it offers
that row.
"""

import argparse
import re
import sys
import zipfile
from pathlib import Path

WIDTH, HEIGHT = 118, 158
FRAMES = range(1, 57)
KEY_COLOURS = {(255, 0, 255), (0, 255, 255)}
ID = re.compile(r"^[a-z0-9_]{1,32}$")
DOLL_FILES = ["body.png"] + [f"hand_{name}.png" for name in
                             ("left_closed", "left_hold", "left_open", "right_fingers", "right_open", "right_hold")]


def clear_key_colours(frame):
    # The HUD makes pure #FF00FF and #00FFFF transparent; resampling leaves them in near-transparent edge pixels.
    pixels = frame.load()
    for y in range(frame.height):
        for x in range(frame.width):
            red, green, blue, _ = pixels[x, y]
            if (red, green, blue) in KEY_COLOURS:
                pixels[x, y] = (0, 0, 0, 0)


def yaml_string(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("frames", type=Path, help="folder with 01.png ... 56.png")
    parser.add_argument("--id", required=True, help="1-32 lower-case letters, digits or _")
    parser.add_argument("--name", help="the name players see; the id when omitted")
    parser.add_argument("--base-character", type=int, required=True, help="character_data.txt row id")
    parser.add_argument("--output", type=Path, required=True, help="package folder to create")
    parser.add_argument("--doll", type=Path, help="folder with body.png and/or hand_<name>.png for the doll")
    parser.add_argument("--zip", action="store_true", help="also write <output>.zip")
    args = parser.parse_args()
    try:
        from PIL import Image
    except ImportError:
        sys.exit("Pillow is required: python3 -m pip install pillow")

    if not ID.match(args.id):
        sys.exit(f"--id {args.id!r}: use 1-32 lower-case letters, digits or _")
    if args.base_character <= 0:
        sys.exit("--base-character must be a character_data.txt row id")
    name = " ".join((args.name or args.id).split())[:48]

    frames = []
    for number in FRAMES:
        path = args.frames / f"{number:02d}.png"
        if not path.is_file():
            sys.exit(f"Missing frame {path}")
        with Image.open(path) as image:
            width, height = image.size
            if abs(width / height - WIDTH / HEIGHT) > 0.01 * WIDTH / HEIGHT:
                sys.exit(f"{path.name}: {width}x{height} is not the portrait proportion 59:79")
            if image.mode not in ("RGBA", "LA", "PA") and "transparency" not in image.info:
                sys.exit(f"{path.name}: needs a transparent background (RGBA)")
            frame = image.convert("RGBA")
        if frame.size != (WIDTH, HEIGHT):
            frame = frame.resize((WIDTH, HEIGHT), Image.LANCZOS)
        clear_key_colours(frame)
        frames.append(frame)

    doll = []
    if args.doll:
        for name in DOLL_FILES:
            path = args.doll / name
            if not path.is_file():
                continue
            with Image.open(path) as image:
                if image.mode != "RGBA":
                    sys.exit(f"{path.name}: the doll images need a transparent background (RGBA)")
                image.load()
                doll.append((name, image.copy()))
        unknown = sorted(p.name for p in args.doll.iterdir() if p.is_file() and p.name not in DOLL_FILES)
        if unknown:
            sys.exit("Unknown doll files: " + ", ".join(unknown) + " (expected " + ", ".join(DOLL_FILES) + ")")
        if not doll:
            sys.exit(f"{args.doll} holds none of " + ", ".join(DOLL_FILES))

    args.output.mkdir(parents=True, exist_ok=True)
    manifest = (f"schema: 1\nid: {args.id}\nname: {yaml_string(name)}\n"
                f"base_character: {args.base_character}\n")
    (args.output / "portrait.yml").write_text(manifest, encoding="utf-8", newline="\n")
    for number, frame in zip(FRAMES, frames):
        frame.save(args.output / f"{number:02d}.png", optimize=True)
    if doll:
        (args.output / "doll").mkdir(exist_ok=True)
        for name, image in doll:
            clear_key_colours(image)
            image.save(args.output / "doll" / name, optimize=True)

    if args.zip:
        archive = args.output.with_suffix(".zip")
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as package:
            for path in sorted(p for p in args.output.rglob("*") if p.is_file()):
                relative = path.relative_to(args.output).as_posix()
                info = zipfile.ZipInfo(f"{args.output.name}/{relative}", date_time=(2026, 1, 1, 0, 0, 0))
                info.compress_type = zipfile.ZIP_DEFLATED
                info.external_attr = 0o644 << 16
                package.writestr(info, path.read_bytes())
        print(f"Wrote {archive}")
    print(f"Wrote {len(frames)} frames, {len(doll)} doll images and portrait.yml to {args.output}")


if __name__ == "__main__":
    main()
