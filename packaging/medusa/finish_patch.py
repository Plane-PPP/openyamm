#!/usr/bin/env python3
"""Adds the Project Medusa notes to a Minotaur incremental update ZIP built by tools/minotaur_updates.py.

The update itself is unchanged (minotaur-update.json, payload, Apply-MinotaurUpdate.cmd): the same verified installer
applies it over the exact base version. This only adds, inside the update folder, the Medusa installation notes
(README-MEDUSA-RU.txt) and MEDUSA-VERSION.txt with the source commits and what the patch replaces or adds.

    python packaging/medusa/finish_patch.py --zip PATCH.zip --content-sha SHA --engine-sha SHA
"""

import argparse
import json
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--zip", type=Path, required=True)
    parser.add_argument("--engine-sha", required=True)
    parser.add_argument("--content-sha", required=True)
    parser.add_argument("--config", type=Path, default=ROOT / "packaging/medusa/content.json")
    args = parser.parse_args()
    config = json.loads(args.config.read_text(encoding="utf-8"))

    with zipfile.ZipFile(args.zip) as archive:
        names = archive.namelist()
        manifests = [name for name in names if name.endswith("/minotaur-update.json") and name.count("/") == 1]
        if len(manifests) != 1:
            raise SystemExit("Expected one update folder with minotaur-update.json")
        prefix = manifests[0].split("/")[0] + "/"
        manifest = json.loads(archive.read(manifests[0]))

    app_files = [entry["path"] for entry in manifest["app_files"]]
    lines = [
        "Project Medusa Alpha " + config["medusa_version"] + " (Windows patch)",
        "",
        "Required installed version (exact): " + manifest["base"]["version"],
        "  OpenYAMM upstream 1.20, Minotaur Rebalance build source " + manifest["base"]["source_sha"],
        "New version after installation:     " + manifest["target"]["version"],
        "",
        "Engine:  https://github.com/Plane-PPP/openyamm/commit/" + args.engine_sha,
        "Content: https://github.com/" + config["repository"] + "/commit/" + args.content_sha,
        "",
        "Replaces the program (app/<new version>/, the old one stays until the first healthy start):",
        *("  " + name for name in app_files),
        "",
        "Changed content packs (installed under assets/packs, unchanged packs are reused):",
        *("  " + name for name in manifest["changed_packs"]),
        "",
        "Unchanged: saves/, settings.ini, logs and the other " + str(len(manifest["reused_packs"])) + " content packs.",
        "Original Might and Magic VI-VIII resources are not part of this patch: the game reads the installed ones.",
    ]
    with zipfile.ZipFile(args.zip, "a", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr(prefix + "MEDUSA-VERSION.txt", "\r\n".join(lines) + "\r\n")
        archive.write(ROOT / "packaging/medusa/README-MEDUSA-RU.txt", prefix + "README-MEDUSA-RU.txt")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
