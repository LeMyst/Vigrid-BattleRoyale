"""
 *  Preprocessor balance: every `#ifdef` / `#ifndef` / `#if` has its `#endif` in the same file.
 *
 *  A guard left open at EOF compiles - that was the convention for the file-level `#ifdef SERVER`.
 *  Which is the problem: an `#endif` lost from the middle of such a file silently extends that
 *  block to EOF, nothing reports it, and the rest of the file compiles (and is read by guards,
 *  client-only-types, diag-guards and discipline) under the wrong condition.
 *
 *  Also reported: an `#else` or `#endif` with nothing open, and a second `#else` for one `#if`.
 *
 *  Tools/allowlists/unclosed_preprocessor.txt lists files still left open at EOF. An entry for a
 *  file that is balanced (or gone) is reported as stale.
"""

from __future__ import annotations

from Checks._source import Finding, allowlist, error, numbered, read, strip_code, tracked

NAME = "preprocessor"
SUMMARY = "every #if has its #endif in the same file; no stray #else / #endif"

ALLOWLIST = "Tools/allowlists/unclosed_preprocessor.txt"


def scan(text: str) -> tuple[list[tuple[int, str]], list[tuple[int, str]]]:
    """(directives still open at EOF, stray-directive problems), each as (line, text)."""
    stack: list[tuple[int, str, bool]] = []      # (line, directive, seen #else)
    problems: list[tuple[int, str]] = []
    for number, line in numbered(strip_code(text)):
        s = line.strip()
        if s.startswith("#if"):
            stack.append((number, s, False))
        elif s.startswith("#else"):
            if not stack:
                problems.append((number, "#else with no open #if"))
            elif stack[-1][2]:
                problems.append((number, f"second #else for `{stack[-1][1]}` (line {stack[-1][0]})"))
            else:
                stack[-1] = (stack[-1][0], stack[-1][1], True)
        elif s.startswith("#endif"):
            if not stack:
                problems.append((number, "#endif with no open #if"))
            else:
                stack.pop()
    return [(n, d) for n, d, _ in stack], problems


def run() -> list[Finding]:
    findings: list[Finding] = []
    exempt = allowlist("unclosed_preprocessor")
    scripts = set(tracked("*.c"))

    for path in sorted(scripts):
        open_at_eof, problems = scan(read(path))
        for number, message in problems:
            findings.append(error(message, path, number))
        if path in exempt:
            if not open_at_eof:
                findings.append(error(
                    f"{path} is allowlisted as unclosed but is balanced now - remove the stale "
                    f"entry",
                    ALLOWLIST,
                ))
            continue
        for number, directive in open_at_eof:
            findings.append(error(
                f"`{directive}` is never closed - add its #endif (at EOF for a file-level guard), "
                f"or a later #endif lost in an edit has silently moved this block's end",
                path, number,
            ))

    for path, reason in sorted(exempt.items()):
        if path not in scripts:
            findings.append(error(
                f"allowlisted but is not a tracked .c file - remove the entry "
                f"({reason or 'no reason recorded'})",
                ALLOWLIST,
            ))

    return findings
