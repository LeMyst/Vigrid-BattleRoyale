"""
 *  Client-only types referenced from code that compiles on the server.
 *
 *  A class declared inside `#ifndef SERVER` does not exist on a dedicated server. Naming it from
 *  anywhere that is NOT itself inside `#ifndef SERVER` - an unguarded file, or an `#ifdef SERVER`
 *  one - is a compile error, and not a local one: it takes down the whole script module, and every
 *  error printed after the first is cascade noise about vanilla types that obviously do exist
 *  ("Bad type 'JsonFileLoader'", "Can't find class Param9").
 *
 *  ⚠️ THE POINT OF THIS CHECK IS THAT AN OFFLINE CLIENT CANNOT FIND THIS CLASS OF BUG, EVER.
 *  `SERVER` is undefined offline, so the guarded type is always present and the module always
 *  compiles. It takes a dedicated server boot - the most expensive test in the loop - to see it,
 *  which is exactly why it is worth catching statically.
 *
 *  It has cost two builds so far:
 *    - `BattleRoyaleRPC` named unguarded in `BattleRoyaleSpectatorCamera`, which took down World.
 *    - `BattleRoyaleShuffleBag` (`#ifndef SERVER`) used by `LoadingScreenBackground` (unguarded),
 *      which took down Game. Its predecessor `ExpansionArray` was unguarded, so swapping in a
 *      guarded replacement silently narrowed where the consumer could compile.
 *
 *  Only plain `class X` declarations count. `modded class X` extends a type that already exists
 *  elsewhere, so a modded declaration inside a guard says nothing about where X itself lives.
"""

from __future__ import annotations

import re

from Checks._source import Finding, allowlist, error, read, scripts, strip_code

NAME = "client-only-types"
SUMMARY = "no server-compiled file names a #ifndef SERVER type"

#  `modded` is excluded by the negative lookahead - see the module docstring.
_DECL = re.compile(r'^\s*(?<!modded )class\s+([A-Za-z_]\w*)', re.M)
_MODDED = re.compile(r'^\s*modded\s+class\s+([A-Za-z_]\w*)', re.M)


def _guard_regions(text: str) -> list[bool]:
    """Per line, True when that line sits inside a live `#ifndef SERVER` region.

    A small directive stack rather than a first-line test, because the real code guards *blocks*
    as often as whole files - BattleRoyaleSpectatorCamera has no top-level guard and wraps the one
    risky call instead, and treating that as unguarded would report a fixed bug forever.
    """
    out: list[bool] = []
    stack: list[bool] = []          # per open #if: does it mean "client only"?
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("#ifndef"):
            stack.append(s[len("#ifndef"):].strip().startswith("SERVER"))
        elif s.startswith("#ifdef"):
            stack.append(False)
        elif s.startswith("#else") and stack:
            stack[-1] = not stack[-1]
        elif s.startswith("#endif") and stack:
            stack.pop()
        out.append(any(stack))
    return out


def run() -> list[Finding]:
    findings: list[Finding] = []
    exempt = allowlist("client_only_refs")

    #  Pass 1: every class declared inside a client-only region, and where.
    client_only: dict[str, str] = {}
    #  A type declared unguarded ANYWHERE exists on the server, whatever else also declares it.
    declared_anywhere: set[str] = set()

    per_file: dict[str, tuple[str, list[bool]]] = {}
    for path in scripts():
        code = strip_code(read(path))
        guards = _guard_regions(code)
        per_file[path] = (code, guards)

        modded = {m.group(1) for m in _MODDED.finditer(code)}
        for m in _DECL.finditer(code):
            name = m.group(1)
            if name in modded:
                continue
            line_no = code.count("\n", 0, m.start())
            if line_no < len(guards) and guards[line_no]:
                client_only.setdefault(name, f"{path}:{line_no + 1}")
            else:
                declared_anywhere.add(name)

    risky = {n: where for n, where in client_only.items() if n not in declared_anywhere}
    if not risky:
        return findings

    #  Pass 2: references from anywhere that is not itself client-only.
    patterns = {n: re.compile(r'\b' + re.escape(n) + r'\b') for n in risky}
    for path, (code, guards) in per_file.items():
        for i, line in enumerate(code.splitlines()):
            if i < len(guards) and guards[i]:
                continue
            if not line.strip():
                continue
            for name, pat in patterns.items():
                if not pat.search(line):
                    continue
                if f"{path}:{name}" in exempt:
                    continue
                findings.append(error(
                    f"names '{name}', which is declared inside #ifndef SERVER "
                    f"({risky[name]}) and so does not exist on a dedicated server. This line is "
                    f"not itself inside #ifndef SERVER, so the module will fail to compile there - "
                    f"guard the reference, or make the type unguarded if it has no client-only "
                    f"dependency. An offline client cannot catch this.",
                    path, i + 1,
                ))

    return findings
