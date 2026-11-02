#!/usr/bin/env python3
"""Check the Minotaur RU/EN documentation pairs. Standard library only; reads files, changes nothing.

For every docs/**/*.en.md and the RU file next to it, report:
- a missing RU or EN partner, or a missing language link in either version;
- local Markdown links whose target is neither on disk nor tracked by git (sparse-hidden files count as present);
- inline `code` spans and URLs present in only one version, which often means a formula or a reference drifted;
- documents git ignores and does not track, which means a .gitignore rule hides them again.

Exit status 1 only for missing partners, language links or broken local links. Drift and ignored files are warnings.
"""
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
LINK = re.compile(r"\[[^\]]*\]\(([^)\s]+)\)")
CODE = re.compile(r"`([^`\n]+)`")
URL = re.compile(r"https?://[^\s)>\]]+")
FENCE = re.compile(r"```.*?```", re.S)


def git_lines(*args):
    output = subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True, check=True).stdout
    return output.splitlines()


def references(text):
    return set(CODE.findall(FENCE.sub("", text))) | set(URL.findall(text))


def main():
    tracked = set(git_lines("ls-files"))
    errors = []
    warnings = []
    english = sorted(ROOT.glob("docs/**/*.en.md"))

    for en_path in english:
        ru_path = en_path.with_name(en_path.name[: -len(".en.md")] + ".md")
        en_rel = en_path.relative_to(ROOT).as_posix()
        ru_rel = ru_path.relative_to(ROOT).as_posix()
        documents = {en_rel: en_path}

        if ru_path.exists():
            documents[ru_rel] = ru_path
        elif ru_rel in tracked:
            warnings.append(f"{ru_rel}: hidden by sparse checkout; add the language link when it is materialized")
        else:
            errors.append(f"{en_rel}: no Russian partner {ru_rel}")

        if f"({ru_path.name})" not in en_path.read_text(encoding="utf-8"):
            errors.append(f"{en_rel}: no language link to {ru_path.name}")

        if ru_path.exists():
            ru_text = ru_path.read_text(encoding="utf-8")
            en_text = en_path.read_text(encoding="utf-8")
            if f"({en_path.name})" not in ru_text:
                errors.append(f"{ru_rel}: no language link to {en_path.name}")
            for item in sorted(references(ru_text) ^ references(en_text)):
                side = "RU only" if item in references(ru_text) else "EN only"
                warnings.append(f"{en_rel}: reference {side}: {item}")

        for rel, path in documents.items():
            for target in LINK.findall(path.read_text(encoding="utf-8")):
                if re.match(r"[a-z]+:", target) or target.startswith("#"):
                    continue
                resolved = (path.parent / target.split("#")[0]).resolve()
                target_rel = resolved.relative_to(ROOT).as_posix() if resolved.is_relative_to(ROOT) else target
                if not resolved.exists() and target_rel not in tracked:
                    errors.append(f"{rel}: broken link {target}")

    candidates = [path.relative_to(ROOT).as_posix() for path in ROOT.glob("docs/**/*.md")]
    candidates = [path for path in candidates if path not in tracked]
    if candidates:
        # Without --no-index: the sparse-hidden .gitignore is only readable through the index.
        check = subprocess.run(["git", "check-ignore", *candidates], cwd=ROOT, capture_output=True, text=True)
        for path in check.stdout.splitlines():
            warnings.append(f"{path}: ignored and untracked; check .gitignore")

    for line in warnings:
        print(f"warning: {line}")
    for line in errors:
        print(f"error: {line}")
    print(f"{len(english)} pairs, {len(errors)} errors, {len(warnings)} warnings")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
