#!/usr/bin/env python3
"""A thin stage and separately packed content join into packages the Windows package verifier accepts."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zipfile

from assemble_minotaur_package import place_index, place_packs, write_package
from verify_minotaur_windows_package import APP_REQUIRED, HOTFIX_FILES, ROOT_REQUIRED, verify, verify_content

VERSION = "minotaur-2.3-0123456789ab-7-1"


def pe_header():
    header = bytearray(256)
    header[:2] = b"MZ"
    struct.pack_into("<I", header, 0x3c, 0x80)
    header[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", header, 0x84, 0x8664)
    return bytes(header)


def fixture(root):
    stage, content, source = root / "stage/OpenYAMM", root / "content", root / "source"
    for name in ROOT_REQUIRED + tuple(f"app/{VERSION}/{name}" for name in APP_REQUIRED if name != "index.json"):
        path = stage / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(pe_header() if name.endswith(".exe") else b"file")
    (stage / "app/current").write_text(VERSION + "\n")
    (source / "tools/packaging").mkdir(parents=True)
    (source / "tools/packaging/packs.yml").write_text("worlds: [mm6, mm7, mm8, mm9]\n")
    sources = {"assets_dev/languages/ru/language.yml": b"id: ru",
               "assets_dev/engine/rulesets/medusa/ruleset.yml": b"id: medusa",
               "assets_dev/engine/rulesets/medusa/icons_x2/med01-01.png": b"face",
               "assets_dev/engine/rulesets/roguelike/ruleset.yml": b"id: roguelike",
               "assets_dev/engine/rulesets/roguelike/icons_x2/med01-01.png": b"face",
               **{"assets_dev/engine/" + name: b"resource" for name in HOTFIX_FILES}}
    for name, data in sources.items():
        (source / name).parent.mkdir(parents=True, exist_ok=True)
        (source / name).write_bytes(data)
    layout = {("engine.core", "engine", "/engine"): {**{n: b"resource" for n in HOTFIX_FILES},
                                                     "languages/ru/language.yml": b"id: ru"},
              ("engine.rulesets", "engine", "/engine/rulesets"): {
                  "medusa/ruleset.yml": b"id: medusa", "medusa/icons_x2/med01-01.png": b"face",
                  "roguelike/ruleset.yml": b"id: roguelike", "roguelike/icons_x2/med01-01.png": b"face"},
              **{(w + ".core", w, "/worlds/" + w): {"world.txt": w.encode()} for w in ("mm6", "mm7", "mm8", "mm9")}}
    (content / "packs").mkdir(parents=True)
    records = []
    for (pack_id, package, mount), entries in layout.items():
        path = content / "packs" / (pack_id + ".zip")
        with zipfile.ZipFile(path, "w") as archive:
            for name, data in entries.items():
                archive.writestr(name, data)
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        records.append({"id": pack_id, "package": package, "mount": mount, "file": path.name,
                        "size": path.stat().st_size, "content_id": digest, "sha256": digest})
    (content / "index.json").write_text(json.dumps(
        {"schema": 1, "profile": "desktop", "content_format": 1, "release": VERSION, "build": "test",
         "packs": records}))
    return stage, content, source


with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    stage, content, source = fixture(root)
    verify_content(content, source)
    version, index = place_index(stage, content)
    assert version == VERSION
    placed = json.loads((stage / "app" / VERSION / "index.json").read_text())
    assert placed["store"] == "../../assets/packs" and placed["packs"] == index["packs"]
    write_package(stage, root / "thin.zip")
    verify(root / "thin.zip", stage, source, content, thin=True, full=True)
    place_packs(stage, content, index)
    write_package(stage, root / "full.zip")
    verify(root / "full.zip", stage, source, full=True)
    with zipfile.ZipFile(root / "full.zip") as archive:
        packs = [info for info in archive.infolist() if info.filename.startswith("OpenYAMM/assets/packs/")]
        assert len(packs) == len(index["packs"]) and all(i.compress_type == zipfile.ZIP_STORED for i in packs)
    # A damaged pack is refused before it reaches an installation.
    damaged = content / "packs" / index["packs"][0]["file"]
    damaged.write_bytes(damaged.read_bytes() + b"x")
    try:
        place_packs(stage, content, index)
    except ValueError as error:
        assert "damaged" in str(error)
    else:
        raise AssertionError("Damaged pack accepted")
    # The command line, as the workflow calls it.
    damaged.write_bytes(damaged.read_bytes()[:-1])
    result = subprocess.run([sys.executable, str(Path(__file__).with_name("assemble_minotaur_package.py")),
                             "--stage", str(stage), "--content", str(content),
                             "--full-output", str(root / "cli.zip")], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    assert "6 packs of engine, mm6, mm7, mm8, mm9" in result.stdout, result.stdout
print("Package assembly regression passed")
