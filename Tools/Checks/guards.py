"""
 *  Module guards.
 *
 *  `Scripts/Client` vs `Scripts/Server` is NOT a runtime split - both PBOs ship in the same mod
 *  and load on both sides. What actually gates execution is the preprocessor guard on line 1, so
 *  a `Scripts/Server` file that forgets `#ifdef SERVER` runs on the client too, silently.
 *
 *  Only the server half is enforced as an error. The client half deliberately has no blanket
 *  rule: 27 of its 56 files are unguarded on purpose, because shared code that compiles on both
 *  sides lives there by design (BattleRoyaleConstants.c, BattleRoyaleUtils.c, MissionBaseWorld.c
 *  and friends). An allowlist of 27 entries would record the status quo without asserting
 *  anything about it. What IS checked on the client side is misfiling - a `#ifdef SERVER` file
 *  sitting in the client tree, which is always a file in the wrong folder.
 *
 *  The in-repo addons (`Party/`, `Extra/*`) follow the same layout one level down, as `Server/` and
 *  `Client/` folders inside each script stage, and get the same two rules.
"""

from __future__ import annotations

from Checks._source import Finding, allowlist, error, read, tracked

NAME = "guards"
SUMMARY = "every Server-folder file opens with #ifdef SERVER"

SERVER_FILES = ("Scripts/Server/*.c", "Party/*/Server/*.c", "Extra/*/Server/*.c")
CLIENT_FILES = ("Scripts/Client/*.c", "Party/*/Client/*.c", "Extra/*/Client/*.c")


def first_code_line(text: str) -> str:
    for line in text.splitlines():
        stripped = line.strip()
        if stripped:
            return stripped
    return ""


def run() -> list[Finding]:
    findings: list[Finding] = []
    exempt = allowlist("unguarded_server")

    for path in tracked(*SERVER_FILES):
        first = first_code_line(read(path))
        if first.startswith("#ifdef SERVER"):
            continue
        if path in exempt:
            continue
        findings.append(error(
            "does not open with #ifdef SERVER, so it compiles into the client too - add the "
            "guard, or record the exception in Tools/allowlists/unguarded_server.txt",
            path, 1,
        ))

    for path in tracked(*CLIENT_FILES):
        first = first_code_line(read(path))
        if first.startswith("#ifdef SERVER"):
            findings.append(error(
                "is guarded #ifdef SERVER but lives in a Client folder - move it to the matching "
                "Server folder so the folder matches what the file actually is",
                path, 1,
            ))

    #  An allowlist entry for a file that has since been guarded (or deleted) is stale, and a
    #  stale exemption is how a rule quietly stops applying.
    for path, reason in sorted(exempt.items()):
        if path not in tracked(*SERVER_FILES):
            findings.append(error(
                f"allowlisted as unguarded but is not a tracked Server-folder file - remove the "
                f"entry ({reason or 'no reason recorded'})",
                "Tools/allowlists/unguarded_server.txt",
            ))
        elif first_code_line(read(path)).startswith("#ifdef SERVER"):
            findings.append(error(
                f"{path} is allowlisted as unguarded but now carries #ifdef SERVER - remove the "
                f"stale entry",
                "Tools/allowlists/unguarded_server.txt",
            ))

    return findings
