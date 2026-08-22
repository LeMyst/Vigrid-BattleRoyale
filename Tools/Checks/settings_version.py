"""
 *  Settings-class migration discipline. Diff-scoped: this one asks about the CHANGE, not the tree.
 *
 *  Every BattleRoyale*Data class carries `int version` plus an `Upgrade()` migration, and Load()
 *  re-saves after reading so new fields appear in existing profile JSONs on the next boot. Adding
 *  a field without bumping the version skips that migration.
 *
 *  For a `ref array` field the Upgrade() branch is NOT OPTIONAL, and this is the silent one: a
 *  field initialiser survives deserialization for scalars but not for arrays. On every server that
 *  already has the JSON on disk, the array loads back EMPTY and the feature does nothing at all -
 *  no error, no warning, and it works perfectly on a fresh server, which is where it gets tested.
 *  BattleRoyaleGameData and BattleRoyaleServerData.placeholder_player_names both refill in
 *  Upgrade(), and only when the array comes back empty, so an admin who deliberately cleared it
 *  keeps their choice.
 *
 *  Compares against the merge base with origin/main. With no origin/main available - a fresh
 *  clone, a detached checkout - the check reports nothing rather than guessing; it is advisory by
 *  construction, and the tree-wide checks are the ones that must always run.
"""

from __future__ import annotations

import re
import subprocess

from Checks._source import Finding, error, read, repo_root, strip_code, warn

NAME = "settings-version"
SUMMARY = "a new settings field bumps version; a new ref array gets an Upgrade() branch"

CONFIG_DIR = "Scripts/Server/3_Game/Config/"
BASE_CANDIDATES = ("origin/main", "main")

MEMBER = re.compile(
    r"^\+\s*(?:ref\s+)?(?P<type>array<[^>]*>|map<[^>]*>|int|float|bool|string|vector)\s+"
    r"(?P<name>[A-Za-z_]\w*)\s*(?:=|;)"
)
HUNK = re.compile(r"^@@ -\d+(?:,\d+)? \+(?P<start>\d+)(?:,\d+)? @@")
VERSION_TOUCHED = re.compile(r"^\+.*\bversion\s*=\s*\d+")
UPGRADE_TOUCHED = re.compile(r"^\+.*\bversion\s*<\s*\d+")


def git(*args: str) -> tuple[int, str]:
    # `text=True` alone decodes with the LOCALE codec, which on a Windows box is cp1252 - so the
    # first non-ASCII byte in a diffed settings file raised UnicodeDecodeError inside subprocess's
    # reader thread, and `run()` then returned stdout=None for the check to crash on. The repo's
    # sources are UTF-8, so say so. errors="replace" because this output is scanned for `+` lines
    # and version numbers: a mangled character in a comment must not take the whole check down.
    result = subprocess.run(
        ["git", "-C", str(repo_root()), *args],
        capture_output=True, text=True, encoding="utf-8", errors="replace")
    return result.returncode, result.stdout or ""


def merge_base() -> str | None:
    for candidate in BASE_CANDIDATES:
        code, _ = git("rev-parse", "--verify", "--quiet", candidate)
        if code != 0:
            continue
        code, out = git("merge-base", "HEAD", candidate)
        if code == 0 and out.strip():
            return out.strip()
    return None


def class_body_lines(path: str) -> set[int]:
    """1-based line numbers of `path` that sit at class-body brace depth.

    A settings *field* is declared directly in the class body (depth 1); anything deeper is a local
    inside a method. Without this the MEMBER pattern cannot tell the two apart, and any re-added
    local in one of these files reads as a brand-new settings field.
    """
    try:
        text = strip_code(read(path))
    except OSError:
        return set()

    at_depth: set[int] = set()
    depth = 0
    for number, line in enumerate(text.splitlines(), start=1):
        if depth == 1:
            at_depth.add(number)
        depth += line.count("{") - line.count("}")
    return at_depth


def run() -> list[Finding]:
    base = merge_base()
    if base is None:
        return []

    code, out = git("diff", "--unified=0", base, "--", f"{CONFIG_DIR}BattleRoyale*Data.c")
    if code != 0 or not out.strip():
        return []

    findings: list[Finding] = []
    path: str | None = None
    added_members: list[tuple[str, str]] = []
    bumped = False
    upgraded = False

    def flush() -> None:
        if path is None or not added_members:
            return
        names = ", ".join(f"`{n}`" for _, n in added_members)
        if not bumped:
            findings.append(error(
                f"adds {names} but does not bump `version` - Upgrade() will not run, so the new "
                f"field never reaches a profile JSON that already exists",
                path,
            ))
        arrays = [n for t, n in added_members if t.startswith(("array<", "map<"))]
        if arrays and not upgraded:
            listed = ", ".join(f"`{n}`" for n in arrays)
            findings.append(error(
                f"adds the collection field(s) {listed} with no new Upgrade() branch - a field "
                f"initialiser does NOT survive deserialization for arrays, so on every server "
                f"that already has this JSON the field loads back empty and the feature silently "
                f"does nothing; refill it in Upgrade(), and only when it comes back empty",
                path,
            ))

    fields: set[int] = set()
    number = 0

    for line in out.splitlines():
        if line.startswith("+++ b/"):
            flush()
            path = line[len("+++ b/"):].strip()
            added_members = []
            bumped = False
            upgraded = False
            fields = class_body_lines(path)
            continue
        hunk = HUNK.match(line)
        if hunk:
            number = int(hunk.group("start"))
            continue
        if path is None or not line.startswith("+") or line.startswith("+++"):
            continue
        if VERSION_TOUCHED.search(line):
            bumped = True
        if UPGRADE_TOUCHED.search(line):
            upgraded = True
        match = MEMBER.match(line)
        #--- Class-body depth only. A local re-added by an unrelated edit is not a settings field.
        if match and number in fields:
            added_members.append((match.group("type"), match.group("name")))
        number += 1

    flush()
    return findings
