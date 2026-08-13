# Tools

Static checks over the tracked sources. No DayZ Tools, no `P:`, no build — Python 3 stdlib only.

```bash
python Tools/check.py
```

or, from `Workbench/Batchfiles` like every other batch file in this repo:

```bash
Check.bat
```

`--list` names every check, `--only <name>` runs one (repeatable, or comma-separated), `-W` makes
warnings fail the run. Exit code is 0 when there are no errors.

## What this is and is not

**It compiles nothing.** The real validation loop is unchanged — `Deploy.bat` → `LaunchOffline.bat`
→ read the `.rpt`. Nothing here can tell you whether a script builds, and passing checks is not
evidence that a change works.

What it covers is the class of defect this codebase actually suffers from: **things that fail
silently at runtime with a clean `.rpt`**, and otherwise cost a 2–5 minute build plus a launch to
discover. A missing stringtable key. A typo'd layout path, which draws nothing and logs nothing. A
`BattleRoyale*` symbol leaking into a standalone addon. A `MENU_*` id collision, which does not
error — one menu just opens the other.

| Check | What it asserts |
|---|---|
| `stringtable` | every `STR_*` referenced is defined, **in its own addon's** table |
| `asset-paths` | every `Vigrid-BattleRoyale/…` reference resolves to a tracked file |
| `guards` | every `Scripts/Server` file opens with `#ifdef SERVER` |
| `discipline` | standalone addons name no `BattleRoyale*` symbol; host calls into them are `#ifdef`-guarded |
| `menu-ids` | no two `MENU_*` constants share an id |
| `rpc` | every `AddRPC` name has a matching CF handler method |
| `version` | `BATTLEROYALE_VERSION` and `mod.cpp` agree |
| `configs` | `CfgPatches` names unique, `requiredAddons` resolve, no stripped vanilla parents, no missing array commas |
| `data` | tracked JSON parses; no UTF-8 BOM on any data file |
| `extra-index` | every `Extra/` addon has a README, an index entry and a config |
| `enfusion` | no ternary operator, no multi-line `if`/`while` conditions |
| `settings-version` | a new settings field bumps `version`; a new `ref array` gets an `Upgrade()` branch |

## Two rules for anyone adding a check

**Enumerate with `git ls-files`, never a filesystem walk.** A recursive glob descends into
`.claude/worktrees/`, and an in-progress worktree there will report its own half-finished sources
as defects in the checked-in tree. This is the same rule `Workbench/Batchfiles/_EnumPaths.bat`
already applies to the build.

**Pick the right stripper.** `strip_code()` blanks comments *and* string contents — right for
identifier matching, because nearly every apparent discipline violation in this repo is a doc
comment explaining the rule it appears to break. `strip_comments()` keeps strings — right whenever
the thing being checked *is* a string literal. Getting this backwards does not produce a wrong
answer, it produces **no** answer: the `rpc` and `version` checks were both written with
`strip_code` first, found nothing at all, and reported a clean pass.

That is the failure mode to design against generally. A check that passes on a clean tree and also
passes on a broken one is worse than no check, and nothing about it looks wrong from the outside.

So there is an acceptance gate — **run it after changing any check, and add a probe with any new
one**:

```bash
python Tools/probe.py
```

It breaks something on purpose for each check, confirms the check fires, and restores the file
byte-exactly. It requires an actual reported finding rather than merely a non-zero exit, because a
*crashed* check also exits non-zero: an early version of that harness handed `subprocess` a
stripped environment, which hid `git` from `check.py`, and it cheerfully reported 20/20 without a
single genuine detection behind it.

Three of the original probes were themselves wrong — one asserted a rule that only applies to
referenced keys while never referencing anything, one used a stringtable key that does not exist,
and one "broke" the `Extra/` index by renaming a link label rather than its target, which the
check was too weak to catch anyway. Two were probe bugs; the third was a real weakness and the
check got stricter. Expect to debug the probe as often as the check.

## Allowlists

`Tools/allowlists/*.txt`, one entry per line as `entry  # reason`. They are documentation as much
as configuration — `unguarded_server.txt` is currently the only place the two deliberately
unguarded server files have their reason written down. `guards` reports a stale entry as an error,
because an exemption that outlives its subject is how a rule quietly stops applying.
