"""
 *  RPC registrations resolve to a handler, and every send resolves to a registration.
 *
 *  CF's `GetRPCManager().AddRPC(namespace, "Name", target)` dispatches by METHOD NAME. A
 *  registration whose name matches no method is not a compile error and not a runtime error - it
 *  is simply an RPC that never fires, and the sending side looks correct from every angle.
 *
 *  Handlers are found by signature rather than by name: the CF handler shape is always
 *  `void Name(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)`, so
 *  "method whose first parameter is a CallType" identifies them precisely, where matching on the
 *  name alone would happily accept any unrelated method that happened to share it.
 *
 *  Names and namespaces may be string literals or `const string` constants (the standalone addons
 *  use `VP_RPC_*`, `VM_RPC_*`, ...); constants are resolved across the whole tree. A name this
 *  check cannot resolve is reported rather than skipped.
 *
 *  A SendRPC is matched on (namespace, name): CF keys its table on both, so a send on the wrong
 *  namespace is dropped exactly like a typo'd name.
"""

from __future__ import annotations

import re

#  strip_COMMENTS, not strip_code: the RPC name IS a string literal, and strip_code blanks string
#  contents. Using it here made every registration invisible and the whole check pass vacuously.
from Checks._source import ERROR, Finding, read, strip_comments, tracked, warn

NAME = "rpc"
SUMMARY = "every AddRPC name has a CF handler method; every SendRPC has an AddRPC"

_ARG = r'(?:"([A-Za-z0-9_.-]*)"|([A-Za-z_]\w*))'
REGISTER = re.compile(r'\bAddRPC\s*\(\s*' + _ARG + r'\s*,\s*' + _ARG)
UNREGISTER = re.compile(r'\bRemoveRPC\s*\(\s*' + _ARG + r'\s*,\s*' + _ARG)
SEND = re.compile(r'\bSendRPC\s*\(\s*' + _ARG + r'\s*,\s*' + _ARG)
HANDLER = re.compile(r"\b([A-Za-z_]\w*)\s*\(\s*CallType\b")
CONSTANT = re.compile(r'\bconst\s+string\s+([A-Za-z_]\w*)\s*=\s*"([^"]*)"\s*;')


def _line(text: str, index: int) -> int:
    return text.count("\n", 0, index) + 1


def run() -> list[Finding]:
    sources = {path: strip_comments(read(path)) for path in tracked("*.c")}

    constants: dict[str, set[str]] = {}
    for code in sources.values():
        for match in CONSTANT.finditer(code):
            constants.setdefault(match.group(1), set()).add(match.group(2))

    findings: list[Finding] = []

    def resolve(match: re.Match, literal: int, path: str, what: str) -> str | None:
        if match.group(literal) is not None:
            return match.group(literal)
        ident = match.group(literal + 1)
        values = constants.get(ident, set())
        if len(values) == 1:
            return next(iter(values))
        findings.append(warn(
            f"{what} `{ident}` is not a string literal or a single `const string` - this check "
            f"cannot verify the RPC",
            path, _line(sources[path], match.start()),
        ))
        return None

    registered: dict[str, tuple[str, int]] = {}
    registered_keys: set[tuple[str, str]] = set()
    unregistered: set[str] = set()
    sends: list[tuple[str, str, str, int]] = []
    handlers: dict[str, tuple[str, int]] = {}

    for path, code in sources.items():
        for match in REGISTER.finditer(code):
            space = resolve(match, 1, path, "AddRPC namespace")
            name = resolve(match, 3, path, "AddRPC name")
            if name is None:
                continue
            registered.setdefault(name, (path, _line(code, match.start())))
            if space is not None:
                registered_keys.add((space, name))
        for match in UNREGISTER.finditer(code):
            name = resolve(match, 3, path, "RemoveRPC name")
            if name is not None:
                unregistered.add(name)
        for match in SEND.finditer(code):
            space = resolve(match, 1, path, "SendRPC namespace")
            name = resolve(match, 3, path, "SendRPC name")
            if space is not None and name is not None:
                sends.append((space, name, path, _line(code, match.start())))
        for match in HANDLER.finditer(code):
            handlers.setdefault(match.group(1), (path, _line(code, match.start())))

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

    for space, name, path, number in sends:
        if (space, name) in registered_keys:
            continue
        others = sorted(s for s, n in registered_keys if n == name)
        hint = f" (it is registered under {', '.join(others)})" if others else ""
        findings.append(warn(
            f'SendRPC("{space}", "{name}") has no matching AddRPC{hint} - CF drops it '
            f"without a word",
            path, number,
        ))

    return sorted(findings, key=lambda f: (f.level != ERROR, f.path or "", f.line or 0))
