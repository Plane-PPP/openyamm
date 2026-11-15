#!/usr/bin/env python3
"""Join a Windows program stage and separately packed content into the Minotaur packages.

The Windows job builds the thin stage without content (as upstream's .github/workflows/packages.yml does) while the
content job packs the resources on Linux; this script does what cmake/CreateOpenYammReleasePackage.cmake does with a
content directory: the index goes to app/<version>/index.json with its store at ../../assets/packs, and the full
installation carries the packs in assets/packs.

    python tools/assemble_minotaur_package.py --stage stage/OpenYAMM --content content --full-output full.zip
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import zipfile

from minotaur_updates import VERSION, read_json, safe_relative, validate_index


def place_index(stage, content):
    """Writes the content's index into the stage's app folder and returns (version, index)."""
    version = (stage / "app/current").read_text(encoding="utf-8-sig").strip()
    if not VERSION.fullmatch(version):
        raise ValueError(f"Invalid stage version: {version!r}")
    app = stage / "app" / version
    if not (app / "openyamm.exe").is_file() and not (app / "openyamm").is_file():
        raise ValueError(f"The stage has no game in app/{version}")
    index = read_json(content / "index.json")
    validate_index(index)
    index["store"] = "../../assets/packs"
    (app / "index.json").write_text(json.dumps(index, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    return version, index


def place_packs(stage, content, index):
    """Links (or copies) every indexed pack into the stage's assets/packs; nothing the index does not list."""
    target = stage / "assets/packs"
    target.mkdir(parents=True, exist_ok=True)
    for pack in index["packs"]:
        name = safe_relative(pack["file"])
        source, destination = content / "packs" / name, target / name
        if not source.is_file() or source.stat().st_size != pack["size"]:
            raise ValueError(f"Missing or damaged content pack: {name}")
        if destination.exists():
            destination.unlink()
        try:
            os.link(source, destination)
        except OSError:
            shutil.copyfile(source, destination)


def write_package(stage, output):
    """The installation's ZIP: every stage file under OpenYAMM/, the packs stored as they are."""
    output = Path(output)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".tmp")
    try:
        with zipfile.ZipFile(temporary, "w", allowZip64=True) as archive:
            for path in sorted(stage.rglob("*")):
                if path.is_symlink():
                    raise ValueError(f"The stage holds a symbolic link: {path}")
                if not path.is_file():
                    continue
                relative = safe_relative(path.relative_to(stage).as_posix())
                packed = relative.startswith("assets/packs/")
                archive.write(path, "OpenYAMM/" + relative,
                              compress_type=zipfile.ZIP_STORED if packed else zipfile.ZIP_DEFLATED)
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--stage", type=Path, required=True, help="The stage's OpenYAMM folder")
    parser.add_argument("--content", type=Path, required=True, help="Packed content: index.json and packs/")
    parser.add_argument("--thin-output", type=Path, help="Write the small program archive (no packs)")
    parser.add_argument("--full-output", type=Path, help="Write the full installation (with every pack)")
    args = parser.parse_args()
    version, index = place_index(args.stage, args.content)
    if args.thin_output:
        write_package(args.stage, args.thin_output)
    if args.full_output:
        place_packs(args.stage, args.content, index)
        write_package(args.stage, args.full_output)
    packages = sorted({pack["package"] for pack in index["packs"]})
    print(f"{version}: {len(index['packs'])} packs of {', '.join(packages)} joined to the stage")


if __name__ == "__main__":
    main()
