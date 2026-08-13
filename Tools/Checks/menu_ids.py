"""
 *  MENU_* id uniqueness.
 *
 *  Menu ids are one flat namespace over every PBO in the build, and `EnterScriptedMenu` resolves
 *  by number. Two addons that pick the same id do not collide loudly - one simply opens the
 *  other's menu. That has already happened here: two branches both took 178, and the map keybind
 *  opened the death screen.
"""

from __future__ import annotations

import re
from collections import defaultdict

from Checks._source import Finding, error, numbered, read, strip_code, tracked

NAME = "menu-ids"
SUMMARY = "no two MENU_* constants share an id across the PBOs"

DECL = re.compile(r"\bconst\s+int\s+(MENU_[A-Z0-9_]+)\s*=\s*(\d+)")


def run() -> list[Finding]:
    by_id: dict[int, list[tuple[str, str, int]]] = defaultdict(list)

    for path in tracked("*.c"):
        for number, line in numbered(strip_code(read(path))):
            match = DECL.search(line)
            if match:
                by_id[int(match.group(2))].append((match.group(1), path, number))

    findings: list[Finding] = []
    for value, declarations in sorted(by_id.items()):
        if len(declarations) < 2:
            continue
        names = ", ".join(sorted(name for name, _, _ in declarations))
        for name, path, number in declarations:
            findings.append(error(
                f"{name} = {value} collides with {names} - menu ids are one flat namespace "
                f"across every PBO, so one of these menus will silently open the other",
                path, number,
            ))
    return findings
