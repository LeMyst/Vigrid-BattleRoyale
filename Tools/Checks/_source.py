"""
 *  Shared plumbing for the static checks.
 *
 *  Two rules here are load-bearing and everything else is convenience.
 *
 *  1. Files come from `git ls-files`, never from a filesystem walk. A plain recursive glob
 *     descends into `.claude/worktrees/`, and an in-progress worktree there will report its own
 *     half-finished sources as violations of the checked-in tree. This is the same rule
 *     `Workbench/Batchfiles/_EnumPaths.bat` already applies to the build, for the same reason.
 *
 *  2. Identifier matching runs on `strip_code()` output, never on the raw text. Nearly every
 *     apparent violation in this repo is a doc comment *explaining* the rule it appears to break,
 *     or an asset-path string constant that legitimately contains the mod name. Without the
 *     stripper the discipline and ternary checks are pure noise.
"""

from __future__ import annotations

import subprocess
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

ERROR = "error"
WARNING = "warning"


@dataclass(frozen=True)
class Finding:
    level: str
    message: str
    path: str | None = None
    line: int | None = None

    def location(self) -> str:
        if self.path is None:
            return ""
        if self.line is None:
            return self.path
        return f"{self.path}:{self.line}"


def error(message: str, path: str | None = None, line: int | None = None) -> Finding:
    return Finding(ERROR, message, path, line)


def warn(message: str, path: str | None = None, line: int | None = None) -> Finding:
    return Finding(WARNING, message, path, line)


# --------------------------------------------------------------------------------------------
#  Repository access
# --------------------------------------------------------------------------------------------

@lru_cache(maxsize=1)
def repo_root() -> Path:
    out = subprocess.run(
        ["git", "rev-parse", "--show-toplevel"],
        capture_output=True, text=True, check=True,
    )
    return Path(out.stdout.strip())


@lru_cache(maxsize=None)
def tracked(*patterns: str) -> tuple[str, ...]:
    """Repo-relative POSIX paths of tracked files matching the given git pathspecs.

    With no patterns, every tracked file. Results are cached, so checks may call this freely.
    """
    cmd = ["git", "-C", str(repo_root()), "ls-files", "-z", "--"]
    cmd.extend(patterns if patterns else ["."])
    out = subprocess.run(cmd, capture_output=True, text=True, check=True)
    return tuple(sorted(p for p in out.stdout.split("\0") if p))


def scripts() -> tuple[str, ...]:
    """Every tracked EnfusionScript file."""
    return tracked("*.c")


def exists(rel: str) -> bool:
    return (repo_root() / rel).is_file()


def read_bytes(rel: str) -> bytes:
    return (repo_root() / rel).read_bytes()


def read(rel: str) -> str:
    """Text of a tracked file. Undecodable bytes are replaced rather than raised - a check
    should report a finding about a bad file, never crash the whole run on it."""
    return read_bytes(rel).decode("utf-8", errors="replace")


# --------------------------------------------------------------------------------------------
#  Comment and string stripping
# --------------------------------------------------------------------------------------------

_CODE, _LINE_COMMENT, _BLOCK_COMMENT, _STRING = range(4)


def strip_code(text: str) -> str:
    """Blank out `//` comments, `/* */` comments and the contents of string literals.

    Length and line structure are preserved exactly - every removed character becomes a space and
    newlines are kept - so a line number taken from the result indexes the original file. String
    *delimiters* survive, so a check can still tell that a literal was there; only the contents go.
    """
    out = []
    state = _CODE
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""

        if state == _CODE:
            if ch == "/" and nxt == "/":
                state = _LINE_COMMENT
                out.append("  ")
                i += 2
                continue
            if ch == "/" and nxt == "*":
                state = _BLOCK_COMMENT
                out.append("  ")
                i += 2
                continue
            if ch == '"':
                state = _STRING
                out.append('"')
                i += 1
                continue
            out.append(ch)
            i += 1
            continue

        if state == _LINE_COMMENT:
            if ch == "\n":
                state = _CODE
                out.append("\n")
            else:
                out.append(" ")
            i += 1
            continue

        if state == _BLOCK_COMMENT:
            if ch == "*" and nxt == "/":
                state = _CODE
                out.append("  ")
                i += 2
                continue
            out.append("\n" if ch == "\n" else " ")
            i += 1
            continue

        # _STRING
        if ch == "\\" and nxt:
            out.append("  ")
            i += 2
            continue
        if ch == '"':
            state = _CODE
            out.append('"')
            i += 1
            continue
        out.append("\n" if ch == "\n" else " ")
        i += 1

    return "".join(out)


