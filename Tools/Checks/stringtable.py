"""
 *  Localization coverage.
 *
 *  Every STR_* key referenced from script, a layout or an XML must exist in a stringtable.csv -
 *  and in the RIGHT one. The engine loads one table per addon, so a STR_PARTY_* key filed in
 *  LanguageCore/stringtable.csv is not merely untidy: it silently fails to localize, and the
 *  player sees the raw key. That is why the owner map below is enforced rather than just the
 *  union of all four tables.
 *
 *  A referenced token ending in `_` is a concatenation stub - `"#STR_BR_CAUSE_" + name` - and is
 *  skipped, since no real key ends in an underscore.
 *
 *  A key only counts as REFERENCED when the token is preceded by `"` or `#`, i.e. it is being
 *  used as a key rather than merely named. Without that rule the C constants that HOLD the keys
 *  (`static const string STR_KILLFEED_ZONE = "#STR_KF_ZONE";`) are themselves read as references
 *  to keys that do not exist, and the check reports fifteen phantom failures on a clean tree.
"""

from __future__ import annotations

import csv
import io
import re

from Checks._source import ERROR, Finding, allowlist, error, read, tracked, warn

NAME = "stringtable"
SUMMARY = "every STR_* key referenced is defined, in its own addon's table"

#  Which table owns which prefix. The longest matching prefix wins, so a more specific
#  namespace could be carved out later without disturbing the general one.
OWNERS = {
    "STR_BR_": "LanguageCore/stringtable.csv",
    "STR_PARTY_": "Party/stringtable.csv",
    "STR_KF_": "Extra/KillFeed/stringtable.csv",
    "STR_MAP_": "Extra/Map/stringtable.csv",
}

#  Preceded by `"` (a bare key, the form server script sends over the wire) or `#` (the localised
#  form a widget resolves itself). Anything else is an identifier that merely starts with STR_.
KEY = re.compile(r'["#](STR_[A-Z0-9_]+)')
REFERENCING_FILES = ("*.c", "*.layout", "*.xml", "*.cpp")


def owner_of(key: str) -> str | None:
    best = None
    for prefix, table in OWNERS.items():
        if key.startswith(prefix):
            if best is None or len(prefix) > len(best[0]):
                best = (prefix, table)
    return best[1] if best else None


def load_tables() -> tuple[dict[str, str], list[Finding]]:
    """{key: table path} plus any structural findings about the CSVs themselves."""
    defined: dict[str, str] = {}
    findings: list[Finding] = []

    for table in tracked("*stringtable.csv"):
        rows = list(csv.reader(io.StringIO(read(table))))
        if not rows:
            findings.append(error("stringtable is empty", table))
            continue

        width = len(rows[0])
        for number, row in enumerate(rows[1:], start=2):
            if not row or not row[0].strip():
                continue
            if len(row) != width:
                findings.append(error(
                    f"row has {len(row)} columns, header has {width} - a short row shifts every "
                    f"later language by one",
                    table, number,
                ))
            key = row[0].strip()
            if key in defined:
                findings.append(error(
                    f"{key} is also defined in {defined[key]} - which table wins is not "
                    f"something to leave to addon load order",
                    table, number,
                ))
            else:
                defined[key] = table

    return defined, findings


def collect_references() -> dict[str, tuple[str, int]]:
    """{key: first (path, line) it was seen at}."""
    refs: dict[str, tuple[str, int]] = {}
    for path in tracked(*REFERENCING_FILES):
        for number, line in enumerate(read(path).splitlines(), start=1):
            for match in KEY.finditer(line):
                refs.setdefault(match.group(1), (path, number))
    return refs


def run() -> list[Finding]:
    defined, findings = load_tables()
    references = collect_references()
    external = allowlist("external_string_keys")

    for key, (path, line) in sorted(references.items()):
        if key.endswith("_"):
            continue  # concatenation stub, e.g. "#STR_BR_CAUSE_" + cause
        if key in external:
            continue  # a vanilla or dependency key, resolved by somebody else's table
        expected = owner_of(key)
        actual = defined.get(key)

        if actual is None:
            if expected is None:
                findings.append(warn(
                    f"{key} is in no stringtable and matches no known addon prefix - if it is a "
                    f"vanilla key, add it to Tools/allowlists/external_string_keys.txt",
                    path, line))
            else:
                findings.append(error(
                    f"{key} is referenced but defined nowhere - expected it in {expected}",
                    path, line))
            continue

        if expected is not None and actual != expected:
            findings.append(error(
                f"{key} is defined in {actual} but belongs in {expected} - the engine loads one "
                f"table per addon, so this key will not resolve",
                path, line))

    for key, table in sorted(defined.items()):
        if key not in references:
            findings.append(warn(f"{key} is defined but never referenced", table))

    #  Sort errors ahead of warnings so the actionable ones are not buried under orphans.
    return sorted(findings, key=lambda f: (f.level != ERROR, f.path or "", f.line or 0))
