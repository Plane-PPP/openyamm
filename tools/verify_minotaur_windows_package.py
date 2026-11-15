#!/usr/bin/env python3
"""Validate the versioned Windows mod package and its content packs.

With --full the package must also be a whole desktop release of its checkout: every package packs.yml ships (the
engine and every world, MM9 included), every language's text and optional packs, every ruleset, and this fork's own
rulesets (Project Medusa, the roguelike run) file for file with the heroine's portrait."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

from minotaur_updates import VERSION, validate_index

APP = "app/minotaur-1.10/"
ROOT_REQUIRED = (
    "OpenYAMM.exe", "app/current",
    "README.md", "CHANGELOG.md", "BUILD-NOTES.md", "BUILD-REVISION.txt",
    "docs/MINOTAUR_REBALANCE.md", "docs/ROADMAP.md",
)
APP_REQUIRED = ("openyamm.exe", "index.json", "OpenYAMM-launcher.exe",
                "runtime/shaders/glsl/vs_outdoor_billboard_lit.bin",
                "runtime/shaders/glsl/fs_sprite_atlas.bin", "runtime/shaders/glsl/fs_model_emissive.bin")
REQUIRED = ROOT_REQUIRED + tuple(APP + name for name in APP_REQUIRED)
HOTFIX_FILES = ("data_tables/npc.txt", "data_tables/npc_topic.txt", "ui/gameplay/character.yml")
# This fork's rulesets: shipped file for file (the packager leaves out only their .md documentation).
OWN_RULESETS = ("medusa", "roguelike")
PORTRAIT = "med01-01.png"


def expected_release(source):
    """(packages, {language: has optional files}) a full desktop release of the checkout ships."""
    import yaml  # only --full reads packs.yml; the installer tests run without PyYAML otherwise

    config = yaml.safe_load((source / "tools/packaging/packs.yml").read_text(encoding="utf-8"))
    worlds = list(config["worlds"])
    languages = {}
    root = source / "assets_dev/languages"
    # Folders are joined by name: under MSYS2's Python on Windows iterdir() joins with a backslash and rglob() with a
    # slash, and relative_to then takes a folder's own files for outside it.
    for folder in sorted(root / child.name for child in root.iterdir()):
        if folder.is_dir() and folder.name != "templates" and (folder / "language.yml").is_file():
            overlays = [p for p in folder.rglob("*") if p.is_file()
                        and p.relative_to(folder).parts[0] in ("engine", "worlds")
                        and (p.relative_to(folder).parts[0] == "engine" or p.relative_to(folder).parts[1] in worlds)]
            languages[folder.name] = bool(overlays)
    return {"engine", *worlds}, languages


def verify_full(index, content, source):
    packages, languages = expected_release(source)
    shipped = {pack["package"] for pack in index["packs"]}
    if shipped != packages:
        raise ValueError(f"Packages {sorted(shipped)} differ from the full release's {sorted(packages)}")
    # Every file the packs hold, by its virtual path (mount + name), for the base game (no language).
    files = {}
    for pack in index["packs"]:
        if "language" in pack:
            continue
        with zipfile.ZipFile(content / "packs" / pack["file"]) as archive:
            for name in archive.namelist():
                if not name.endswith("/"):
                    files[pack["mount"].lstrip("/") + "/" + name] = pack["file"]

    def packed(virtual):
        with zipfile.ZipFile(content / "packs" / files[virtual]) as archive:
            prefix = next(p["mount"].lstrip("/") + "/" for p in index["packs"] if p["file"] == files[virtual])
            return archive.read(virtual[len(prefix):])

    for language, optional in languages.items():
        if f"engine/languages/{language}/language.yml" not in files:
            raise ValueError(f"Missing language text: {language}")
        if optional and not any(pack.get("language") == language for pack in index["packs"]):
            raise ValueError(f"Missing optional packs of language {language}")
    rulesets = source / "assets_dev/engine/rulesets"
    for folder in sorted(rulesets / p.name for p in rulesets.iterdir() if (p / "ruleset.yml").is_file()):
        required = ([p for p in folder.rglob("*") if p.is_file() and p.suffix != ".md"]
                    if folder.name in OWN_RULESETS else [folder / "ruleset.yml"])
        for path in required:
            virtual = "engine/rulesets/" + path.relative_to(rulesets).as_posix()
            if virtual not in files:
                raise ValueError(f"Missing ruleset file: {virtual}")
            if packed(virtual) != path.read_bytes():
                raise ValueError(f"Ruleset file differs from checkout: {virtual}")
    for ruleset in OWN_RULESETS:
        if f"engine/rulesets/{ruleset}/run.yml" not in files and f"engine/rulesets/{ruleset}/ruleset.yml" not in files:
            raise ValueError(f"Missing ruleset: {ruleset}")
        if f"engine/rulesets/{ruleset}/icons_x2/{PORTRAIT}" not in files:
            raise ValueError(f"Missing the Medusa heroine's portrait in ruleset {ruleset}")
    return sorted(packages), sorted(languages)


def verify_content(content, source):
    """Packed content alone (the content job, before any installation exists): its index, every pack's hash and size
    and the full release's composition."""
    index = json.loads((Path(content) / "index.json").read_text(encoding="utf-8"))
    validate_index(index)
    for pack in index["packs"]:
        local = Path(content) / "packs" / pack["file"]
        with local.open("rb") as stream:
            digest = hashlib.file_digest(stream, "sha256").hexdigest()
        if digest != pack["sha256"] or local.stat().st_size != pack["size"]:
            raise ValueError(f"Content pack hash/size mismatch: {pack['file']}")
    packages, languages = verify_full(index, Path(content), source)
    size = sum(pack["size"] for pack in index["packs"])
    print(f"Content verified: {len(index['packs'])} packs, {size / 2**30:.2f} GiB; {', '.join(packages)}; "
          f"languages {', '.join(languages)}; rulesets {', '.join(OWN_RULESETS)} with the heroine's portrait")


