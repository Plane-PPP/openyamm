#!/usr/bin/env python3
"""Exercise the release contract without relying on platform-specific tar output."""
from pathlib import Path
import struct
import tempfile
import zipfile

from verify_minotaur_windows_package import HOTFIX_FILES, REQUIRED, verify


with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    stage = root / "stage"
    (stage / "assets").mkdir(parents=True)
    source = root / "source"
    with zipfile.ZipFile(stage / "assets/engine.zip", "w") as engine:
        for name in HOTFIX_FILES:
            path = source / "assets_dev/engine" / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"resource")
            engine.writestr(name, b"resource")
    pe = bytearray(256)
    pe[:2] = b"MZ"
    struct.pack_into("<I", pe, 0x3c, 0x80)
    pe[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", pe, 0x84, 0x8664)
    package = root / "package.zip"
    for missing in (None, "assets/worlds/mm8.zip"):
        with zipfile.ZipFile(package, "w") as archive:
            for name in REQUIRED:
                if name != missing:
                    archive.writestr("OpenYAMM/" + name, pe if name == "openyamm.exe" else b"file")
        if missing is None:
            verify(package, stage, source)  # 1.5 deliberately has no mmmerge.zip.
        else:
            try:
                verify(package, stage, source)
            except ValueError as error:
                assert missing in str(error)
            else:
                raise AssertionError("Missing MM8 package accepted")
print("Package verification regression passed")
