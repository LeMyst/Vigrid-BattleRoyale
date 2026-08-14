"""
 *  Asset paths resolve to a file that exists.
 *
 *  The highest silent-failure-to-effort ratio in the set. `CreateWidgets()` on a path that does
 *  not exist returns nothing, draws nothing, and logs NOTHING - there is no .rpt line to find.
 *  The same is true of a layout naming an .edds that was renamed. The only way this surfaces
 *  today is a 2-5 minute build, a launch, and a widget that is simply absent.
 *
 *  Only paths carrying a known asset extension are validated, since a bare PBO-relative prefix
 *  (`"Vigrid-BattleRoyale/Extra/Map/"`) is a fragment that gets concatenated at runtime. Profile
 *  and mission paths (`$profile:`, `$mission:`) are runtime directories, not repo files, and are
 *  skipped.
"""

from __future__ import annotations

import re

from Checks._source import Finding, error, exists, read, string_literals, tracked

NAME = "asset-paths"
SUMMARY = "every Vigrid-BattleRoyale/... asset reference resolves"

PREFIX = "Vigrid-BattleRoyale/"
BACKSLASH_PREFIX = "Vigrid-BattleRoyale\\"
EXTENSIONS = (".layout", ".edds", ".paa", ".imageset", ".json", ".xml", ".ogg", ".wav", ".p3d")

#  Layouts are not C, so their references are pulled with a plain regex rather than the C
#  string-literal reader.
LAYOUT_PATH = re.compile(r"(Vigrid-BattleRoyale/[^\s\"']+)")


def validate(path_value: str, source: str, line: int | None) -> Finding | None:
    if not path_value.lower().endswith(EXTENSIONS):
        return None
    relative = path_value[len(PREFIX):]
    if exists(relative):
        return None
    return error(
        f"references `{path_value}`, which is not a tracked file - a missing asset path draws "
        f"nothing and logs nothing",
        source, line,
    )


def run() -> list[Finding]:
    findings: list[Finding] = []

    for path in tracked("*.c"):
        text = read(path)
        for line, value in string_literals(text):
            if value.startswith(PREFIX):
                finding = validate(value, path, line)
                if finding:
                    findings.append(finding)
            elif value.startswith(BACKSLASH_PREFIX):
                findings.append(error(
                    f"references `{value}` with backslashes - script and JSON asset paths use "
                    f"the forward-slash PBO-relative form; only config.cpp samples[] uses "
                    f"backslashes",
                    path, line,
                ))

    for path in tracked("*.layout"):
        for number, line_text in enumerate(read(path).splitlines(), start=1):
            for match in LAYOUT_PATH.finditer(line_text):
                finding = validate(match.group(1), path, number)
                if finding:
                    findings.append(finding)

    return findings
