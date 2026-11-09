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

    #  `*.json` is in REFERENCING_FILES because data files carry keys too - Data/hints*.json holds
    #  the twenty-two hint pages, Data/credits.json the section headings. Breaking the reference
    #  INSIDE the JSON is what makes this probe specific to that: it fires only while *.json is
    #  listed, so the file list can no longer be narrowed without a probe going MISSED. Worth having
    #  as a probe rather than a one-off manual check, since dropping it back turns thirty-odd live
    #  keys into orphan warnings and nothing else complains.
    ("stringtable: undefined key referenced from a JSON data file", "stringtable",
     "Data/credits.json",
     replace('"#STR_BR_CREDITS_PROJECTLEAD"', '"#STR_BR_CREDITS_NO_SUCH_KEY"')),

    ("asset-paths: layout path that does not exist", "asset-paths",
     "Scripts/Client/5_Mission/GUI/DeathScreenMenu.c",
     append('\nclass BRProbe { void F() { string p = '
            '"Vigrid-BattleRoyale/GUI/layouts/nope.layout"; } }\n')),

    ("asset-paths: backslash asset path", "asset-paths",
     "Scripts/Client/5_Mission/GUI/DeathScreenMenu.c",
     append('\nclass BRProbe2 { void F() { string p = '
            '"Vigrid-BattleRoyale\\\\GUI\\\\layouts\\\\death_screen.layout"; } }\n')),

    ("asset-paths: config.cpp inputs= names a renamed Inputs.xml", "asset-paths",
     "Scripts/Client/config.cpp",
     replace('"Vigrid-BattleRoyale/Data/Inputs.xml"', '"Vigrid-BattleRoyale/Data/Input.xml"')),

    ("asset-paths: script module folder that does not exist", "asset-paths",
     "Scripts/Client/config.cpp",
     replace('"Vigrid-BattleRoyale/Scripts/Client/2_GameLib"',
             '"Vigrid-BattleRoyale/Scripts/Client/2_GameLibX"')),

    ("asset-paths: loading screen missing from Data/LoadingScreens.json", "asset-paths",
     "Data/LoadingScreens.json", replace("br_loading_12.edds", "br_loading_13.edds")),

    ("guards: server file loses #ifdef SERVER", "guards",
     "Scripts/Server/3_Game/Config/BattleRoyaleGameData.c",
     replace("#ifdef SERVER", "//probe")),

    #  The #else of an unrelated #ifdef compiles everywhere; it used to read as guarded.
    ("client-only-types: client-only type in the #else of #ifdef VIGRID_PARTY", "client-only-types",
     "Scripts/Server/5_Mission/BattleRoyale/Server/BattleRoyaleVoice.c",
     append("\n#ifdef VIGRID_PARTY\n#else\n"
            "class BRProbe6 { void F() { BattleRoyaleRPC.GetInstance(); } }\n#endif\n")),

    ("diag-guards: diag type in the #else of #ifdef VIGRID_PARTY", "diag-guards",
     "Scripts/Server/5_Mission/BattleRoyale/Server/BattleRoyaleVoice.c",
     append("\n#ifdef VIGRID_PARTY\n#else\n"
            "class BRProbe7 { void F() { BattleRoyaleDiag.Run(); } }\n#endif\n")),

    ("discipline: BattleRoyale symbol inside Party/", "discipline",
     "Party/Scripts/3_Game/VigridPartyLog.c",
     append('\nclass VPProbe { void F() { BattleRoyaleUtils.Info("x"); } }\n')),

    ("discipline: unguarded host call into an addon API", "discipline",
     "Scripts/Client/5_Mission/BattleRoyale/Client/BattleRoyaleClient.c",
     append("\nclass BRProbe3 { void F() { VigridMapAPI.ClearZones(); } }\n")),

    ("discipline: BattleRoyale symbol inside Extra/AutoRun/", "discipline",
     "Extra/AutoRun/Scripts/3_Game/VigridAutoRunLog.c",
     append('\nclass VAProbe { void F() { BattleRoyaleUtils.Info("x"); } }\n')),

    ("guards: Party Server-folder file loses #ifdef SERVER", "guards",
     "Party/Scripts/4_World/Server/VigridPartyManager.c",
     replace("#ifdef SERVER", "//probe")),

    ("menu-ids: two constants share an id", "menu-ids",
     "Scripts/Client/3_Game/Constants.c", append("\nconst int MENU_BR_PROBE = 178;\n")),

    #  rpc reports at WARNING level, so this probe runs with -W or a correct detection reads as a
    #  miss. It renames a LIVE registration rather than adding a new line, so the probe also proves
    #  the handler lookup is what answers, not the mere presence of an AddRPC call.
    ("rpc: registration with no handler", "rpc",
     "Scripts/Server/5_Mission/BattleRoyale/Server/BattleRoyaleServer.c",
     replace('AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerReadyUp", this);',
             'AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "ProbeHandlerThatDoesNotExist", this);'),
     True),

    #  Registered through a `const string`, which the check used to skip entirely.
    ("rpc: constant-named registration loses its handler", "rpc",
     "Party/Scripts/3_Game/VigridPartyConstants.c",
     replace('VP_RPC_SETTINGS = "VP_Settings"', 'VP_RPC_SETTINGS = "VP_Setting"'),
     True),

    ("rpc: SendRPC on the wrong namespace", "rpc",
     "Scripts/Client/5_Mission/BattleRoyale/Client/BattleRoyaleClient.c",
     replace('SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerReadyUp"',
             'SendRPC( RPC_DAYZBR_NAMESPACE, "PlayerReadyUp"'),
     True),

    ("version: mod.cpp drifts from the constant", "version",
     "mod.cpp", replace('version = "0.2.1-Vigrid"', 'version = "9.9.9-probe"')),

    ("configs: duplicate CfgPatches addon name", "configs",
     "Extra/SpawnCarFull/config.cpp",
     replace("class vigrid_spawn_car_full", "class vigrid_spawn_with_battery")),

    ("configs: requiredAddons names something undeclared", "configs",
     "Extra/SpawnCarFull/config.cpp", replace('"DZ_Scripts"', '"DZ_Scriptz"')),

    #  Declared only in a config.cpp.disabled, so never built: the game refuses to start.
    ("configs: requiredAddons names a parked addon", "configs",
     "Extra/SpawnCarFull/config.cpp",
     replace('"DZ_Scripts"', '"DZ_Scripts", "Dump_Item_Heights"')),

    ("configs: missing comma between array entries", "configs",
     "Extra/MapSatellite/config.cpp",
     replace('"DZ_Data",\n            "DZ_Gear_Navigation"',
             '"DZ_Data"\n            "DZ_Gear_Navigation"')),

    ("configs: parentless RscMapControl (the 2026-08-07 freeze)", "configs",
     "Extra/MapSatellite/config.cpp",
     append("\nclass RscMapControl\n{\n    maxSatelliteAlpha = 1;\n};\n")),

    #  The 2026-08-22 outage, re-allowlisted the way it was first let in. The second lock
    #  (optional_addons.txt) must still fire.
    ("configs: optional mod's addon required, even when re-allowlisted", "configs",
     [("Scripts/Client/config.cpp",
       replace('"DZ_Scripts"', '"DZ_Scripts",\n            "DayZExpansion_Scripts"')),
      ("Tools/allowlists/external_addons.txt",
       append("DayZExpansion_Scripts   # probe\n"))],
     None),

    ("data: UTF-8 BOM on a JSON file", "data",
     "Data/hints.json", lambda b: b"\xef\xbb\xbf" + b),

    ("data: malformed JSON", "data",
     "Data/credits.json", lambda b: b.rstrip()[:-1] + b","),

    #  The first #endif is an inner block's; losing it moves that block's end to EOF.
    ("preprocessor: an #endif lost mid-file", "preprocessor",
     "Scripts/Server/5_Mission/BattleRoyale/Server/BattleRoyaleVoice.c",
     lambda b: b.replace(b"#endif", b"", 1)),

    ("preprocessor: stray #endif", "preprocessor",
     "Scripts/Server/3_Game/BattleRoyale/Webhook/EventWebhook.c", append("\n#endif\n")),

    ("extra-index: addon dropped from the index", "extra-index",
     "Extra/README.md", replace("](SpawnCarFull/README.md)", "](SpawnCarFullX/README.md)")),

    ("client-only-types: server file names a #ifndef SERVER class", "client-only-types",
     "Scripts/Server/5_Mission/BattleRoyale/Server/BattleRoyaleVoice.c",
     append("\nclass BRProbeCOT { void F() { BattleRoyaleRPC.GetInstance(); } }\n")),

    #  The BR_DiagTeleport outage: a shipping caller of a method declared under DIAG_DEVELOPER.
    ("diag-guards: unguarded call to a diag-only method", "diag-guards",
     "Scripts/Server/5_Mission/BattleRoyale/Server/BattleRoyaleVoice.c",
     append("\nclass BRProbeDiag { void F(BattleRoyaleState s) "
            "{ s.BR_DiagTeleportRing(null, \"0 0 0\", 1); } }\n")),

    ("inputs: declared keybind missing from <preset>", "inputs",
     "Data/Inputs.xml",
     replace('        <input name="UADayZBRLeaderboard">\n            <btn name="kF4" />\n'
             '        </input>\n', '')),

    ("inputs: loc= with a leading '#'", "inputs",
     "Extra/AutoRun/Data/Inputs.xml",
     replace('loc="STR_AUTORUN_KEYBIND_TOGGLE"', 'loc="#STR_AUTORUN_KEYBIND_TOGGLE"')),

    ("enfusion: ternary operator", "enfusion",
     "Scripts/Client/3_Game/BattleRoyaleUtils.c",
     append("\nclass BRProbe4 { int F(bool b) { return b ? 1 : 0; } }\n")),

    ("enfusion: multi-line if condition", "enfusion",
     "Scripts/Client/3_Game/BattleRoyaleUtils.c",
     append("\nclass BRProbe5 { void F(bool a, bool b) { if (a\n && b) { } } }\n")),

    ("settings-version: new field in Party's VigridPartyData, no version bump", "settings-version",
     "Party/Scripts/3_Game/Server/VigridPartyData.c",
     replace("    int version = 2;", "    bool probe_flag = true;\n    int version = 2;")),

    ("settings-version: new ref array, no version bump, no Upgrade branch", "settings-version",
     "Scripts/Server/3_Game/Config/BattleRoyaleGameData.c",
     replace("\tint version =", "\tref array<string> probe_field = {\"a\"};\n\tint version =")),

    #  Assembled, so this file stays clean for the check it probes. Warning level, hence -W.
    ("profanity: a profane log string", "profanity",
     "Scripts/Server/5_Mission/BattleRoyale/Server/States/4_BattleRoyalePrepare.c",
     replace('Trace("Village rejected")', 'Trace("Another ' + "fu" + 'cked up village!")'),
     True),
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
