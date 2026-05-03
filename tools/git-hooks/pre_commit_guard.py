#!/usr/bin/env python3
"""Repo-local pre-commit checks for the Unreal project."""

from __future__ import annotations

import subprocess
import sys
from pathlib import PurePosixPath


ROOT_BLOCKED_DIRS = {"Binaries", "Intermediate", "DerivedDataCache", "Saved", ".vs"}
PLUGIN_BLOCKED_DIRS = {"Binaries", "Intermediate"}
BLOCKED_SUFFIXES = {".sln", ".suo", ".VC.db", ".opendb"}
BLOCKED_BASENAMES = {"Thumbs.db", ".DS_Store"}


def run_git(*args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", *args],
        capture_output=True,
        text=True,
        check=False,
    )


def staged_paths() -> list[str]:
    result = run_git("diff", "--cached", "--name-only", "--diff-filter=ACMR", "-z")
    if result.returncode != 0:
        sys.stderr.write(result.stderr)
        raise SystemExit(result.returncode)
    return [path for path in result.stdout.split("\0") if path]


def block_reason(path: str) -> str | None:
    normalized = path.replace("\\", "/")
    pure = PurePosixPath(normalized)
    parts = pure.parts
    if not parts:
        return None

    if parts[0] in ROOT_BLOCKED_DIRS:
        return f"generated Unreal output under '{parts[0]}/' should not be committed"

    if (
        len(parts) >= 3
        and parts[0] == "Plugins"
        and parts[2] in PLUGIN_BLOCKED_DIRS
    ):
        return f"generated plugin output under 'Plugins/*/{parts[2]}/' should not be committed"

    if pure.name in BLOCKED_BASENAMES:
        return f"workspace noise file '{pure.name}' should not be committed"

    for suffix in BLOCKED_SUFFIXES:
        if normalized.endswith(suffix):
            return f"generated IDE file '*{suffix}' should not be committed"

    return None


def main() -> int:
    failures: list[str] = []
    paths = staged_paths()

    for path in paths:
        reason = block_reason(path)
        if reason:
            failures.append(f"{path}: {reason}")

    whitespace = run_git("diff", "--cached", "--check")
    if whitespace.returncode != 0:
        details = (whitespace.stdout + whitespace.stderr).strip()
        if details:
            failures.append("git diff --cached --check reported issues:\n" + details)
        else:
            failures.append("git diff --cached --check reported whitespace or conflict-marker issues")

    if failures:
        sys.stderr.write("pre-commit guard failed:\n")
        for failure in failures:
            sys.stderr.write(f"- {failure}\n")
        return 1

    if paths:
        sys.stdout.write(f"pre-commit guard OK ({len(paths)} staged file(s) checked).\n")
    else:
        sys.stdout.write("pre-commit guard OK (no staged files).\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
