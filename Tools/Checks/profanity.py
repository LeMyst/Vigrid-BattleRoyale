"""
 *  No profanity or slurs in the tracked text files - comments, log strings and docs included.
 *
 *  The repo is public, and Trace/Info strings land in server operators' logs. Matching is whole
 *  words, case-insensitive, over the raw text: comments and strings are exactly where it hides.
 *
 *  Tools/allowlists/profanity.txt takes `<repo-relative path>:<word>` (word lowercased) for a
 *  legitimate use such as a third-party name. An entry that no longer matches is reported.
"""

from __future__ import annotations

import re

from Checks._source import Finding, allowlist, numbered, read, tracked, warn

NAME = "profanity"
SUMMARY = "no profanity or slurs in sources, comments, log strings or docs"

TEXT_FILES = (
    "*.c", "*.cpp", "*.layout", "*.imageset", "*.json", "*.csv", "*.xml", "*.md", "*.py",
    "*.bat", "*.cfg", "*.txt", "*.yml",
)
#  This file and the allowlist necessarily spell the words out.
SKIP = ("Tools/Checks/profanity.py", "Tools/allowlists/profanity.txt")

WORDS = re.compile(
    r"\b\w*(?:fuck|shit)\w*"
    r"|\b(?:bitch|cunt|asshole|faggot|nigg|slut|whore)\w*"
    r"|\bretard(?:ed|s)?\b"
    r"|\bwtf\b",
    re.IGNORECASE,
)


def run() -> list[Finding]:
    findings: list[Finding] = []
    exempt = allowlist("profanity")
    used: set[str] = set()

    for path in tracked(*TEXT_FILES):
        if path in SKIP:
            continue
        for number, line in numbered(read(path)):
            for match in WORDS.finditer(line):
                word = match.group(0).lower()
                key = f"{path}:{word}"
                if key in exempt:
                    used.add(key)
                    continue
                findings.append(warn(
                    f"`{match.group(0)}` - rephrase it; the repo is public and log strings reach "
                    f"server operators. Record a legitimate use in Tools/allowlists/profanity.txt",
                    path, number,
                ))

    for key in sorted(set(exempt) - used):
        findings.append(warn(
            f"allowlist entry `{key}` matches nothing any more - remove it",
            "Tools/allowlists/profanity.txt",
        ))

    return findings
