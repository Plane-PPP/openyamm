#!/usr/bin/env python3
"""Validate the Windows package and embedded hotfix resources for the OpenYAMM 1.5 base."""
import argparse
from pathlib import Path
import struct
import zipfile

REQUIRED = (
    "openyamm.exe", "settings.ini", "README.md", "CHANGELOG.md", "BUILD-NOTES.md",
    "BUILD-REVISION.txt", "docs/MINOTAUR_REBALANCE.md", "docs/ROADMAP.md",
    "runtime/shaders/glsl/vs_outdoor_billboard_lit.bin",
    "runtime/shaders/glsl/fs_sprite_atlas.bin", "assets/engine.zip",
    "assets/worlds/mm6.zip", "assets/worlds/mm7.zip", "assets/worlds/mm8.zip",
)
HOTFIX_FILES = ("data_tables/npc.txt", "data_tables/npc_topic.txt", "ui/gameplay/character.yml")


def verify(path, stage, source):
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
        missing = ["OpenYAMM/" + name for name in REQUIRED if "OpenYAMM/" + name not in names]
        if missing:
            raise ValueError(f"Missing package members: {missing}; actual members: {sorted(names)}")
        for name in REQUIRED:
            if archive.getinfo("OpenYAMM/" + name).file_size == 0:
                raise ValueError(f"Empty required package member: {name}")
        with archive.open("OpenYAMM/openyamm.exe") as executable:
            header = executable.read(65536)
        pe = struct.unpack_from("<I", header, 0x3c)[0]
        if (header[:2] != b"MZ" or header[pe:pe + 4] != b"PE\0\0"
                or struct.unpack_from("<H", header, pe + 4)[0] != 0x8664):
            raise ValueError("Executable is not Windows x64 PE")
    with zipfile.ZipFile(stage / "assets/engine.zip") as engine:
        for name in HOTFIX_FILES:
            if engine.read(name) != (source / "assets_dev/engine" / name).read_bytes():
                raise ValueError(f"Embedded hotfix resource differs from checkout: {name}")
    print("Windows x64 package verified: engine + MM6/MM7/MM8, shaders, docs and embedded hotfix resources")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("--stage", required=True, type=Path)
    args = parser.parse_args()
    verify(args.archive, args.stage, Path(__file__).resolve().parents[1])


if __name__ == "__main__":
    main()
