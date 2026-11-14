#!/usr/bin/env python3
"""Check the Minotaur Windows workflow against the release it must build, in seconds and before any build.

- The content job checks out at least what upstream's desktop content job does (.github/workflows/packages.yml) and
  every world tools/packaging/packs.yml ships, and packs them all (no --world or --engine-only narrowing).
- The Windows job checks out every shipped world, so the tests that read their data (MM9's) find it.
- Every scenario the workflow plays exists, and the test groups it runs are tools/minotaur_ci_tests.sh's.

    python3 tools/check_minotaur_workflow.py
"""
from pathlib import Path
import re
import sys

import yaml

ROOT = Path(__file__).resolve().parents[1]


def checkout_paths(job):
    for step in job["steps"]:
        uses = step.get("uses", "")
        if uses.startswith("actions/checkout@") and "repository" not in step.get("with", {}):
            return [line.strip() for line in str(step["with"].get("sparse-checkout", "")).splitlines() if line.strip()]
    raise ValueError(f"No checkout of this repository in job {job.get('name')}")


def run_text(job):
    return "\n".join(str(step.get("run", "")) for step in job["steps"])


def main():
    problems = []
    workflow = yaml.safe_load((ROOT / ".github/workflows/minotaur-windows.yml").read_text(encoding="utf-8"))
    upstream = yaml.safe_load((ROOT / ".github/workflows/packages.yml").read_text(encoding="utf-8"))
    worlds = yaml.safe_load((ROOT / "tools/packaging/packs.yml").read_text(encoding="utf-8"))["worlds"]
    jobs = workflow["jobs"]

    desktop = next(entry for entry in upstream["jobs"]["content"]["strategy"]["matrix"]["include"]
                   if entry["profile"] == "desktop")
    upstream_assets = [line.strip() for line in desktop["assets"].splitlines() if line.strip()]
    content = checkout_paths(jobs["content"])
    for path in upstream_assets + [f"/assets_dev/worlds/{world}/" for world in worlds]:
        if path not in content:
            problems.append(f"content job does not check out {path} (upstream's desktop release does)")
    packing = run_text(jobs["content"])
    if "package_runtime_assets.py" not in packing:
        problems.append("content job does not run tools/package_runtime_assets.py")
    for narrowing in ("--world", "--engine-only"):
        if narrowing in packing:
            problems.append(f"content job narrows the release with {narrowing}")
    if "--content-only" not in packing:
        problems.append("content job does not verify the full release (verify_minotaur_windows_package.py "
                        "--content-only)")

    windows = checkout_paths(jobs["windows"])
    for world in worlds:
        if f"/assets_dev/worlds/{world}/" not in windows:
            problems.append(f"windows job does not check out /assets_dev/worlds/{world}/ its tests read")

    text = (ROOT / ".github/workflows/minotaur-windows.yml").read_text(encoding="utf-8")
    for scenario in sorted(set(re.findall(r"\b((?:release|mm9|rulesets|shared_fx|turn_based)/[\w./-]+\.yml)", text))):
        if not (ROOT / "tests/scenarios" / scenario).is_file():
            problems.append(f"the workflow plays a missing scenario: tests/scenarios/{scenario}")
    groups = set(re.findall(r"^\s+([a-z0-9-]+)\)$", (ROOT / "tools/minotaur_ci_tests.sh").read_text(), re.M))
    for call in re.findall(r"minotaur_ci_tests\.sh[ \t]+\S+((?:[ \t]+[a-z0-9-]+)*)[ \t]*$", text, re.M):
        for group in call.split():
            if group not in groups:
                problems.append(f"the workflow runs an unknown test group: {group}")
    if "openyamm_unit_tests.exe --" in text or "openyamm_runtime_tests.exe --" in text:
        problems.append("a test filter is written into the workflow; keep it in tools/minotaur_ci_tests.sh")

    problems = list(dict.fromkeys(problems))
    for problem in problems:
        print("::error::" + problem)
    if problems:
        sys.exit(1)
    print(f"Workflow checked: content as upstream's desktop release plus {', '.join(worlds)}; "
          f"{len(groups)} test groups")


if __name__ == "__main__":
    main()
