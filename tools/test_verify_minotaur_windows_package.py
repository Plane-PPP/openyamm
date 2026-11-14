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
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        packs.append({"id": package + ".core", "package": package,
                      "mount": "/engine" if package == "engine" else "/worlds/" + package,
                      "content_id": digest, "file": path.name, "size": path.stat().st_size, "sha256": digest})
    pe = bytearray(256)
    pe[:2] = b"MZ"
    struct.pack_into("<I", pe, 0x3c, 0x80)
    pe[0x80:0x84] = b"PE\0\0"
    struct.pack_into("<H", pe, 0x84, 0x8664)
    archive_path = root / "package.zip"
    def index(records):
        return {"schema": 1, "profile": "desktop", "content_format": 1, "packs": records}
    for missing in (None, "mm8"):
        document = index([p for p in packs if p["package"] != missing])
        with zipfile.ZipFile(archive_path, "w") as archive:
            for name in REQUIRED:
                data = (json.dumps(document).encode() if name == APP + "index.json"
                        else b"minotaur-1.10\n" if name == "app/current"
                        else pe if name.endswith(".exe") else b"file")
                archive.writestr("OpenYAMM/" + name, data)
            for pack in document["packs"]:
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
    # A thin archive is valid only with separately verified content; no large ZIP is required.
    with zipfile.ZipFile(archive_path, "w") as archive:
        for name in REQUIRED:
            archive.writestr("OpenYAMM/" + name, json.dumps(index(packs)).encode()
                             if name == APP + "index.json" else b"minotaur-1.10\n" if name == "app/current"
                             else pe if name.endswith(".exe") else b"file")
    verify(archive_path, stage, source, stage / "assets", thin=True)
    try:
        verify(archive_path, stage, source, thin=True)
    except ValueError as error:
        assert "external content" in str(error)
    else:
        raise AssertionError("Thin package accepted without resource verification")
    # Restore the complete index then reject an unexpected hotfix source revision.
    with zipfile.ZipFile(archive_path, "w") as archive:
        for name in REQUIRED:
            archive.writestr("OpenYAMM/" + name, json.dumps(index(packs)).encode()
                             if name == APP + "index.json" else b"minotaur-1.10\n" if name == "app/current"
                             else pe if name.endswith(".exe") else b"file")
        for pack in packs:
            archive.write(stage / "assets/packs" / pack["file"], "OpenYAMM/assets/packs/" + pack["file"])
    (source / "assets_dev/engine" / HOTFIX_FILES[0]).write_bytes(b"wrong revision")
    try:
        verify(archive_path, stage, source)
    except ValueError as error:
        assert "differs" in str(error)
    else:
        raise AssertionError("Wrong hotfix accepted")

# --full: the package must be the whole desktop release of its checkout (packs.yml worlds, languages, rulesets).
def full_case(drop=None, change=None):
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        stage, source = root / "stage", root / "source"
        (source / "tools/packaging").mkdir(parents=True)
        (source / "tools/packaging/packs.yml").write_text("worlds: [mm6, mm7, mm8, mm9]\n")
        files = {
            "assets_dev/languages/ru/language.yml": b"id: ru",
            "assets_dev/languages/ru/engine/fonts/a.ttf": b"font",
            "assets_dev/languages/templates/language.yml": b"template",
            "assets_dev/engine/rulesets/maw/ruleset.yml": b"id: maw",
            "assets_dev/engine/rulesets/maw/data/large.txt": b"not checked file for file",
            "assets_dev/engine/rulesets/medusa/ruleset.yml": b"id: medusa",
            "assets_dev/engine/rulesets/medusa/README.md": b"documentation, not shipped",
            "assets_dev/engine/rulesets/medusa/icons_x2/med01-01.png": b"face",
            "assets_dev/engine/rulesets/roguelike/ruleset.yml": b"id: roguelike",
            "assets_dev/engine/rulesets/roguelike/run.yml": b"rules: {}",
            "assets_dev/engine/rulesets/roguelike/icons_x2/med01-01.png": b"face",
        }
        for name in HOTFIX_FILES:
            files["assets_dev/engine/" + name] = b"resource"
        for name, data in files.items():
            (source / name).parent.mkdir(parents=True, exist_ok=True)
            (source / name).write_bytes(data)
        layout = {
            ("engine.core", "engine", "/engine", None): {**{n: b"resource" for n in HOTFIX_FILES},
                                                         "languages/ru/language.yml": b"id: ru"},
            ("engine.rulesets", "engine", "/engine/rulesets", None): {
                "maw/ruleset.yml": b"id: maw", "medusa/ruleset.yml": b"id: medusa",
                "medusa/icons_x2/med01-01.png": b"face", "roguelike/ruleset.yml": b"id: roguelike",
                "roguelike/run.yml": b"rules: {}", "roguelike/icons_x2/med01-01.png": b"face"},
            ("lang-ru.engine.fonts", "engine", "/engine/fonts", "ru"): {"a.ttf": b"font"},
            **{(w + ".core", w, "/worlds/" + w, None): {"manifest.txt": b"world"} for w in ("mm6", "mm7", "mm8", "mm9")},
        }
        (stage / "assets/packs").mkdir(parents=True)
        records = []
        for (pack_id, package, mount, language), entries in layout.items():
            if pack_id == drop:
                continue
            entries = dict(entries)
            if change and change[0] == pack_id:
                entries.pop(change[1])
            path = stage / "assets/packs" / (pack_id + ".zip")
            with zipfile.ZipFile(path, "w") as content:
                for name, data in entries.items():
                    content.writestr(name, data)
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            record = {"id": pack_id, "package": package, "mount": mount, "content_id": digest,
                      "file": path.name, "size": path.stat().st_size, "sha256": digest}
            if language:
                record["language"] = language
            records.append(record)
        document = {"schema": 1, "profile": "desktop", "content_format": 1, "packs": records}
        archive_path = root / "package.zip"
        with zipfile.ZipFile(archive_path, "w") as archive:
            for name in REQUIRED:
                archive.writestr("OpenYAMM/" + name, json.dumps(document).encode() if name == APP + "index.json"
                                 else b"minotaur-1.10\n" if name == "app/current"
                                 else pe if name.endswith(".exe") else b"file")
            for record in records:
                archive.write(stage / "assets/packs" / record["file"], "OpenYAMM/assets/packs/" + record["file"])
        verify(archive_path, stage, source, full=True)


full_case()
for drop, change, expected in (("mm9.core", None, "differ from the full release"),
                               ("lang-ru.engine.fonts", None, "optional packs of language ru"),
                               (None, ("engine.core", "languages/ru/language.yml"), "language text"),
                               (None, ("engine.rulesets", "roguelike/icons_x2/med01-01.png"), "ruleset file"),
                               (None, ("engine.rulesets", "maw/ruleset.yml"), "ruleset file")):
    try:
        full_case(drop, change)
    except ValueError as error:
        assert expected in str(error), (expected, str(error))
    else:
        raise AssertionError(f"Incomplete release accepted: {drop or change}")
print("Package verification regression passed")