def verify(path, stage, source, content_dir=None, thin=False, full=False):
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
        missing = ["OpenYAMM/" + name for name in ROOT_REQUIRED if "OpenYAMM/" + name not in names]
        if missing:
            raise ValueError(f"Missing package members: {missing}")
        version = archive.read("OpenYAMM/app/current").decode("utf-8-sig").strip()
        if not VERSION.fullmatch(version):
            raise ValueError("Invalid version directory")
        app = "app/" + version + "/"
        required = ROOT_REQUIRED + tuple(app + name for name in APP_REQUIRED)
        missing = ["OpenYAMM/" + name for name in required if "OpenYAMM/" + name not in names]
        if missing:
            raise ValueError(f"Missing package members: {missing}")
        for name in required:
            if archive.getinfo("OpenYAMM/" + name).file_size == 0:
                raise ValueError(f"Empty required package member: {name}")
        for name in ("OpenYAMM.exe", app + "openyamm.exe", app + "OpenYAMM-launcher.exe"):
            with archive.open("OpenYAMM/" + name) as executable:
                header = executable.read(65536)
            pe = struct.unpack_from("<I", header, 0x3c)[0]
            if (header[:2] != b"MZ" or header[pe:pe + 4] != b"PE\0\0"
                    or struct.unpack_from("<H", header, pe + 4)[0] != 0x8664):
                raise ValueError(f"Executable is not Windows x64 PE: {name}")
        index = json.loads(archive.read("OpenYAMM/" + app + "index.json"))
        validate_index(index)
        if thin and content_dir is None:
            raise ValueError("Thin package verification requires the external content directory")
        if thin and any(name.startswith("OpenYAMM/assets/packs/") for name in names):
            raise ValueError("Thin package unexpectedly contains resource packs")
        content = Path(content_dir) if content_dir is not None else stage / "assets"
        found = set()
        for pack in index["packs"]:
            relative = "assets/packs/" + pack["file"]
            if not thin and "OpenYAMM/" + relative not in names:
                raise ValueError(f"Missing content pack: {relative}")
            local = content / "packs" / pack["file"]
            with local.open("rb") as stream:
                digest = hashlib.file_digest(stream, "sha256").hexdigest()
            if digest != pack["sha256"] or local.stat().st_size != pack["size"]:
                raise ValueError(f"Content pack hash/size mismatch: {relative}")
            if pack["package"] == "engine":
                with zipfile.ZipFile(local) as engine:
                    for name in HOTFIX_FILES:
                        if name in engine.namelist():
                            if engine.read(name) != (source / "assets_dev/engine" / name).read_bytes():
                                raise ValueError(f"Embedded hotfix resource differs from checkout: {name}")
                            found.add(name)
        if found != set(HOTFIX_FILES):
            raise ValueError(f"Missing embedded hotfix resources: {set(HOTFIX_FILES) - found}")
        if full:
            packages, languages = verify_full(index, content, source)
            print(f"Full release verified: {', '.join(packages)}; languages {', '.join(languages)}; "
                  f"rulesets {', '.join(OWN_RULESETS)} with the heroine's portrait")
    print("Windows x64 launcher/game, all content pack hashes and embedded hotfix verified")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path, nargs="?")
    parser.add_argument("--stage", type=Path)
    parser.add_argument("--content-only", type=Path, help="Check packed content alone (index.json and packs/)")
    parser.add_argument("--content-dir", type=Path)
    parser.add_argument("--thin", action="store_true")
    parser.add_argument("--full", action="store_true", help="Require the whole desktop release of the checkout")
    args = parser.parse_args()
    if args.content_only:
        verify_content(args.content_only, Path(__file__).resolve().parents[1])
        return
    if args.archive is None or args.stage is None:
        parser.error("an archive and --stage are required unless --content-only is given")
    verify(args.archive, args.stage, Path(__file__).resolve().parents[1], args.content_dir, args.thin, args.full)


if __name__ == "__main__":
    main()
