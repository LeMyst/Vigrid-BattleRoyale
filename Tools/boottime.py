#!/usr/bin/env python3
"""
 *  Turn a server boot into a number, by reading the markers the engine already prints.
 *
 *      python Tools/boottime.py
 *
 *  Why this exists: boot time here is NOT stable. Two consecutive boots of an identical build
 *  measured 3m36s and 2m27s - a 47% spread, cold page cache vs warm. So a single slow boot proves
 *  nothing, and "it feels slower than last week" cannot be settled by feel. Worse, ClearLogs.bat
 *  deletes *.log and *.rpt from the profile directory at the START of every launch, so each run
 *  destroys the evidence of the one before it and there is never a baseline to compare against.
 *
 *  This reads the newest .RPT, prints where the time went, and appends one row to
 *  Workbench/Logs/boottime.csv - which survives ClearLogs because it lives in the repo, not in the
 *  profile directory. Run it after a few boots before believing any regression or any fix.
 *
 *  It parses only markers the engine prints unprompted. Nothing here needs a build, a script
 *  change, or a diag flag, so it works on a stock server and on any past .RPT you still have:
 *
 *      python Tools/boottime.py --rpt "F:/ServerProfile/DayZDiag_x64_2026-08-14_11-01-39.RPT"
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
from datetime import datetime
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
CSV_PATH = REPO / "Workbench" / "Logs" / "boottime.csv"

#  Process start comes from the filename rather than the first logged line: the engine writes its
#  first timestamp some seconds in, and that gap is exactly the PBO-load cost we want to measure.
RPT_NAME = re.compile(r"_(\d{4})-(\d{2})-(\d{2})_(\d{2})-(\d{2})-(\d{2})\.RPT$", re.IGNORECASE)
TIME = re.compile(r"^\s*(\d{2}):(\d{2}):(\d{2})(?:\.(\d+))?\s")

LOOT_DONE = re.compile(
    r"Initially \(re\)spawned:(\d+).*?Total in Map:\s*(\d+)\s+at\s+(\d+)\s*\(sec\)"
)

#  Ordered: each phase ends at the first line matching its marker, so the table is a straight walk
#  down the boot. "ready" is the dedicated-server readiness marker - on a SERVER "[IdleMode]
#  Entering IN" means the mission is up and the login queue is open (it means the opposite offline,
#  where it is the frozen-on-IDLE-MODE-ACTIVE failure).
PHASES = [
    ("load", "engine + PBO load", "Hostname of server:"),
    ("world", "world, scripts, mission OnInit", "[CE][TypeSetup]"),
    ("ce_loot", "CE loot spawn", None),            # matched by LOOT_DONE
    ("ce_vehicles", "CE vehicle respawn", None),   # last [CE][VehicleRespawner] line
    ("ready", "finalise + first save", "[IdleMode] Entering IN"),
]


def parse_config() -> dict[str, str]:
    """project.cfg then user.cfg, same precedence and comment rules as _Config.bat."""
    values: dict[str, str] = {}
    for name in ("project.cfg", "user.cfg"):
        path = REPO / "Workbench" / name
        if not path.exists():
            continue
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            if not line.strip() or line.lstrip()[:1] in (";", "#"):
                continue
            key, sep, value = line.partition("=")
            if sep:
                values[key.strip()] = value.strip()
    return values


def newest_rpt(profile: Path) -> Path | None:
    candidates = [p for p in profile.glob("*.RPT") if RPT_NAME.search(p.name)]
    if not candidates:
        return None
    return max(candidates, key=lambda p: p.stat().st_mtime)


def start_time(rpt: Path) -> datetime | None:
    match = RPT_NAME.search(rpt.name)
    if not match:
        return None
    year, month, day, hour, minute, second = (int(g) for g in match.groups())
    return datetime(year, month, day, hour, minute, second)


def scan(rpt: Path, origin: datetime) -> dict:
    """Walk the RPT once, recording seconds-since-start for each marker.

    Times are wall-clock HH:MM:SS with no date, so a boot spanning midnight would run backwards.
    Track the previous value and add a day whenever it does.
    """
    found: dict[str, float] = {}
    stats: dict[str, int] = {}
    base = origin.hour * 3600 + origin.minute * 60 + origin.second
    previous = float(base)
    day = 0.0

    for line in rpt.read_text(encoding="utf-8", errors="replace").splitlines():
        stamp = TIME.match(line)
        if not stamp:
            continue
        hour, minute, second, millis = stamp.groups()
        now = int(hour) * 3600 + int(minute) * 60 + int(second) + (int(millis or 0) / 1000)
        now += day
        if now < previous - 1:
            day += 86400
            now += 86400
        previous = now
        offset = now - base

        loot = LOOT_DONE.search(line)
        if loot and "ce_loot" not in found:
            found["ce_loot"] = offset
            stats["ce_items"] = int(loot.group(1))
            stats["ce_total_in_map"] = int(loot.group(2))
            stats["ce_reported_s"] = int(loot.group(3))

        #  Vehicles keep going after the first line, so this one deliberately overwrites: the phase
        #  ends at the LAST respawner line, not the first.
        if "[CE][VehicleRespawner]" in line:
            found["ce_vehicles"] = offset
            if "Failed to spawn" in line:
                stats["vehicle_shortfalls"] = stats.get("vehicle_shortfalls", 0) + 1

        for key, _, marker in PHASES:
            if marker and marker in line and key not in found:
                found[key] = offset

        #  Stop at readiness. The CE respawners keep running for the life of the server - the
        #  vehicle one logs its first "Respawning:" round within a minute - so a scan that ran to
        #  EOF put ce_vehicles PAST the ready marker and reported a negative final phase.
        if "ready" in found:
            break

    return {"marks": found, "stats": stats}


def script_log_lines(profile: Path, origin: datetime) -> int | None:
    """Line count of the script log belonging to this boot.

    Worth carrying in the CSV: the script log is the cheapest proxy for per-item work during the CE
    phase, and it is where a chatty new hook shows up first.
    """
    best = None
    for path in profile.glob("script_*.log"):
        stamp = re.search(r"_(\d{4})-(\d{2})-(\d{2})_(\d{2})-(\d{2})-(\d{2})\.log$", path.name)
        if not stamp:
            continue
        when = datetime(*(int(g) for g in stamp.groups()))
        #  The script log opens a few seconds AFTER the process starts. Anything earlier belongs to
        #  a previous boot that ClearLogs did not reach.
        if when < origin:
            continue
        if best is None or when < best[0]:
            best = (when, path)
    if best is None:
        return None
    with best[1].open("rb") as handle:
        return sum(1 for _ in handle)


def humanise(seconds: float) -> str:
    return f"{int(seconds) // 60}m{int(seconds) % 60:02d}s"


def report(rpt: Path, origin: datetime, scanned: dict, log_lines: int | None) -> dict:
    marks, stats = scanned["marks"], scanned["stats"]
    total = marks.get("ready")

    print()
    print(f"  {rpt.name}")
    print(f"  started {origin:%Y-%m-%d %H:%M:%S}")
    print()

    row = {"when": origin.isoformat(sep=" "), "rpt": rpt.name}
    previous = 0.0
    for key, label, _ in PHASES:
        at = marks.get(key)
        if at is None:
            print(f"    {'--':>7}  {'':>7}  {label}  (marker not found)")
            row[key] = ""
            continue
        span = at - previous
        row[key] = f"{span:.1f}"
        print(f"    {humanise(span):>7}  {humanise(at):>7}  {label}")
        previous = at

    print()
    if total is not None:
        print(f"    TOTAL   {humanise(total)}  to a joinable server")
        row["total"] = f"{total:.1f}"
    else:
        #  No readiness marker means the boot never finished - a crash, or a still-running server.
        print("    TOTAL   -- ([IdleMode] Entering IN never logged; boot did not complete)")
        row["total"] = ""

    if stats:
        print()
        if "ce_items" in stats:
            print(f"    CE spawned {stats['ce_items']} items "
                  f"(engine reported {stats['ce_reported_s']}s)")
        if "vehicle_shortfalls" in stats:
            print(f"    {stats['vehicle_shortfalls']} vehicle type(s) hit their attempt limit")
    if log_lines is not None:
        print(f"    script log: {log_lines} lines")

    print()
    row["ce_items"] = stats.get("ce_items", "")
    row["ce_reported_s"] = stats.get("ce_reported_s", "")
    row["vehicle_shortfalls"] = stats.get("vehicle_shortfalls", "")
    row["script_log_lines"] = log_lines if log_lines is not None else ""
    return row


FIELDS = ["when", "rpt", "total", "load", "world", "ce_loot", "ce_vehicles", "ready",
          "ce_items", "ce_reported_s", "vehicle_shortfalls", "script_log_lines"]


def already_recorded(row: dict) -> bool:
    """Has this exact boot been filed already?

    The tool is meant to be run after a launch, and re-running it without a new launch reads the
    same .RPT again. Appending that would inflate the sample with duplicates and quietly narrow the
    spread - the one number the CSV exists to report honestly.
    """
    if not CSV_PATH.exists():
        return False
    with CSV_PATH.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    return bool(rows) and rows[-1].get("rpt") == row["rpt"]


def append_csv(row: dict) -> None:
    CSV_PATH.parent.mkdir(parents=True, exist_ok=True)
    fresh = not CSV_PATH.exists()
    with CSV_PATH.open("a", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=FIELDS, extrasaction="ignore")
        if fresh:
            writer.writeheader()
        writer.writerow(row)


def show_history(limit: int = 10) -> None:
    if not CSV_PATH.exists():
        return
    with CSV_PATH.open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))
    if len(rows) < 2:
        return

    recent = rows[-limit:]
    print(f"  last {len(recent)} boots (Workbench/Logs/boottime.csv):")
    for entry in recent:
        total = entry.get("total") or ""
        shown = humanise(float(total)) if total else "--"
        print(f"    {entry['when']}  {shown:>7}  ce_items={entry.get('ce_items') or '--'}")

    totals = [float(e["total"]) for e in rows if e.get("total")]
    if len(totals) >= 2:
        print()
        print(f"  spread over {len(totals)} boots: "
              f"{humanise(min(totals))} - {humanise(max(totals))}, "
              f"mean {humanise(sum(totals) / len(totals))}")
        print()


def main() -> int:
    parser = argparse.ArgumentParser(description="Report where DayZ server boot time went.")
    parser.add_argument("--rpt", help="path to a specific .RPT (default: newest in the profile)")
    parser.add_argument("--profile", help="server profile directory (default: from user.cfg)")
    parser.add_argument("--no-record", action="store_true",
                        help="print the table but do not append to boottime.csv")
    args = parser.parse_args()

    if args.rpt:
        rpt = Path(args.rpt)
        if not rpt.exists():
            print(f"boottime: no such file: {rpt}", file=sys.stderr)
            return 1
        profile = rpt.parent
    else:
        raw = args.profile or parse_config().get("ServerProfileDirectory")
        if not raw:
            print("boottime: ServerProfileDirectory is not set in Workbench/user.cfg; "
                  "pass --profile or --rpt.", file=sys.stderr)
            return 1
        profile = Path(raw)
        if not profile.is_dir():
            print(f"boottime: profile directory not found: {profile}", file=sys.stderr)
            return 1
        found = newest_rpt(profile)
        if found is None:
            print(f"boottime: no .RPT in {profile} - has the server been launched yet?",
                  file=sys.stderr)
            return 1
        rpt = found

    origin = start_time(rpt)
    if origin is None:
        print(f"boottime: cannot read a start time out of {rpt.name}; expected "
              "DayZDiag_x64_YYYY-MM-DD_HH-MM-SS.RPT", file=sys.stderr)
        return 1

    scanned = scan(rpt, origin)
    row = report(rpt, origin, scanned, script_log_lines(profile, origin))

    if args.no_record:
        pass
    elif already_recorded(row):
        print("  (already recorded - relaunch the server for a new sample)")
        print()
    else:
        append_csv(row)
    show_history()
    return 0


if __name__ == "__main__":
    sys.exit(main())
