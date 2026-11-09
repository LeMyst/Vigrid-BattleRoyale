# Copilot Instructions for Vigrid Battle Royale

Full guidance lives in **[`CLAUDE.md`](../CLAUDE.md)** at the repository root — read it
before making changes. It is the single source of truth for build commands,
architecture, and conventions. Do not duplicate it here.

The constraints below are repeated because ignoring them produces code that will not
compile or will silently run on the wrong side.

## EnfusionScript

- No ternary operator (`a ? b : c`). Use `if`/`else`.
- No multi-line `if` conditions — the whole condition must be on one line.
- One declaration per variable name per method scope, even across disjoint branches.

## Client/Server

`Scripts/Client/` and `Scripts/Server/` are not a runtime split — both PBOs load on
both sides. Execution is gated by the preprocessor guard on line 1 of each file:
`#ifdef SERVER` for server-only, `#ifndef SERVER` for client-only, no guard for shared.
Always add the correct guard.

## Build

Windows only, requires DayZ Tools and a mounted `P:\` drive.
Run `Deploy.bat` from `Workbench/Batchfiles/` (cwd matters). There are no unit tests,
but `python Tools/check.py -W` runs 15 static source checks, and CI
(`.github/workflows/checks.yml`) runs it on every push to `main` and every PR. It compiles
nothing, so also validate by launching with `DayZDiag_x64.exe` and reading the `.rpt` log.
