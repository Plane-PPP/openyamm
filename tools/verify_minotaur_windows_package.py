#!/usr/bin/env python3
"""Validate the versioned Windows mod package and its content packs on OpenYAMM 1.10."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile

APP = "app/minotaur-1.10/"
REQUIRED = (
    "OpenYAMM.exe", "app/current", APP + "openyamm.exe", APP + "index.json",
    "README.md", "CHANGELOG.md", "BUILD-NOTES.md", "BUILD-REVISION.txt",
    "docs/MINOTAUR_REBALANCE.md", "docs/ROADMAP.md",
    APP + "runtime/shaders/glsl/vs_outdoor_billboard_lit.bin",
    APP + "runtime/shaders/glsl/fs_sprite_atlas.bin",
    APP + "runtime/shaders/glsl/fs_model_emissive.bin",
)
HOTFIX_FILES = ("data_tables/npc.txt", "data_tables/npc_topic.txt", "ui/gameplay/character.yml")


def verify(path, stage, source):
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
        missing = ["OpenYAMM/" + name for name in REQUIRED if "OpenYAMM/" + name not in names]
        if missing:
            raise ValueError(f"Missing package members: {missing}")
        for name in REQUIRED:
            if archive.getinfo("OpenYAMM/" + name).file_size == 0:
                raise ValueError(f"Empty required package member: {name}")
        for name in ("OpenYAMM.exe", APP + "openyamm.exe"):
            with archive.open("OpenYAMM/" + name) as executable:
                header = executable.read(65536)
            pe = struct.unpack_from("<I", header, 0x3c)[0]
            if (header[:2] != b"MZ" or header[pe:pe + 4] != b"PE\0\0"
                    or struct.unpack_from("<H", header, pe + 4)[0] != 0x8664):
                raise ValueError(f"Executable is not Windows x64 PE: {name}")
        index = json.loads(archive.read("OpenYAMM/" + APP + "index.json"))
        if {p["package"] for p in index["packs"]} != {"engine", "mm6", "mm7", "mm8"}:
            raise ValueError("Content index must include engine and MM6/MM7/MM8")
        found = set()
        for pack in index["packs"]:
            relative = "assets/packs/" + pack["file"]
            if "OpenYAMM/" + relative not in names:
                raise ValueError(f"Missing content pack: {relative}")
            local = stage / relative
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
    args = parser.parse_args()
    verify(args.archive, args.stage, Path(__file__).resolve().parents[1])


if __name__ == "__main__":
    main()
