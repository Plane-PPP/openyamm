#!/usr/bin/env python3
"""Validate the versioned Windows mod package and its content packs on OpenYAMM 1.10."""
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


def verify(path, stage, source, content_dir=None, thin=False):
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
    print("Windows x64 launcher/game, all content pack hashes, MM6/MM7/MM8 and embedded hotfix verified")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("--stage", required=True, type=Path)
    parser.add_argument("--content-dir", type=Path)
    parser.add_argument("--thin", action="store_true")
    args = parser.parse_args()
    verify(args.archive, args.stage, Path(__file__).resolve().parents[1], args.content_dir, args.thin)


if __name__ == "__main__":
    main()
