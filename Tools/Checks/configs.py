"""
 *  config.cpp structure.
 *
 *  Four things, each of which the build accepts happily and the game gets wrong:
 *
 *    - a config with no CfgPatches, or two configs claiming the same addon name (addon names are
 *      one flat namespace, exactly like menu ids);
 *    - a requiredAddons entry naming an in-repo addon that no longer exists, which does not fail
 *      the build - it just stops constraining load order, and this mod depends on that ordering
 *      (Scripts_Server requires Scripts_Client so that server code can `modded class` over
 *      client-declared classes);
 *    - two adjacent array entries with no comma between them, which is a rapify syntax error that
 *      only surfaces when the game loads the module - the documented MOVING_ZONE trap in
 *      Scripts/Client/config.cpp's defines[];
 *    - redeclaring a vanilla class without restating its parent, which REPLACES it rather than
 *      merge-patching it. See Tools/allowlists/vanilla_config_parents.txt.
 *
 *  Parsing runs on strip_comments() output, not strip_code(): requiredAddons entries ARE string
 *  literals, so blanking string contents would make the whole check pass vacuously.
"""

from __future__ import annotations

import re

from Checks._source import (
    Finding, allowlist, error, read, repo_root, strip_comments, tracked, warn,
)

NAME = "configs"
SUMMARY = "CfgPatches names unique, requiredAddons resolve, no stripped vanilla parents"

CLASS = re.compile(r"\bclass\s+([A-Za-z_]\w*)\s*(?::\s*([A-Za-z_]\w*)\s*)?([{;])")
ARRAY = re.compile(r"\b([A-Za-z_]\w*)\s*\[\s*\]\s*=\s*\{", re.MULTILINE)
ADJACENT_STRINGS = re.compile(r'"\s+"')
STRING = re.compile(r'"([^"]*)"')


def line_of(text: str, index: int) -> int:
    return text.count("\n", 0, index) + 1


def depth_map(text: str) -> list[int]:
    """Brace depth before each character, ignoring braces inside string literals."""
    depths = []
    depth = 0
    in_string = False
    escaped = False
    for ch in text:
        depths.append(depth)
        if escaped:
            escaped = False
            continue
        if ch == "\\" and in_string:
            escaped = True
            continue
        if ch == '"':
            in_string = not in_string
            continue
        if in_string:
            continue
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth = max(0, depth - 1)
    return depths


def matching_brace(text: str, open_index: int) -> int:
    """Index of the `}` closing the `{` at open_index, or len(text) if unterminated."""
    depth = 0
    in_string = False
    escaped = False
    for i in range(open_index, len(text)):
        ch = text[i]
        if escaped:
            escaped = False
            continue
        if ch == "\\" and in_string:
            escaped = True
            continue
        if ch == '"':
            in_string = not in_string
            continue
        if in_string:
            continue
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return i
    return len(text)


def addon_names(text: str) -> list[tuple[str, int]]:
    """Classes declared directly inside `class CfgPatches`."""
    depths = depth_map(text)
    names: list[tuple[str, int]] = []
    for match in CLASS.finditer(text):
        if match.group(1) != "CfgPatches" or match.group(3) != "{":
            continue
        open_index = text.index("{", match.end() - 1)
        close_index = matching_brace(text, open_index)
        want_depth = depths[open_index] + 1
        for inner in CLASS.finditer(text, open_index, close_index):
            if inner.group(3) == "{" and depths[inner.start()] == want_depth:
                names.append((inner.group(1), line_of(text, inner.start())))
    return names


def required_addons(text: str) -> list[tuple[str, int]]:
    entries: list[tuple[str, int]] = []
    for match in ARRAY.finditer(text):
        if match.group(1) != "requiredAddons":
            continue
        open_index = match.end() - 1
        close_index = matching_brace(text, open_index)
        body = text[open_index:close_index]
        for value in STRING.finditer(body):
            entries.append((value.group(1), line_of(text, open_index + value.start())))
    return entries


def run() -> list[Finding]:
    findings: list[Finding] = []
    declared: dict[str, str] = {}
    external = allowlist("external_addons")
    parents = {
        name.strip(): parent.strip()
        for entry in allowlist("vanilla_config_parents")
        for name, _, parent in [entry.partition(":")]
        if parent.strip()
    }

    configs = list(tracked("*config.cpp"))
    #  A parked addon still gets read: `config.cpp.disabled` is a live document that someone will
    #  eventually rename back, and a defect frozen into it is a defect waiting to be re-enabled.
    #  The one filesystem walk in the whole suite, because a .disabled file is untracked by
    #  definition. Dot-prefixed folders are skipped for the same reason _EnumPaths.bat skips them:
    #  .claude/worktrees/ holds other branches' sources, and reading those would report a
    #  half-finished experiment as a defect in the checked-in tree.
    #  Test the RELATIVE parts, not the absolute ones: the repo itself may live under a
    #  dot-prefixed directory (it does when checked out as a worktree under .claude/worktrees/),
    #  which would otherwise filter out every file and silently empty this list.
    disabled = []
    for absolute in repo_root().glob("**/config.cpp.disabled"):
        relative = absolute.relative_to(repo_root())
        if any(part.startswith(".") for part in relative.parts):
            continue
        disabled.append(str(relative).replace("\\", "/"))

    for path in configs + disabled:
        text = strip_comments(read(path))
        depths = depth_map(text)

        names = addon_names(text)
        if not names and path in configs:
            findings.append(error(
                "declares no CfgPatches class - the build makes this folder a PBO regardless, but "
                "nothing can depend on it and its load order is unconstrained",
                path,
            ))
        for name, line in names:
            if name in declared:
                findings.append(error(
                    f"CfgPatches class `{name}` is also declared in {declared[name]} - addon "
                    f"names are one flat namespace",
                    path, line,
                ))
            else:
                declared[name] = path

        #  Missing comma between adjacent array entries: a rapify syntax error that packs fine and
        #  only surfaces when the game loads the module.
        for match in ARRAY.finditer(text):
            open_index = match.end() - 1
            close_index = matching_brace(text, open_index)
            body = text[open_index:close_index]
            for gap in ADJACENT_STRINGS.finditer(body):
                findings.append(error(
                    f"{match.group(1)}[] has two entries with no comma between them - rapify "
                    f"rejects this, and packing succeeds regardless so it only shows when the "
                    f"module loads",
                    path, line_of(text, open_index + gap.start()),
                ))

        #  A vanilla class redeclared without the parent vanilla gave it.
        for match in CLASS.finditer(text):
            name, parent = match.group(1), match.group(2)
            if match.group(3) != "{" or name not in parents:
                continue
            if parent != parents[name]:
                findings.append(error(
                    f"redeclares `{name}` " +
                    (f"as `: {parent}`" if parent else "with no parent") +
                    f" - vanilla declares it `: {parents[name]}`, and omitting the parent REPLACES "
                    f"the class rather than merge-patching it, silently stripping every inherited "
                    f"property",
                    path, line_of(text, match.start()),
                ))

        del depths  # only needed inside addon_names; kept explicit so the intent is not misread

    #  requiredAddons resolution, once every config has contributed its declarations.
    for path in configs + disabled:
        text = strip_comments(read(path))
        for entry, line in required_addons(text):
            if entry in declared or entry in external:
                continue
            findings.append(error(
                f"requiredAddons names `{entry}`, which is neither declared in this repo nor "
                f"listed in Tools/allowlists/external_addons.txt - a stale entry stops "
                f"constraining load order without failing the build",
                path, line,
            ))

    return findings
