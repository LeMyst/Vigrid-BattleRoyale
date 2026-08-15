"""
 *  Keybind declarations.
 *
 *  An `Inputs.xml` describes each keybind three times - `<actions>` declares it and gives it a
 *  label, `<sorting>` files it under a category in Options -> Controls, and `<preset>` gives it a
 *  default binding - and the engine does not complain when those three disagree. An input missing
 *  from `<sorting>` is declared but unlisted; one missing from `<preset>` has no default key, so
 *  `GetInputByName(...)` resolves and never fires. Both are silent, and both look like a bug in the
 *  script that reads the input rather than in the XML that declares it.
 *
 *  Malformed XML is worse and just as quiet: the engine drops the whole file, so EVERY keybind the
 *  addon declares stops working at once.
 *
 *  The commented-entry rule is hygiene rather than correctness, and it is here because this repo
 *  has now been bitten by it twice. Three dead minimap entries sat commented out long enough to be
 *  filed as a review finding; deleting them by dropping every line containing "MiniMap" then left
 *  six orphaned `<btn>` and `</input>` fragments behind, because in `<preset>` an entry spans three
 *  lines and only the first carries the name. Neither was visible to a parser - they are comments.
 *  Parked keybinds belong in git history, not in the shipped file.
 *
 *  Deliberately NOT checked here: that a `loc` key exists in a stringtable. The `stringtable` check
 *  already scans `*.xml`, so a missing key is reported there and reporting it twice would only make
 *  one of the two the annoying one. What IS checked is the shape `loc` must take, which that check
 *  cannot know about.
"""

from __future__ import annotations

import re
import xml.etree.ElementTree as ET

from Checks._source import Finding, error, numbered, read, tracked, warn

NAME = "inputs"
SUMMARY = "every Inputs.xml parses, and its three sections agree"

#  A commented-out `<input>` or `<btn>` fragment, whole-line. Matched on the raw text on purpose:
#  the point is precisely the debris a parser cannot see.
COMMENTED = re.compile(r"<!--\s*<?/?\s*(input|btn)\b|<!--\s*</input>")

#  Where a given name is declared, for reporting. Section-agnostic: the first mention is close
#  enough to lead a reader to the right block, and it avoids a second hand-rolled XML walk.
NAMED = re.compile(r'<\s*input\s+name\s*=\s*"([^"]+)"')


def line_of(text: str, name: str) -> int | None:
    for number, line in numbered(text):
        match = NAMED.search(line)
        if match and match.group(1) == name:
            return number
    return None


def check_file(path: str) -> list[Finding]:
    text = read(path)
    findings: list[Finding] = []

    #  Commented-out entries first: this one survives a parse failure, and if the file is
    #  malformed it may well be the reason why.
    for number, line in numbered(text):
        if COMMENTED.search(line):
            findings.append(warn(
                "commented-out keybind entry - delete it rather than parking it here; a partial "
                "deletion leaves fragments that read as meaningful and that no parser can see",
                path, number,
            ))

    try:
        root = ET.fromstring(text)
    except ET.ParseError as exc:
        line, column = exc.position
        findings.append(error(
            f"malformed XML ({exc.msg}) - the engine drops the whole file, so EVERY keybind this "
            f"addon declares stops working, with nothing in the log to say so",
            path, line,
        ))
        return findings

    declared: list[str] = []
    for element in root.findall(".//actions/input"):
        name = element.get("name")
        if not name:
            findings.append(error("<input> in <actions> has no name attribute", path))
            continue
        declared.append(name)

        loc = element.get("loc")
        if loc is None:
            findings.append(warn(
                f"{name} has no loc= - Options -> Controls shows the raw input name",
                path, line_of(text, name),
            ))
        elif loc.startswith("#"):
            findings.append(error(
                f"{name} loc=\"{loc}\" must be the BARE stringtable key with no leading '#' - "
                f"that is the engine's convention here, and DayZ-Expansion's own Inputs.xml files "
                f"use it. With the '#' the label ships unresolved",
                path, line_of(text, name),
            ))

    for element in root.findall(".//sorting"):
        loc = element.get("loc")
        if loc and loc.startswith("#"):
            findings.append(error(
                f"<sorting> loc=\"{loc}\" must be the bare stringtable key, no leading '#'",
                path,
            ))

    sorted_names = [e.get("name") for e in root.findall(".//sorting/input") if e.get("name")]
    preset_names = [e.get("name") for e in root.findall(".//preset/input") if e.get("name")]

    declared_set = set(declared)

    for name in sorted(declared_set - set(sorted_names)):
        findings.append(error(
            f"{name} is declared in <actions> but absent from <sorting> - it will not be listed "
            f"under its category in Options -> Controls",
            path, line_of(text, name),
        ))

    for name in sorted(declared_set - set(preset_names)):
        findings.append(error(
            f"{name} is declared in <actions> but absent from <preset> - it gets no default "
            f"binding, so GetInputByName resolves and the key never fires",
            path, line_of(text, name),
        ))

    for section, names in (("sorting", sorted_names), ("preset", preset_names)):
        for name in sorted(set(names) - declared_set):
            findings.append(error(
                f"{name} appears in <{section}> but is not declared in <actions>",
                path, line_of(text, name),
            ))

    for element in root.findall(".//preset/input"):
        name = element.get("name")
        if name and not element.findall("btn"):
            findings.append(error(
                f"{name} has a <preset> entry with no <btn> - no default binding",
                path, line_of(text, name),
            ))

    for section, names in (("actions", declared), ("sorting", sorted_names),
                           ("preset", preset_names)):
        duplicates = {n for n in names if names.count(n) > 1}
        for name in sorted(duplicates):
            findings.append(error(
                f"{name} is listed more than once in <{section}>", path, line_of(text, name),
            ))

    return findings


def run() -> list[Finding]:
    findings: list[Finding] = []
    for path in tracked("*Inputs.xml"):
        findings.extend(check_file(path))
    return findings
