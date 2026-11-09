"""Check the GitHub wiki against the code: the three checks CLAUDE.md asks for before a wiki push.

The wiki is a separate repo (LeMyst/Vigrid-BattleRoyale.wiki), cloned beside the primary
checkout as ..\\Vigrid-BattleRoyale.wiki. It restates every settings field by hand, so it drifts
silently; this is what catches it.

    python Tools/wiki_check.py               # the wiki beside the primary checkout
    python Tools/wiki_check.py --wiki PATH   # another clone
    python Tools/wiki_check.py --self-test   # prove each check still fires (run after editing this)

1. links    every internal link names an existing page, and its #anchor an existing heading.
2. json     every ```json block parses as strict JSON, and every top-level key in it is a field of
            a settings class (or is allowlisted in Tools/allowlists/wiki_json_keys.txt).
3. coverage every field of each settings class appears, backquoted, on that class's page.

Exit code is 0 when nothing is reported. Not run by check.py or CI: the wiki is not in this repo.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from Checks._source import allowlist, read, scripts, strip_comments  # noqa: E402

#--- Wiki page -> the settings classes whose every field must be documented on it.
PAGES = {
    "General-Settings.md": ["BattleRoyaleGameData"],
    "Lobby-Settings.md": ["BattleRoyaleLobbyData"],
    "Zone-Settings.md": ["BattleRoyaleZoneData"],
    "Voice-Settings.md": ["BattleRoyaleVoiceData"],
    "Spawns-Settings.md": ["BattleRoyaleSpawnsData"],
    "POIs-Settings.md": ["BattleRoyalePOIsData"],
    "Server-Settings.md": ["BattleRoyaleServerData"],
    "Leaderboard-Settings.md": ["BattleRoyaleLeaderboardData"],
    "Party-Settings.md": ["VigridPartyData"],
    "Addon-Settings.md": ["KillFeedData", "VigridMapData"],
}

LINK = re.compile(r"(?<!!)\[[^\]]*\]\(([^)\s]+)\)")
HEADING = re.compile(r"^(#{1,6})\s+(.*?)\s*#*\s*$")
JSON_BLOCK = re.compile(r"```json[ \t]*\r?\n(.*?)\r?\n```", re.S)
CLASS = re.compile(r"^class\s+(\w+)\b[^\n{]*\{?", re.M)
MEMBER = re.compile(
    r"^\s+(?:ref\s+)?(?:array<[^>]*>|map<[^>]*>|set<[^>]*>|bool|int|float|string|vector)\s+(\w+)\s*(?:=|;)"
)


def default_wiki() -> Path:
    """The clone beside the PRIMARY checkout, found the same way _Config.bat finds the primary."""
    out = subprocess.run(
        ["git", "rev-parse", "--path-format=absolute", "--git-common-dir"],
        capture_output=True, text=True, check=True,
    )
    primary = Path(out.stdout.strip()).parent
    return primary.parent / (primary.name + ".wiki")


def slug(text: str) -> str:
    """GitHub's heading anchor: emphasis and code marks dropped, underscores and hyphens kept."""
    text = re.sub(r"`|\*\*|\*", "", text).strip().lower()
    text = re.sub(r"[^\w\- ]", "", text)
    return text.replace(" ", "-")


def anchors(text: str) -> set[str]:
    found: set[str] = set()
    seen: dict[str, int] = {}
    in_code = False
    for line in text.splitlines():
        if line.startswith("```"):
            in_code = not in_code
        if in_code:
            continue
        m = HEADING.match(line)
        if m:
            s = slug(m.group(2))
            n = seen.get(s, 0)
            found.add(s if n == 0 else f"{s}-{n}")
            seen[s] = n + 1
    return found


