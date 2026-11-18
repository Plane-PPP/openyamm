#!/usr/bin/env python3
"""Check the Minotaur Windows workflow against the release it must build, in seconds and before any build.

- The content job checks out at least what upstream's desktop content job does (.github/workflows/packages.yml) and
  every world tools/packaging/packs.yml ships, and packs them all (no --world or --engine-only narrowing).
- The content and Windows jobs check out every source path upstream's jobs build with (its content job; its Windows
  and Linux jobs, the latter with the tests), so a directory a new release adds (2.6's core/) is not left behind.
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


def covered(path, paths):
    """A sparse path is checked out when it is listed or a listed directory holds it (negations do not count)."""
    return any(path == entry or (entry.endswith("/") and path.startswith(entry))
               for entry in paths if not entry.startswith("!"))


def source_paths(job):
    """A job's checkout without its matrix's asset lines (`${{ matrix.assets }}`)."""
    return [path for path in checkout_paths(job) if not path.startswith("${{")]


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
    for path in source_paths(upstream["jobs"]["content"]):
        if not covered(path, content):
            problems.append(f"content job does not check out {path} (upstream's content job builds with it)")
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
    for upstream_job in ("windows", "linux"):
        for path in source_paths(upstream["jobs"][upstream_job]):
            if not covered(path, windows):
                problems.append(f"windows job does not check out {path} (upstream's {upstream_job} job builds with it)")
    for world in worlds:
        if f"/assets_dev/worlds/{world}/" not in windows:
            problems.append(f"windows job does not check out /assets_dev/worlds/{world}/ its tests read")

    if "/devdocs/maw/data/" not in windows:
        problems.append("windows job does not check out /devdocs/maw/data/ for the MAW formula regression tables")

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
