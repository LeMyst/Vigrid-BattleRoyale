"""
 *  Extra/ stays self-describing.
 *
 *  Every folder under Extra/ is an independent single-purpose PBO. The index in Extra/README.md
 *  is the only place they are listed together, so a new addon that nobody adds to it is
 *  effectively undiscoverable, and a folder with neither config.cpp nor config.cpp.disabled is a
 *  folder the build ignores entirely - which looks exactly like a working addon until someone
 *  goes looking for its PBO.
"""

from __future__ import annotations

from Checks._source import Finding, error, read, repo_root, warn

NAME = "extra-index"
SUMMARY = "every Extra/ addon has a README, an index entry and a config"

INDEX = "Extra/README.md"


def run() -> list[Finding]:
    findings: list[Finding] = []
    extra = repo_root() / "Extra"
    if not extra.is_dir():
        return [error("Extra/ does not exist", None)]

    index_text = read(INDEX)

    for folder in sorted(p for p in extra.iterdir() if p.is_dir()):
        name = folder.name
        if name.startswith("."):
            continue

        if not (folder / "README.md").is_file():
            findings.append(error(
                "has no README.md - every Extra/ addon documents itself", f"Extra/{name}"))

        #  The index links each addon as `[Name](Name/README.md)`. Test for the LINK, not for the
        #  name as a substring: a bare `name in index_text` is satisfied by any longer name that
        #  contains it, and by a passing mention in the prose above the table.
        if f"]({name}/README.md)" not in index_text:
            findings.append(error(
                f"{name} has no `]({name}/README.md)` link in {INDEX} - a new addon nobody "
                f"indexes is one nobody finds",
                INDEX,
            ))

        has_config = (folder / "config.cpp").is_file()
        has_disabled = (folder / "config.cpp.disabled").is_file()
        if not has_config and not has_disabled:
            findings.append(error(
                "has neither config.cpp nor config.cpp.disabled, so the build ignores it "
                "entirely and it produces no PBO",
                f"Extra/{name}",
            ))
        elif has_config and has_disabled:
            findings.append(warn(
                "has both config.cpp and config.cpp.disabled - the addon is live, and the "
                ".disabled copy will drift",
                f"Extra/{name}",
            ))

    return findings
