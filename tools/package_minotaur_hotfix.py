#!/usr/bin/env python3
"""Build the small, deterministic resource overlay for the Windows v0.4 package."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import zipfile

BASE_SHA = "385a1e447aadb17cdf3b0db36691a92889f23786"
FILES = (
    "data_tables/npc.txt",
    "data_tables/npc_topic.txt",
    "ui/gameplay/character.yml",
)


def archive(files):
    result = io.BytesIO()
    with zipfile.ZipFile(result, "w", compression=zipfile.ZIP_DEFLATED) as out:
        for name, payload in sorted(files.items()):
            info = zipfile.ZipInfo(name, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            out.writestr(info, payload)
    return result.getvalue()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("docs/patches/Minotaur-v0.4-hotfix-1.zip"))
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    payloads = {"engine/" + name: (root / "assets_dev/engine" / name).read_bytes() for name in FILES}
    overlay = archive(payloads)
    manifest = {
        "patch": "Minotaur-v0.4-hotfix-1",
        "base_build_sha": BASE_SHA,
        "base_run": "https://github.com/Plane-PPP/openyamm/actions/runs/38784738765",
        "fixes": ["LS-001", "LS-002"],
        "executable_changed": False,
        "assets_zip_sha256": hashlib.sha256(overlay).hexdigest(),
        "files": {name: hashlib.sha256(data).hexdigest() for name, data in payloads.items()},
    }
    readme = (root / "docs/patches/README.md").read_bytes()
    package = archive({"assets/assets.zip": overlay, "MINOTAUR-HOTFIX-1-README.md": readme,
                       "MINOTAUR-HOTFIX-1.json": (json.dumps(manifest, indent=2) + "\n").encode()})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(package)
    print(f"{args.output}: {len(package)} bytes; sha256={hashlib.sha256(package).hexdigest()}")


if __name__ == "__main__":
    main()
