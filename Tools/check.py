#!/usr/bin/env python3
"""
 *  Static checks for the Vigrid-BattleRoyale sources.
 *
 *  These do NOT compile anything and are not a substitute for the real validation loop
 *  (Deploy.bat -> LaunchOffline.bat -> read the .rpt). A hosted runner cannot build this mod at
 *  all: binarize/rapify/pack need DayZ Tools, a mounted P: and Windows.
 *
 *  What they cover instead is the class of defect this codebase actually suffers from - things
 *  that fail SILENTLY at runtime with a clean .rpt, and otherwise cost a 2-5 minute build plus a
 *  launch to discover. A missing stringtable key, a typo'd layout path, a BattleRoyale* symbol
 *  leaking into a standalone addon, a MENU_* id collision. Every one of those is decidable from
 *  the source text alone, in under a second, for free.
 *
 *  Usage:
 *      python Tools/check.py                 run everything
 *      python Tools/check.py --list          name every check
 *      python Tools/check.py --only rpc      run one (repeatable, or comma-separated)
 *      python Tools/check.py -W              treat warnings as errors
 *
 *  Exit code is 0 when no errors were found, 1 otherwise. Warnings do not fail the run unless -W
 *  is passed, so a known-benign finding does not have to be silenced to keep the tree green.
 */
"""

from __future__ import annotations

import argparse
import importlib.util
import os
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from Checks._source import ERROR, WARNING, Finding  # noqa: E402

CHECKS_DIR = Path(__file__).resolve().parent / "Checks"
IN_GITHUB = os.environ.get("GITHUB_ACTIONS") == "true"


class Check:
    def __init__(self, name: str, summary: str, run):
        self.name = name
        self.summary = summary
        self.run = run


def discover() -> list[Check]:
    checks: list[Check] = []
    for path in sorted(CHECKS_DIR.glob("*.py")):
        if path.name.startswith("_"):
            continue
        spec = importlib.util.spec_from_file_location(f"Checks.{path.stem}", path)
        if spec is None or spec.loader is None:
            continue
        module = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = module
        spec.loader.exec_module(module)
        name = getattr(module, "NAME", path.stem)
        summary = getattr(module, "SUMMARY", "")
        run = getattr(module, "run", None)
        if run is None:
            raise SystemExit(f"{path.name} defines no run()")
        checks.append(Check(name, summary, run))
    return checks


# --------------------------------------------------------------------------------------------
#  Reporting
# --------------------------------------------------------------------------------------------

USE_COLOR = sys.stdout.isatty() and os.environ.get("NO_COLOR") is None


def paint(text: str, code: str) -> str:
    if not USE_COLOR:
        return text
    return f"\033[{code}m{text}\033[0m"


def annotate(check: str, finding: Finding) -> None:
    """Emit a GitHub Actions workflow command so the finding lands on the PR diff."""
    if not IN_GITHUB:
        return
    bits = []
    if finding.path:
        bits.append(f"file={finding.path}")
    if finding.line:
        bits.append(f"line={finding.line}")
    bits.append(f"title={check}")
    message = finding.message.replace("\n", "%0A")
    print(f"::{finding.level} {','.join(bits)}::{message}")


def report(check: Check, findings: list[Finding], elapsed: float) -> None:
    errors = [f for f in findings if f.level == ERROR]
    warnings = [f for f in findings if f.level == WARNING]

    if errors:
        badge = paint("FAIL", "1;31")
    elif warnings:
        badge = paint("WARN", "1;33")
    else:
        badge = paint("ok  ", "1;32")

    tail = f"({elapsed * 1000:.0f} ms)"
    print(f"  {badge}  {check.name:<18} {check.summary} {paint(tail, '2')}")

    for finding in findings:
        marker = paint("error", "31") if finding.level == ERROR else paint("warning", "33")
        where = finding.location()
        prefix = f"          {where}: " if where else "          "
        print(f"{prefix}{marker}: {finding.message}")
        annotate(check.name, finding)


# --------------------------------------------------------------------------------------------

def main() -> int:
    parser = argparse.ArgumentParser(description="Static checks for Vigrid-BattleRoyale.")
    parser.add_argument("--list", action="store_true", help="name every check and exit")
    parser.add_argument("--only", action="append", default=[],
                        help="run only these checks (repeatable, or comma-separated)")
    parser.add_argument("-W", "--warnings-as-errors", action="store_true",
                        help="exit non-zero on warnings too")
    args = parser.parse_args()

    checks = discover()

    if args.list:
        for check in checks:
            print(f"  {check.name:<18} {check.summary}")
        return 0

    wanted: set[str] = set()
    for item in args.only:
        wanted.update(part.strip() for part in item.split(",") if part.strip())
    if wanted:
        known = {c.name for c in checks}
        unknown = wanted - known
        if unknown:
            print(f"unknown check(s): {', '.join(sorted(unknown))}", file=sys.stderr)
            print(f"known: {', '.join(sorted(known))}", file=sys.stderr)
            return 2
        checks = [c for c in checks if c.name in wanted]

    print()
    print(paint("  Vigrid-BattleRoyale static checks", "1"))
    print()

    total_errors = 0
    total_warnings = 0
    for check in checks:
        started = time.perf_counter()
        findings = list(check.run())
        report(check, findings, time.perf_counter() - started)
        total_errors += sum(1 for f in findings if f.level == ERROR)
        total_warnings += sum(1 for f in findings if f.level == WARNING)

    print()
    if total_errors:
        verdict = paint(f"{total_errors} error(s)", "1;31")
    else:
        verdict = paint("no errors", "1;32")
    print(f"  {len(checks)} checks, {verdict}, {total_warnings} warning(s)")
    print()

    if total_errors:
        return 1
    if total_warnings and args.warnings_as_errors:
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
