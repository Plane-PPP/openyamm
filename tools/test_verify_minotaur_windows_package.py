#!/usr/bin/env python3
"""Check versioned package validation, missing worlds and hotfix resource rejection."""
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import zipfile
from verify_minotaur_windows_package import APP, HOTFIX_FILES, REQUIRED, verify

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    stage, source = root / "stage", root / "source"
    (stage / "assets/packs").mkdir(parents=True)
    packs = []
    for package in ("engine", "mm6", "mm7", "mm8"):
        path = stage / "assets/packs" / (package + ".zip")
        with zipfile.ZipFile(path, "w") as content:
            content.writestr("manifest.txt", "test")
            if package == "engine":
                for name in HOTFIX_FILES:
                    original = source / "assets_dev/engine" / name
                    original.parent.mkdir(parents=True, exist_ok=True)
                    original.write_bytes(b"resource")
                    content.writestr(name, b"resource")
        packs.append({"package": package, "file": path.name, "size": path.stat().st_size,
                      "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    pe = bytearray(256)
    pe[:2] = b"MZ"
    struct.pack_into("<I", pe, 0x3c, 0x80)
    pe[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", pe, 0x84, 0x8664)
    archive_path = root / "package.zip"
    for missing in (None, "mm8"):
        index = {"packs": [p for p in packs if p["package"] != missing]}
        with zipfile.ZipFile(archive_path, "w") as archive:
            for name in REQUIRED:
                data = (json.dumps(index).encode() if name == APP + "index.json"
                        else pe if name.endswith(".exe") else b"file")
                archive.writestr("OpenYAMM/" + name, data)
            for pack in index["packs"]:
                archive.write(stage / "assets/packs" / pack["file"], "OpenYAMM/assets/packs/" + pack["file"])
        if missing:
            try:
                verify(archive_path, stage, source)
            except ValueError as error:
                assert "MM6/MM7/MM8" in str(error)
            else:
                raise AssertionError("Missing MM8 accepted")
        else:
            verify(archive_path, stage, source)
    # Restore the complete index then reject an unexpected hotfix source revision.
    with zipfile.ZipFile(archive_path, "w") as archive:
        for name in REQUIRED:
            archive.writestr("OpenYAMM/" + name, json.dumps({"packs": packs}).encode()
                             if name == APP + "index.json" else pe if name.endswith(".exe") else b"file")
        for pack in packs:
            archive.write(stage / "assets/packs" / pack["file"], "OpenYAMM/assets/packs/" + pack["file"])
    (source / "assets_dev/engine" / HOTFIX_FILES[0]).write_bytes(b"wrong revision")
    try:
        verify(archive_path, stage, source)
    except ValueError as error:
        assert "differs" in str(error)
    else:
        raise AssertionError("Wrong hotfix accepted")
print("Package verification regression passed")
