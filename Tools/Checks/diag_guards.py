"""
 *  Diag-only symbols referenced from code that compiles in a retail build.
 *
 *  A class or method declared inside `#ifdef DIAG_DEVELOPER` (or `#ifdef DIAG`) exists only in a
 *  DayZDiag_x64 build. Naming it from anywhere that is NOT itself inside such a region is a
 *  compile error on a retail server - and it takes down the whole script module, with every error
 *  after the first being cascade noise about vanilla types that obviously do exist ("Bad type
 *  'Param2'", "Can't find class Param3", "Bad type 'map'").
 *
 *  ⚠️ THE POINT OF THIS CHECK IS THAT NO LOCAL LAUNCH CAN FIND THIS CLASS OF BUG, EVER.
 *  `project.cfg` points both ClientEXE and ServerEXE at DayZDiag_x64.exe, so DIAG_DEVELOPER is
 *  defined in every offline session, every local client and every local server. The module always
 *  compiles. It takes a retail boot - `Workbench/Batchfiles/Release.bat` - to see it, and until
 *  2026-08-22 there was no local way to do that at all.
 *
 *  It has cost one production server so far. `BattleRoyaleState.BR_DiagTeleport` sat inside a diag
 *  block while `BRMasterControlsModule`'s live "teleport to the live circle" admin action called it
 *  unguarded, directly beneath a comment asserting it was ungated. Every local launch compiled it;
 *  the first retail server died at `Undefined function 'BattleRoyaleState.BR_DiagTeleport'`.
 *
 *  **The rule is: gate the CALLERS, never the shared helper.** A helper that a shipping code path
 *  needs belongs outside the guard, whatever its name says - which is why BR_DiagTeleport is now
 *  ungated while BR_DiagTeleportRing, whose callers really are all diag, is not.
 *
 *  Two exclusions keep this quiet, and both are load-bearing:
 *
 *    - A name declared UNGUARDED anywhere in the tree is not diag-only, whatever else also
 *      declares it. Without this the check drowns in `Update`, `Reset` and `OnMissionFinish` -
 *      every diag harness declares them and so does half the mod.
 *    - A declaration line does not end in `;`, a call statement does. That one test is what keeps
 *      `int level = GetGame().ServerConfigGetInt(...);` from being read as a declaration of
 *      `ServerConfigGetInt` inside whatever region it happens to sit in. A prototype without it
 *      reported 137 KB of findings, none of them real.
 *
 *  Only plain `class X` declarations count, for the same reason as in client_only_types: a
 *  `modded class X` extends a type that already exists elsewhere, so a modded declaration inside a
 *  guard says nothing about where X itself lives.
"""

from __future__ import annotations

import re

from Checks._source import Finding, allowlist, error, read, scripts, strip_code, tracked

NAME = "diag-guards"
SUMMARY = "no retail-compiled file names a #ifdef DIAG_DEVELOPER class or method"

#  Both spellings gate real code here: DIAG_DEVELOPER is the engine's, DIAG is the mod's own
#  (BattleRoyaleConstants.c uses it for BR_TRACE_ENABLED). Neither survives a retail build.
_DIAG_DEFINES = ("DIAG_DEVELOPER", "DIAG")

#  `modded` is excluded by the negative lookbehind - see the module docstring.
_DECL_CLASS = re.compile(r'^\s*(?<!modded )class\s+([A-Za-z_]\w*)', re.M)
_MODDED = re.compile(r'^\s*modded\s+class\s+([A-Za-z_]\w*)', re.M)
_OVERRIDE = re.compile(r'^\s*(?:(?:static|protected|private)\s+)*override\b')

#  A method signature: optional modifiers, a return type, the name, then "(". The type token is
#  what a bare call statement lacks, and the no-";" test below is what a call statement has.
_DECL_METHOD = re.compile(
    r'^\s*(?:(?:static|protected|private|override|proto|native|volatile|ref|const|autoptr)\s+)*'
    r'(?:void|bool|int|float|string|vector|typename|[A-Za-z_]\w*(?:<[^>()]*>)?)\s+'
    r'([A-Za-z_]\w*)\s*\('
)

#  Never a method name, whatever the line looks like.
_KEYWORDS = frozenset({
    "if", "else", "while", "for", "foreach", "switch", "case", "return", "delete", "new",
    "sizeof", "typeof", "thread", "super", "this", "class", "enum", "modded", "break", "continue",
})


def _compiled_scripts() -> tuple[str, ...]:
    """Tracked `.c` files that the build actually compiles.

    An addon parked as `config.cpp.disabled` produces no PBO, so nothing under it is ever compiled
    and a declaration there cannot make a symbol diag-only anywhere. Skipping them is not a
    convenience: `Extra/DumpItemHeights/` declares a diag-only `Close()`, which - method names
    being per-class, unlike the globally unique class names client_only_types relies on - collided
    with vanilla `UIScriptedMenu.Close()` and reported eight shipping call sites as broken.
    """
    parked = tuple(p.rsplit("/", 1)[0] + "/" for p in tracked("*config.cpp.disabled"))
    if not parked:
        return scripts()
    return tuple(p for p in scripts() if not p.startswith(parked))


