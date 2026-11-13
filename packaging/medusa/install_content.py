#!/usr/bin/env python3
"""Copies the Project Medusa ruleset package from a project-medusa checkout into assets_dev/engine/rulesets/medusa.

Development links the package (project-medusa/tools/link-dev.sh); a release build copies it, so the packs never
depend on a symbolic link. The copy refuses links, absolute or parent paths and anything outside the package folder.

    python packaging/medusa/install_content.py --source medusa-content --assets assets_dev
"""

import argparse
import json
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--source", type=Path, required=True, help="project-medusa checkout")
    parser.add_argument("--assets", type=Path, default=ROOT / "assets_dev")
    parser.add_argument("--config", type=Path, default=ROOT / "packaging/medusa/content.json")
    args = parser.parse_args()

    config = json.loads(args.config.read_text(encoding="utf-8"))
    package = args.source / config["package"]
    if not (package / "ruleset.yml").is_file():
        sys.exit(f"No Medusa ruleset package at {package}")
    for path in package.rglob("*"):
        if path.is_symlink():
            sys.exit(f"Symbolic link in the Medusa package: {path}")
    target = args.assets / "engine/rulesets/medusa"
    if target.is_symlink():
        sys.exit(f"{target} is a development link; remove it (tools/link-dev.sh --unlink) before a release copy")
    if target.exists():
        shutil.rmtree(target)
    shutil.copytree(package, target)
    files = sorted(p.relative_to(target).as_posix() for p in target.rglob("*") if p.is_file())
    print(f"Installed {len(files)} Medusa files into {target}")
    for name in files:
        print("  " + name)


if __name__ == "__main__":
    main()
