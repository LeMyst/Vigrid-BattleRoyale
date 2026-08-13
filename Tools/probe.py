#!/usr/bin/env python3
"""
 *  Negative probes for the static checks: break something on purpose, confirm the check fires,
 *  restore the file byte-exactly.
 *
 *  This is the acceptance gate for Tools/check.py, not a nicety. A check that passes on a clean
 *  tree AND on a broken one is worse than no check, because it reports safety it is not
 *  providing - and that failure mode is completely invisible from the outside. It has already
 *  happened twice here: `rpc` and `version` were both written against strip_code(), which blanks
 *  string contents, so they found nothing at all and reported a clean pass.
 *
 *  Run it after changing any check, and add a probe with any new one:
 *
 *      python Tools/probe.py
 *
 *  Every probe must FIRE. The harness requires an actual reported finding, not merely a non-zero
 *  exit - a crashed check also exits non-zero, which is how an early version of this file managed
 *  to report 20/20 without a single genuine detection behind it.
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent


def run_check(name: str, strict: bool = False) -> tuple[int, str]:
    #  Pass the REAL environment. A stripped one hides `git` from check.py, which then crashes on
    #  every probe and looks exactly like every probe succeeding.
    env = dict(os.environ)
    env["NO_COLOR"] = "1"
    cmd = [sys.executable, "Tools/check.py", "--only", name]
    if strict:
        cmd.append("-W")
    result = subprocess.run(cmd, cwd=REPO, capture_output=True, text=True, env=env)
    return result.returncode, result.stdout + result.stderr


def probe(label, check, path, mutate, strict=False) -> bool:
    """mutate: bytes -> bytes. Pass path as [(path, mutate), ...] for a multi-file probe.

    strict=True for checks that report at warning level.
    """
    edits = path if isinstance(path, list) else [(path, mutate)]
    originals = {p: (REPO / p).read_bytes() for p, _ in edits}
    marker = "warning:" if strict else "error:"
    try:
        for target, change in edits:
            (REPO / target).write_bytes(change(originals[target]))

        code, out = run_check(check, strict)
        detail = ""
        for line in out.splitlines():
            if marker in line and "is defined but never referenced" not in line:
                detail = line.strip()[:160]
                break

        fired = code != 0 and bool(detail)
        crashed = code != 0 and not detail
        status = "FIRED " if fired else ("CRASH " if crashed else "MISSED")
        print(f"  [{status}] {label}")
        if detail:
            print(f"           {detail}")
        if not fired:
            print("           ---- no finding reported; tail of output ----")
            for line in out.splitlines()[-12:]:
                print("           " + line)
        return fired
    finally:
        for target, original in originals.items():
            (REPO / target).write_bytes(original)


def append(text: str):
    return lambda b: b + text.encode("utf-8")


def replace(old: str, new: str):
    #  Anchors are newline-agnostic: this repo's sources are a mix of LF and CRLF.
    def go(b: bytes) -> bytes:
        for newline in ("\n", "\r\n"):
            anchor = old.replace("\n", newline).encode()
            if anchor in b:
                return b.replace(anchor, new.replace("\n", newline).encode(), 1)
        raise AssertionError(f"probe anchor not found: {old!r}")
    return go


PROBES = [
    ("stringtable: reference an undefined key", "stringtable",
     "GUI/layouts/death_screen.layout", append('\n// "#STR_BR_TOTALLY_MADE_UP"\n')),

    #  The wrong-owner rule only fires on a key that is REFERENCED, so this probe has to both file
    #  the row in the wrong table and reference it.
    ("stringtable: key filed in the wrong addon's table", "stringtable",
     [("LanguageCore/stringtable.csv",
       lambda b: b + b'\n"STR_PARTY_WRONG_TABLE"' + b',"x"' * 14 + b"\n"),
      ("GUI/layouts/death_screen.layout", append('\n// "#STR_PARTY_WRONG_TABLE"\n'))],
     None),

    ("stringtable: duplicate key across two tables", "stringtable",
     "Party/stringtable.csv",
     lambda b: b + b'\n"STR_BR_COT_NAME"' + b',"x"' * 14 + b"\n"),

    ("asset-paths: layout path that does not exist", "asset-paths",
     "Scripts/Client/5_Mission/GUI/DeathScreenMenu.c",
     append('\nclass BRProbe { void F() { string p = '
            '"Vigrid-BattleRoyale/GUI/layouts/nope.layout"; } }\n')),

    ("asset-paths: backslash asset path", "asset-paths",
     "Scripts/Client/5_Mission/GUI/DeathScreenMenu.c",
     append('\nclass BRProbe2 { void F() { string p = '
            '"Vigrid-BattleRoyale\\\\GUI\\\\layouts\\\\death_screen.layout"; } }\n')),

    ("guards: server file loses #ifdef SERVER", "guards",
     "Scripts/Server/3_Game/Config/BattleRoyaleGameData.c",
     replace("#ifdef SERVER", "//probe")),

    ("discipline: BattleRoyale symbol inside Party/", "discipline",
     "Party/Scripts/3_Game/VigridPartyLog.c",
     append('\nclass VPProbe { void F() { BattleRoyaleUtils.Info("x"); } }\n')),

    ("discipline: unguarded host call into an addon API", "discipline",
     "Scripts/Client/5_Mission/BattleRoyale/Client/BattleRoyaleClient.c",
     append("\nclass BRProbe3 { void F() { VigridMapAPI.ClearZones(); } }\n")),

    ("menu-ids: two constants share an id", "menu-ids",
     "Scripts/Client/3_Game/Constants.c", append("\nconst int MENU_BR_PROBE = 178;\n")),

    #  rpc reports at WARNING level, so this probe runs with -W or a correct detection reads as a
    #  miss.
    ("rpc: registration with no handler", "rpc",
     "Scripts/Server/5_Mission/BattleRoyale/Server/BattleRoyaleServer.c",
     replace('AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "RequestEntityHealthUpdate", this);',
             'AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "ProbeHandlerThatDoesNotExist", this);'),
     True),

    ("version: mod.cpp drifts from the constant", "version",
     "mod.cpp", replace('version = "0.1.0-Vigrid"', 'version = "9.9.9-probe"')),

    ("configs: duplicate CfgPatches addon name", "configs",
     "Extra/SpawnCarFull/config.cpp",
     replace("class vigrid_spawn_car_full", "class vigrid_spawn_with_battery")),

    ("configs: requiredAddons names something undeclared", "configs",
     "Extra/SpawnCarFull/config.cpp", replace('"DZ_Scripts"', '"DZ_Scriptz"')),

    ("configs: missing comma between array entries", "configs",
     "Extra/MapSatellite/config.cpp",
     replace('"DZ_Data",\n            "DZ_Gear_Navigation"',
             '"DZ_Data"\n            "DZ_Gear_Navigation"')),

    ("configs: parentless RscMapControl (the 2026-08-07 freeze)", "configs",
     "Extra/MapSatellite/config.cpp",
     append("\nclass RscMapControl\n{\n    maxSatelliteAlpha = 1;\n};\n")),

    ("data: UTF-8 BOM on a JSON file", "data",
     "Data/hints.json", lambda b: b"\xef\xbb\xbf" + b),

    ("data: malformed JSON", "data",
     "Data/credits.json", lambda b: b.rstrip()[:-1] + b","),

    ("extra-index: addon dropped from the index", "extra-index",
     "Extra/README.md", replace("](SpawnCarFull/README.md)", "](SpawnCarFullX/README.md)")),

    ("enfusion: ternary operator", "enfusion",
     "Scripts/Client/3_Game/BattleRoyaleUtils.c",
     append("\nclass BRProbe4 { int F(bool b) { return b ? 1 : 0; } }\n")),

    ("enfusion: multi-line if condition", "enfusion",
     "Scripts/Client/3_Game/BattleRoyaleUtils.c",
     append("\nclass BRProbe5 { void F(bool a, bool b) { if (a\n && b) { } } }\n")),

    ("settings-version: new ref array, no version bump, no Upgrade branch", "settings-version",
     "Scripts/Server/3_Game/Config/BattleRoyaleGameData.c",
     replace("\tint version =", "\tref array<string> probe_field = {\"a\"};\n\tint version =")),
]


def main() -> int:
    print()
    print("  Negative probes - every one should FIRE")
    print()
    fired = sum(1 for entry in PROBES if probe(*entry))
    print()
    print(f"  {fired}/{len(PROBES)} probes fired")

    #  The probes edit tracked files in place. Prove the tree came back.
    dirty = subprocess.run(
        ["git", "-C", str(REPO), "status", "--porcelain", "--untracked-files=no"],
        capture_output=True, text=True,
    ).stdout.strip()
    if dirty:
        print()
        print("  WARNING - tracked files were left modified:")
        for line in dirty.splitlines():
            print("    " + line)
        return 1

    print("  tree restored, no tracked file modified")
    print()
    return 0 if fired == len(PROBES) else 1


if __name__ == "__main__":
    sys.exit(main())
