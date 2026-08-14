"""
 *  The mod version is written down twice and must agree.
 *
 *  `BATTLEROYALE_VERSION` in Scripts/Client/2_GameLib/BattleRoyaleConstants.c is what script
 *  reports; `version` in mod.cpp is what the launcher shows. Nothing keeps them in step.
"""

from __future__ import annotations

import re

#  strip_COMMENTS, not strip_code: both versions ARE string literals, and strip_code blanks
#  string contents - which made the two empty strings compare equal and the check pass vacuously.
from Checks._source import Finding, error, read, strip_comments

NAME = "version"
SUMMARY = "BATTLEROYALE_VERSION and mod.cpp agree"

CONSTANTS = "Scripts/Client/2_GameLib/BattleRoyaleConstants.c"
MOD = "mod.cpp"

SCRIPT_VERSION = re.compile(r'\bBATTLEROYALE_VERSION\s*=\s*"([^"]*)"')
MOD_VERSION = re.compile(r'^\s*version\s*=\s*"([^"]*)"', re.MULTILINE)


def run() -> list[Finding]:
    script_match = SCRIPT_VERSION.search(strip_comments(read(CONSTANTS)))
    if script_match is None:
        return [error("BATTLEROYALE_VERSION not found", CONSTANTS)]

    mod_match = MOD_VERSION.search(strip_comments(read(MOD)))
    if mod_match is None:
        return [error("no `version = \"...\"` found", MOD)]

    #  Two empty strings compare equal. Assert we actually read something, so a future change to
    #  the extraction cannot turn this check into a no-op that reports success.
    if not script_match.group(1):
        return [error("BATTLEROYALE_VERSION is empty", CONSTANTS)]
    if not mod_match.group(1):
        return [error("version is empty", MOD)]

    if script_match.group(1) != mod_match.group(1):
        return [error(
            f'version is "{mod_match.group(1)}" but BATTLEROYALE_VERSION in {CONSTANTS} is '
            f'"{script_match.group(1)}"',
            MOD,
        )]
    return []
