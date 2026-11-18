#!/usr/bin/env python3
"""Verify the additive public NoteX/upstream provenance transition."""
import os
import subprocess
from pathlib import Path

BASE = "dd40fe45598791e9aa38f3aeb200e10fb3d6338a"
SNAPSHOT = "0458e7536ecd7988089be6b3395bc5577fdbca94"
IMPORT = "e6c7cde6c9d0b90ee1a1e7ec78b3099d4d5be8a7"
MERGE = "baa4ccad3b06a57f69adb846e8ac2b47eb55f97f"
TREE = "e719ea979004c936228ffbe29fa75e8d7a85e37d"
ROOT = Path(__file__).resolve().parent.parent
ENV = dict(os.environ, GIT_NO_REPLACE_OBJECTS="1")


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT, env=ENV, text=True).strip()


def require(condition, message):
    if not condition:
        raise SystemExit(message)


def main():
    require(git("rev-parse", "--is-shallow-repository") == "false", "Use a full clone.")
    require(not git("for-each-ref", "--format=%(refname)", "refs/replace/"), "Replacement refs are not allowed.")
    grafts = Path(git("rev-parse", "--git-path", "info/grafts"))
    if not grafts.is_absolute():
        grafts = ROOT / grafts
    require(not grafts.exists() or not grafts.read_bytes().strip(), "Grafts are not allowed.")
    require(git("show", "-s", "--format=%P", IMPORT) == BASE, "Unexpected import parent.")
    require(git("show", "-s", "--format=%P", MERGE) == f"{SNAPSHOT} {IMPORT}", "Unexpected merge parents.")
    for commit in (SNAPSHOT, IMPORT, MERGE):
        require(git("rev-parse", f"{commit}^{{tree}}") == TREE, "The import changed the public snapshot tree.")
    for commit in (BASE, SNAPSHOT, IMPORT, MERGE):
        git("merge-base", "--is-ancestor", commit, "HEAD")
    print("PASS: full public upstream ancestry, additive transition, and identical import/merge trees.")


if __name__ == "__main__":
    main()
