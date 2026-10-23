#!/usr/bin/env python3
"""Build offline Minotaur updates using upstream's logical content-pack identities."""
import argparse
import copy
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import zipfile

SCHEMA = 1
CHANNEL = "minotaur"
ROOT = Path(__file__).resolve().parents[1]
SHA = re.compile(r"[0-9a-f]{64}\Z")
VERSION = re.compile(r"minotaur-[A-Za-z0-9](?:[A-Za-z0-9.-]*[A-Za-z0-9])?\Z")
PACK_FILE = re.compile(r"[A-Za-z0-9][A-Za-z0-9._-]*\.zip\Z")


def read_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8-sig"))


def write_json(path, value):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")


def digest_file(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def safe_relative(path):
    p = PurePosixPath(path)
    if (not path or p.is_absolute() or "\\" in path or ":" in path or "\x00" in path
            or any(part in ("", ".", "..") or part.endswith((".", " "))
                   or re.match(r"^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(?:\.|$)", part, re.I)
                   for part in path.split("/"))):
        raise ValueError(f"Unsafe relative path: {path!r}")
    return path


def validate_index(index):
    if (index.get("schema") != 1 or index.get("profile") != "desktop"
            or index.get("content_format") != 1 or not isinstance(index.get("packs"), list)):
        raise ValueError("Expected an upstream schema1 desktop content index")
    ids, files, packages = set(), set(), set()
    for pack in index["packs"]:
        package = pack["package"]
        mount = "/engine" if package == "engine" else "/worlds/" + package
        safe_relative(pack["id"])
        if (package not in {"engine", "mm6", "mm7", "mm8"}
                or not (pack["mount"] == mount or pack["mount"].startswith(mount + "/"))):
            raise ValueError("Unexpected package or mount")
        safe_relative(pack["mount"].lstrip("/"))
        safe_relative(pack["file"])
        if (not PACK_FILE.fullmatch(pack["file"]) or not SHA.fullmatch(pack["sha256"])
                or not SHA.fullmatch(pack["content_id"]) or type(pack["size"]) is not int
                or pack["size"] <= 0):
            raise ValueError("Invalid pack filename, digest or size")
        if pack["id"] in ids or pack["file"].casefold() in files:
            raise ValueError("Duplicate pack id or filename")
        ids.add(pack["id"])
        files.add(pack["file"].casefold())
        packages.add(package)
    if packages != {"engine", "mm6", "mm7", "mm8"}:
        raise ValueError("The mod requires engine and MM6/MM7/MM8")


def logical_pack(pack):
    return {k: pack[k] for k in ("id", "package", "mount", "content_id")}


def logical_index(index):
    validate_index(index)
    return {"schema": index["schema"], "profile": index["profile"],
            "content_format": index["content_format"],
            "packs": sorted((logical_pack(p) for p in index["packs"]), key=lambda p: p["id"])}


def index_identity(index):
    data = json.dumps(logical_index(index), sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(data).hexdigest()


def make_baseline(version, source_sha, index):
    if not VERSION.fullmatch(version) or not re.fullmatch(r"[0-9a-f]{40}", source_sha):
        raise ValueError("Expected a Minotaur version name and full commit SHA")
    validate_index(index)
    return {"schema": SCHEMA, "channel": CHANNEL, "version": version,
            "source_sha": source_sha, "index_identity": index_identity(index), "index": index}


def validate_baseline(base):
    if base.get("schema") != SCHEMA or base.get("channel") != CHANNEL:
        raise ValueError("Not a Minotaur update baseline")
    expected = make_baseline(base["version"], base["source_sha"], base["index"])
    if base.get("index_identity") != expected["index_identity"]:
        raise ValueError("Baseline content identity mismatch")


def plan_update(base, target):
    validate_baseline(base)
    validate_baseline(target)
    if base["version"].casefold() == target["version"].casefold():
        raise ValueError("Every update must use a new app version directory")
    previous = {p["id"]: p for p in base["index"]["packs"]}
    reused, changed = [], []
    for pack in target["index"]["packs"]:
        if pack["id"] in previous and logical_pack(pack) == logical_pack(previous[pack["id"]]):
            reused.append(pack["id"])
        else:
            changed.append(pack)
    return reused, changed


def resolve_index(installed, target, reused):
    """Keep installed ZIP bytes/digests even if a new compressor produced different bytes."""
    validate_index(installed)
    validate_index(target)
    result = copy.deepcopy(target)
    previous = {p["id"]: p for p in installed["packs"]}
    for pack in result["packs"]:
        if pack["id"] in reused:
            old = previous[pack["id"]]
            if logical_pack(old) != logical_pack(pack):
                raise ValueError("Installed logical content differs from the update base")
            for key in ("file", "size", "sha256"):
                pack[key] = old[key]
    result["store"] = "../../assets/packs"
    return result


def verify_file(path, record):
    path = Path(path)
    if not path.is_file() or path.stat().st_size != record["size"] or digest_file(path) != record["sha256"]:
        raise ValueError(f"Missing or damaged payload: {path.name}")


def build_update(stage, content, base, source_sha, output, baseline_output):
    stage, content, output = Path(stage), Path(content), Path(output)
    version = (stage / "app/current").read_text(encoding="utf-8-sig").strip()
    target = make_baseline(version, source_sha, read_json(stage / "app" / version / "index.json"))
    reused, changed = plan_update(base, target)
    app = stage / "app" / version
    app_files = []
    app_names = set()
    sources = {}
    for path in sorted(app.rglob("*")):
        if path.is_symlink():
            raise ValueError("App payload cannot contain symlinks")
        if not path.is_file():
            continue
        name = safe_relative(path.relative_to(app).as_posix())
        if name.casefold() in ("index.json", "healthy", "starts"):
            continue
        if name.casefold() in app_names:
            raise ValueError("App payload contains case-colliding paths")
        app_names.add(name.casefold())
        app_files.append({"path": name, "size": path.stat().st_size, "sha256": digest_file(path)})
        sources["payload/app/" + name] = path
    required = {"openyamm.exe", "OpenYAMM-launcher.exe"}
    if not required.issubset({p["path"] for p in app_files}):
        raise ValueError("App payload lacks game or launcher")
    for pack in changed:
        path = content / "packs" / pack["file"]
        verify_file(path, pack)
        sources["payload/packs/" + pack["file"]] = path
    manifest = {"schema": SCHEMA, "channel": CHANNEL, "base": base, "target": target,
                "reused_packs": reused, "changed_packs": [p["id"] for p in changed],
                "app_files": app_files,
                "changed_pack_bytes": sum(p["size"] for p in changed),
                "reused_pack_bytes": sum(p["size"] for p in base["index"]["packs"] if p["id"] in reused)}
    prefix = "Minotaur-Update-" + version + "/"
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".tmp")
    try:
        with zipfile.ZipFile(temporary, "w", compression=zipfile.ZIP_DEFLATED, allowZip64=True) as archive:
            archive.writestr(prefix + "minotaur-update.json", json.dumps(manifest, ensure_ascii=False, indent=2))
            for name in ("Apply-MinotaurUpdate.ps1", "Apply-MinotaurUpdate.cmd", "README-RU.txt"):
                archive.write(ROOT / "packaging/minotaur" / name, prefix + name)
            for name in ("CHANGELOG.md", "BUILD-NOTES.md", "docs/MINOTAUR_REBALANCE.md", "docs/ROADMAP.md"):
                if (stage / name).is_file():
                    archive.write(stage / name, prefix + name)
            for name, path in sorted(sources.items()):
                compression = zipfile.ZIP_STORED if name.startswith("payload/packs/") else zipfile.ZIP_DEFLATED
                archive.write(path, prefix + name, compress_type=compression)
        temporary.replace(output)
    finally:
        temporary.unlink(missing_ok=True)
    write_json(baseline_output, target)
    write_json(output.with_suffix(".json"), {"base": base["version"], "target": version,
               "source_sha": source_sha, "archive_bytes": output.stat().st_size,
               "archive_sha256": digest_file(output), "changed_packs": len(changed),
               "reused_packs": len(reused), "changed_pack_bytes": manifest["changed_pack_bytes"],
               "reused_pack_bytes": manifest["reused_pack_bytes"]})
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    build = sub.add_parser("build")
    for name in ("stage", "content", "base", "output", "baseline-output"):
        build.add_argument("--" + name, type=Path, required=True)
    build.add_argument("--source-sha", required=True)
    check = sub.add_parser("check-base")
    check.add_argument("path", type=Path)
    args = parser.parse_args()
    if args.command == "check-base":
        base = read_json(args.path)
        validate_baseline(base)
        print(base["version"] + " " + base["source_sha"])
    else:
        result = build_update(args.stage, args.content, read_json(args.base), args.source_sha,
                              args.output, args.baseline_output)
        print(f"Reused {len(result['reused_packs'])} packs; shipped {len(result['changed_packs'])} changed packs")


if __name__ == "__main__":
    main()