def _guard_regions(text: str) -> list[bool]:
    """Per line, True when that line sits inside a live `#ifdef DIAG_DEVELOPER` / `#ifdef DIAG`.

    A directive stack rather than a first-line test, because the real code guards *blocks* far
    more often than whole files - 0_BattleRoyaleState.c carries four separate diag blocks in one
    otherwise-shipping class.
    """
    out: list[bool] = []
    stack: list[bool] = []          # per open #if: does it mean "diag build only"?
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("#ifdef"):
            symbol = (s[len("#ifdef"):].strip().split() or [""])[0]
            stack.append(symbol in _DIAG_DEFINES)
        elif s.startswith("#ifndef") or s.startswith("#if"):
            #  `#ifndef DIAG_DEVELOPER` is the RETAIL side of the fence, so it is never diag-only.
            stack.append(False)
        elif s.startswith("#else") and stack:
            stack[-1] = not stack[-1]
        elif s.startswith("#endif") and stack:
            stack.pop()
        out.append(any(stack))
    return out


def _declarations(code: str) -> list[tuple[str, int, str]]:
    """Every class and method declared in `code`, as (name, 0-indexed line, kind).

    `kind` is "class", "method" or "override". Methods are recognised by signature shape and by
    NOT ending in ";" - see the docstring.
    """
    found: list[tuple[str, int, str]] = []

    modded = {m.group(1) for m in _MODDED.finditer(code)}
    for m in _DECL_CLASS.finditer(code):
        if m.group(1) in modded:
            continue
        found.append((m.group(1), code.count("\n", 0, m.start()), "class"))

    for i, line in enumerate(code.splitlines()):
        head, sep, _ = line.partition("(")
        if not sep or "=" in head:
            continue
        #  A call statement ends in ";" and a declaration does not. Enfusion forbids multi-line
        #  conditions and this repo keeps signatures on one line, so this is safe here.
        if line.rstrip().endswith(";"):
            continue
        m = _DECL_METHOD.match(line)
        if m is None:
            continue
        name = m.group(1)
        if name in _KEYWORDS:
            continue
        #  An `override` re-declares a method the parent already has, so it says nothing about
        #  where the symbol lives - the same reasoning that excludes `modded class` above.
        kind = "override" if _OVERRIDE.match(line) else "method"
        found.append((name, i, kind))

    return found


def run() -> list[Finding]:
    findings: list[Finding] = []
    exempt = allowlist("diag_refs")

    #  Pass 1: everything declared inside a diag region, and everything declared outside one.
    diag_only: dict[str, tuple[str, bool]] = {}     # name -> (where, is_class)
    declared_anywhere: set[str] = set()

    per_file: dict[str, tuple[str, list[bool]]] = {}
    for path in _compiled_scripts():
        code = strip_code(read(path))
        guards = _guard_regions(code)
        per_file[path] = (code, guards)

        for name, line_no, kind in _declarations(code):
            in_diag = line_no < len(guards) and guards[line_no]
            if in_diag and kind != "override":
                diag_only.setdefault(name, (f"{path}:{line_no + 1}", kind == "class"))
            else:
                declared_anywhere.add(name)

    risky = {n: v for n, v in diag_only.items() if n not in declared_anywhere}
    if not risky:
        return findings

    #  A class is named bare ("BattleRoyaleDiag.Foo()"); a method is only ever named with its call
    #  parenthesis, and matching it bare would fire on every unrelated field of the same name.
    #
    #  ONE alternation over every candidate rather than a compiled pattern per name: pass 2 visits
    #  a few hundred thousand lines, and 120-odd separate .search() calls on each of them cost 3 s
    #  against a whole-suite budget of about 4.
    classes = sorted(n for n, (_, is_class) in risky.items() if is_class)
    methods = sorted(n for n, (_, is_class) in risky.items() if not is_class)
    scanners = []
    if classes:
        scanners.append(re.compile(r'\b(' + "|".join(map(re.escape, classes)) + r')\b'))
    if methods:
        scanners.append(re.compile(r'\b(' + "|".join(map(re.escape, methods)) + r')\s*\('))

    #  Pass 2: references from anywhere that is not itself diag-only.
    for path, (code, guards) in per_file.items():
        for i, line in enumerate(code.splitlines()):
            if i < len(guards) and guards[i]:
                continue
            if not line.strip():
                continue
            seen: set[str] = set()
            for scanner in scanners:
                for match in scanner.finditer(line):
                    seen.add(match.group(1))
            for name in sorted(seen):
                where, is_class = risky[name]
                if where == f"{path}:{i + 1}":
                    continue
                if f"{path}:{name}" in exempt:
                    continue
                kind = "class" if is_class else "method"
                findings.append(error(
                    f"names '{name}', a {kind} declared inside #ifdef DIAG_DEVELOPER "
                    f"({where}) and so absent from a retail build. This line is not itself inside "
                    f"a diag guard, so the script module will fail to compile on a production "
                    f"server - guard the caller, or move the {kind} out of the diag block if a "
                    f"shipping code path needs it. No local launch can catch this: the whole rig "
                    f"is DayZDiag_x64.exe. Reproduce with Release.bat LaunchServer.bat.",
                    path, i + 1,
                ))

    return findings
