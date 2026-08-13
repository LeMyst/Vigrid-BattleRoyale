"""
 *  Standalone-addon discipline, both directions.
 *
 *  `Party/`, `Extra/KillFeed/`, `Extra/SafeZone/` and `Extra/Map/` are self-contained addons that
 *  happen to ship in this repo. Each carries its own logger, settings, stringtable and RPC
 *  namespace so that extracting it into its own mod stays a build-plumbing job rather than a
 *  rewrite - and so that renaming its `config.cpp` to `.disabled` remains a one-rename kill
 *  switch. Both properties rest on the same rule, and it erodes one convenient reference at a
 *  time:
 *
 *    OUTBOUND  no `BattleRoyale*` symbol may appear in an addon's sources.
 *    INBOUND   every host-mod call into an addon's API sits inside that addon's `#ifdef`, so the
 *              mod still builds when the addon is not there.
 *
 *  Matching runs on strip_code() output. Nearly every apparent outbound violation in the tree is
 *  a doc comment explaining this very rule, plus the asset-path constants that legitimately read
 *  "Vigrid-BattleRoyale/Party/" - both of which the stripper removes.
"""

from __future__ import annotations

import re

from Checks._source import Finding, error, numbered, read, strip_code, tracked

NAME = "discipline"
SUMMARY = "standalone addons name no BattleRoyale symbol; host calls are #ifdef-guarded"

#  addon path prefix -> (its #ifdef, the API classes the host mod may call)
ADDONS = {
    "Party/": ("VIGRID_PARTY", ("VigridPartyAPI", "VigridPartyClientAPI")),
    "Extra/KillFeed/": ("KILLFEED", ("KillFeedAPI",)),
    "Extra/SafeZone/": ("VIGRID_SAFEZONE", ("VigridSafeZoneAPI",)),
    "Extra/Map/": ("VIGRID_MAP", ("VigridMapAPI",)),
}

FORBIDDEN = re.compile(r"\bBattleRoyale\w*")


def guard_stack(lines: list[str]) -> list[list[tuple[str, bool]]]:
    """For each line, the stack of enclosing preprocessor conditions as (name, negated).

    Deliberately tolerant of an unclosed `#ifdef` at end of file: that is this repo's convention
    for the file-level SERVER guard, not an error, and a strict reader would reject 58 files.
    """
    stack: list[tuple[str, bool]] = []
    per_line: list[list[tuple[str, bool]]] = []
    for raw in lines:
        line = raw.strip()
        if line.startswith("#ifdef ") or line.startswith("#ifndef "):
            negated = line.startswith("#ifndef ")
            name = line.split(None, 1)[1].strip() if len(line.split(None, 1)) > 1 else ""
            per_line.append(list(stack))
            stack.append((name, negated))
            continue
        if line.startswith("#else"):
            per_line.append(list(stack))
            if stack:
                name, negated = stack[-1]
                stack[-1] = (name, not negated)
            continue
        if line.startswith("#endif"):
            if stack:
                stack.pop()
            per_line.append(list(stack))
            continue
        per_line.append(list(stack))
    return per_line


def guarded_by(stack: list[tuple[str, bool]], define: str) -> bool:
    return any(name == define and not negated for name, negated in stack)


def check_outbound() -> list[Finding]:
    findings: list[Finding] = []
    for prefix, (define, _apis) in ADDONS.items():
        for path in tracked(f"{prefix}*.c", f"{prefix}*.cpp"):
            code = strip_code(read(path))
            for number, line in numbered(code):
                match = FORBIDDEN.search(line)
                if match:
                    findings.append(error(
                        f"names `{match.group(0)}` - this addon must stay free of BattleRoyale "
                        f"symbols so it can be extracted, and so `config.cpp.disabled` remains a "
                        f"working kill switch",
                        path, number,
                    ))
    return findings


def check_inbound() -> list[Finding]:
    findings: list[Finding] = []
    api_to_define = {}
    for prefix, (define, apis) in ADDONS.items():
        for api in apis:
            api_to_define[api] = (define, prefix)

    pattern = re.compile(r"\b(" + "|".join(sorted(api_to_define)) + r")\s*\.")

    for path in tracked("*.c"):
        #  An addon calling its own API needs no guard - it is always present to itself.
        owner = next((p for p in ADDONS if path.startswith(p)), None)
        code = strip_code(read(path))
        lines = code.splitlines()
        stacks = guard_stack(lines)

        for index, line in enumerate(lines):
            for match in pattern.finditer(line):
                api = match.group(1)
                define, prefix = api_to_define[api]
                if owner == prefix:
                    continue
                if guarded_by(stacks[index], define):
                    continue
                findings.append(error(
                    f"calls {api} outside `#ifdef {define}` - the mod must still build with "
                    f"{prefix}config.cpp renamed to .disabled",
                    path, index + 1,
                ))
    return findings


def run() -> list[Finding]:
    return check_outbound() + check_inbound()