def strip_comments(text: str) -> str:
    """Blank out comments but KEEP string contents.

    The config checks need the strings - `requiredAddons[] = {"DZ_Data"}` is entirely string
    values - so strip_code() is the wrong tool there. Length and lines are preserved as above.
    """
    out = []
    state = _CODE
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""

        if state == _CODE:
            if ch == "/" and nxt == "/":
                state = _LINE_COMMENT
                out.append("  ")
                i += 2
                continue
            if ch == "/" and nxt == "*":
                state = _BLOCK_COMMENT
                out.append("  ")
                i += 2
                continue
            if ch == '"':
                state = _STRING
            out.append(ch)
            i += 1
            continue

        if state == _LINE_COMMENT:
            if ch == "\n":
                state = _CODE
                out.append("\n")
            else:
                out.append(" ")
            i += 1
            continue

        if state == _BLOCK_COMMENT:
            if ch == "*" and nxt == "/":
                state = _CODE
                out.append("  ")
                i += 2
                continue
            out.append("\n" if ch == "\n" else " ")
            i += 1
            continue

        # _STRING - copied through verbatim
        if ch == "\\" and nxt:
            out.append(ch)
            out.append(nxt)
            i += 2
            continue
        if ch == '"':
            state = _CODE
        out.append(ch)
        i += 1

    return "".join(out)


def string_literals(text: str) -> list[tuple[int, str]]:
    """Every double-quoted literal in `text`, as (line number, contents).

    The inverse of strip_code: checks that validate asset paths want the strings and nothing else.
    """
    found: list[tuple[int, str]] = []
    state = _CODE
    line = 1
    buf: list[str] = []
    start_line = 1
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        nxt = text[i + 1] if i + 1 < n else ""

        if ch == "\n":
            line += 1

        if state == _CODE:
            if ch == "/" and nxt == "/":
                state = _LINE_COMMENT
                i += 2
                continue
            if ch == "/" and nxt == "*":
                state = _BLOCK_COMMENT
                i += 2
                continue
            if ch == '"':
                state = _STRING
                buf = []
                start_line = line
                i += 1
                continue
            i += 1
            continue

        if state == _LINE_COMMENT:
            if ch == "\n":
                state = _CODE
            i += 1
            continue

        if state == _BLOCK_COMMENT:
            if ch == "*" and nxt == "/":
                state = _CODE
                i += 2
                continue
            i += 1
            continue

        # _STRING
        if ch == "\\" and nxt:
            buf.append(nxt)
            i += 2
            continue
        if ch == '"':
            state = _CODE
            found.append((start_line, "".join(buf)))
            i += 1
            continue
        buf.append(ch)
        i += 1

    return found


def numbered(text: str):
    """Yield (line number, line text) pairs, 1-indexed."""
    for idx, line in enumerate(text.splitlines(), start=1):
        yield idx, line


# --------------------------------------------------------------------------------------------
#  Allowlists
# --------------------------------------------------------------------------------------------

def allowlist(name: str) -> dict[str, str]:
    """Load `Tools/allowlists/<name>.txt` as {entry: reason}.

    One entry per line, `entry  # reason`. Blank lines and whole-line `#` comments are skipped.
    The reason is not optional in spirit - these files are where an exception gets documented -
    but an empty one is tolerated rather than made a second failure mode.
    """
    path = repo_root() / "Tools" / "allowlists" / f"{name}.txt"
    entries: dict[str, str] = {}
    if not path.is_file():
        return entries
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        entry, _, reason = line.partition("#")
        entry = entry.strip()
        if entry:
            entries[entry] = reason.strip()
    return entries