def class_fields(wanted: set[str]) -> dict[str, set[str]]:
    """Serialized fields of each wanted class: depth-1 members, minus [NonSerialized()] ones."""
    fields: dict[str, set[str]] = {}
    for rel in scripts():
        text = strip_comments(read(rel))
        for m in CLASS.finditer(text):
            name = m.group(1)
            if name not in wanted:
                continue
            names = fields.setdefault(name, set())
            depth = 0
            started = False
            skip_next = False
            for line in text[m.start():].splitlines():
                if depth == 1:
                    if "NonSerialized" in line:
                        skip_next = True
                    else:
                        mm = MEMBER.match(line)
                        if mm:
                            if not skip_next:
                                names.add(mm.group(1))
                            skip_next = False
                depth += line.count("{") - line.count("}")
                if "{" in line:
                    started = True
                if started and depth <= 0:
                    break
    return fields


def run(wiki: Path, inject: str | None = None) -> list[str]:
    """All findings. `inject` breaks one check on purpose, for --self-test."""
    pages = {p.name: p.read_text(encoding="utf-8") for p in sorted(wiki.glob("*.md"))}
    wanted = {cls for classes in PAGES.values() for cls in classes}
    fields = class_fields(wanted)
    known_keys = set().union(*fields.values()) | set(allowlist("wiki_json_keys"))
    findings: list[str] = []

    for cls in sorted(wanted - set(fields)):
        findings.append(f"settings class {cls} not found in the sources - update PAGES")
    for page in sorted(set(PAGES) - set(pages)):
        findings.append(f"settings page {page} not found in the wiki - update PAGES")

    for name, text in pages.items():
        if inject == "link" and name == "Home.md":
            text += "\n[broken](Zone-Settings#no-such-heading)\n"
        if inject == "json" and name == "Zone-Settings.md":
            text += '\n```json\n{ "not_a_real_zone_key": 1 }\n```\n'
        if inject == "coverage" and name == "Zone-Settings.md":
            text = text.replace("`version`", "")

        for target in LINK.findall(text):
            if re.match(r"[a-z][a-z0-9+.-]*:", target):
                continue
            page, _, anchor = target.partition("#")
            page_file = f"{page}.md" if page else name
            if page_file not in pages:
                findings.append(f"{name}: link to a missing page: {target}")
            elif anchor and anchor.lower() not in anchors(pages[page_file]):
                findings.append(f"{name}: link to a missing heading: {target}")

        for i, block in enumerate(JSON_BLOCK.findall(text), 1):
            try:
                data = json.loads(block)
            except ValueError as exc:
                findings.append(f"{name}: json block {i} is not strict JSON: {exc}")
                continue
            if isinstance(data, dict):
                for key in sorted(set(data) - known_keys):
                    findings.append(f"{name}: json block {i} has a key no settings class declares: {key}")

        for cls in PAGES.get(name, []):
            for field in sorted(fields.get(cls, set())):
                if f"`{field}`" not in text:
                    findings.append(f"{name}: {cls}.{field} is not documented")
    return findings


def self_test(wiki: Path) -> bool:
    """Each injected defect must produce a finding the clean run did not have."""
    clean = set(run(wiki))
    ok = True
    for kind, needle in (("link", "no-such-heading"), ("json", "not_a_real_zone_key"), ("coverage", ".version is not")):
        extra = [f for f in run(wiki, inject=kind) if f not in clean]
        caught = any(needle in f for f in extra)
        print(f"  self-test {kind:<8} {'caught' if caught else 'MISSED'}")
        ok = ok and caught
    return ok


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--wiki", type=Path, help="wiki clone (default: beside the primary checkout)")
    parser.add_argument("--self-test", action="store_true", help="prove every check still fires")
    args = parser.parse_args()

    wiki = args.wiki or default_wiki()
    if not (wiki / "Home.md").is_file():
        print(f"no wiki clone at {wiki} (pass --wiki)")
        return 2

    findings = run(wiki)
    for f in findings:
        print(f"  FAIL  {f}")
    print(f"  {len(list(wiki.glob('*.md')))} pages, {len(findings)} finding(s)  [{wiki}]")

    if args.self_test and not self_test(wiki):
        return 2
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
