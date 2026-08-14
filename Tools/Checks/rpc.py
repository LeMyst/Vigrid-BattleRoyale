"""
 *  RPC registrations resolve to a handler.
 *
 *  CF's `GetRPCManager().AddRPC(namespace, "Name", target)` dispatches by METHOD NAME. A
 *  registration whose name matches no method is not a compile error and not a runtime error - it
 *  is simply an RPC that never fires, and the sending side looks correct from every angle.
 *
 *  Handlers are found by signature rather than by name: the CF handler shape is always
 *  `void Name(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)`, so
 *  "method whose first parameter is a CallType" identifies them precisely, where matching on the
 *  name alone would happily accept any unrelated method that happened to share it.
"""

from __future__ import annotations

import re

#  strip_COMMENTS, not strip_code: the RPC name IS a string literal, and strip_code blanks string
#  contents. Using it here made every registration invisible and the whole check pass vacuously.
from Checks._source import ERROR, Finding, error, numbered, read, strip_comments, tracked, warn

NAME = "rpc"
SUMMARY = "every AddRPC name has a matching CF handler method"

REGISTER = re.compile(r'\bAddRPC\s*\(\s*[A-Za-z_][\w.]*\s*,\s*"([A-Za-z0-9_]+)"')
UNREGISTER = re.compile(r'\bRemoveRPC\s*\(\s*[A-Za-z_][\w.]*\s*,\s*"([A-Za-z0-9_]+)"')
HANDLER = re.compile(r"\b([A-Za-z_]\w*)\s*\(\s*CallType\b")


def run() -> list[Finding]:
    registered: dict[str, tuple[str, int]] = {}
    unregistered: set[str] = set()
    handlers: dict[str, tuple[str, int]] = {}

    for path in tracked("*.c"):
        code = strip_comments(read(path))
        for number, line in numbered(code):
            for match in REGISTER.finditer(line):
                registered.setdefault(match.group(1), (path, number))
            for match in UNREGISTER.finditer(line):
                unregistered.add(match.group(1))
            for match in HANDLER.finditer(line):
                handlers.setdefault(match.group(1), (path, number))

    findings: list[Finding] = []

    for name, (path, number) in sorted(registered.items()):
        if name not in handlers:
            findings.append(warn(
                f'AddRPC registers "{name}" but no method of that name takes a CallType - CF '
                f"dispatches by method name, so this RPC can never fire",
                path, number,
            ))

    #  A RemoveRPC for a name that was never registered is the mirror mistake: a state's
    #  Deactivate() tearing down something its Activate() never set up, usually after a rename.
    for name in sorted(unregistered - set(registered)):
        findings.append(warn(
            f'RemoveRPC("{name}") has no matching AddRPC anywhere', None, None))

    return sorted(findings, key=lambda f: (f.level != ERROR, f.path or "", f.line or 0))
