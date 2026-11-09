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
 *
 *  Scanned: `*.c`, `*.layout`, `*.imageset`, `*.json`, every live `config.cpp` and `mod.cpp`.
 *  Configs are rapified, not C: a backslash is literal there, and an extensionless entry is either
 *  a script-module folder (`files[]`) or a sound sample whose extension the engine supplies.
"""

from __future__ import annotations

import json
import re
from functools import lru_cache

from Checks._source import Finding, error, exists, read, string_literals, strip_comments, tracked

NAME = "asset-paths"
SUMMARY = "every Vigrid-BattleRoyale/... asset reference resolves"

PREFIX = "Vigrid-BattleRoyale/"
BACKSLASH_PREFIX = "Vigrid-BattleRoyale\\"
EXTENSIONS = (".layout", ".edds", ".paa", ".imageset", ".json", ".xml", ".ogg", ".wav", ".p3d")
SAMPLE_EXTENSIONS = (".ogg", ".wav", ".wss")

#  Layouts and imagesets are not C, so their references are pulled with a plain regex rather than
#  the C string-literal reader.
LAYOUT_PATH = re.compile(r"(Vigrid-BattleRoyale/[^\s\"']+)")
CONFIG_STRING = re.compile(r'"([^"\n]*)"')
JSON_STRING = re.compile(r'"((?:[^"\\\n]|\\.)*)"')


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


@lru_cache(maxsize=1)
def tracked_dirs() -> frozenset[str]:
    dirs: set[str] = set()
    for path in tracked():
        parts = path.split("/")[:-1]
        for i in range(1, len(parts) + 1):
            dirs.add("/".join(parts[:i]))
    return frozenset(dirs)


def validate_config(value: str, source: str, line: int) -> Finding | None:
    normalised = value.replace("\\", "/").lstrip("/")
    if not normalised.startswith(PREFIX):
        return None
    if "." in normalised.rsplit("/", 1)[-1]:
        return validate(normalised, source, line)
    relative = normalised[len(PREFIX):].rstrip("/")
    if relative in tracked_dirs():
        return None
    if any(exists(relative + ext) for ext in SAMPLE_EXTENSIONS):
        return None
    return error(
        f"references `{value}`, which is neither a tracked folder (a script module's files[]) nor "
        f"a tracked sound file",
        source, line,
    )


def run() -> list[Finding]:
    findings: list[Finding] = []

    def keep(finding: Finding | None) -> None:
        if finding:
            findings.append(finding)

    for path in tracked("*.c"):
        text = read(path)
        for line, value in string_literals(text):
            if value.startswith(PREFIX):
                keep(validate(value, path, line))
            elif value.startswith(BACKSLASH_PREFIX):
                findings.append(error(
                    f"references `{value}` with backslashes - script and JSON asset paths use "
                    f"the forward-slash PBO-relative form; only config.cpp samples[] uses "
                    f"backslashes",
                    path, line,
                ))

    for path in tracked("*.layout", "*.imageset"):
        for number, line_text in enumerate(read(path).splitlines(), start=1):
            for match in LAYOUT_PATH.finditer(line_text):
                keep(validate(match.group(1), path, number))

    for path in tracked("*.json"):
        for number, line_text in enumerate(read(path).splitlines(), start=1):
            for match in JSON_STRING.finditer(line_text):
                try:
                    value = json.loads(f'"{match.group(1)}"')
                except ValueError:
                    continue
                if value.startswith(PREFIX):
                    keep(validate(value, path, number))
                elif value.startswith(BACKSLASH_PREFIX):
                    findings.append(error(
                        f"references `{value}` with backslashes - JSON asset paths use the "
                        f"forward-slash PBO-relative form",
                        path, number,
                    ))

    for path in tracked("*config.cpp", "mod.cpp"):
        text = strip_comments(read(path))
        for match in CONFIG_STRING.finditer(text):
            keep(validate_config(match.group(1), path, text.count("\n", 0, match.start()) + 1))

    return findings
