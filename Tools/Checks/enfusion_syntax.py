"""
 *  EnfusionScript constraints the compiler enforces, caught before the build.
 *
 *  Neither of these is subtle once the compiler sees it - both are hard compile errors. The point
 *  is purely the feedback loop: finding them here costs a second, finding them the normal way
 *  costs a 2-5 minute Deploy.bat and a launch, and packing succeeds regardless so the failure does
 *  not surface until the game loads the module.
 *
 *  Matching runs on strip_code() output, so a `?` inside a string ("Running in a straight line?
 *  Bold choice.") or a comment is not a ternary. That matters here - this repo's hint strings are
 *  full of question marks.
 *
 *  Deliberately NOT checked: the "Formula too complex" ceiling. It is real - a single expression
 *  of around ten concatenated terms is rejected outright - but BattleRoyaleZone.c:626 concatenates
 *  twelve and compiles, so the threshold cannot be calibrated without the actual compiler and any
 *  number picked here would be superstition.
"""

from __future__ import annotations

import re

from Checks._source import Finding, error, numbered, read, strip_code, tracked

NAME = "enfusion"
SUMMARY = "no ternary operator, no multi-line if/while conditions"

CONDITION = re.compile(r"\b(if|while)\s*\(")


def run() -> list[Finding]:
    findings: list[Finding] = []

    for path in tracked("*.c"):
        for number, line in numbered(strip_code(read(path))):
            if line.lstrip().startswith("#"):
                continue  # preprocessor directive, not an expression

            if "?" in line:
                findings.append(error(
                    "uses `?` - EnfusionScript has no ternary operator; write if/else",
                    path, number,
                ))

            match = CONDITION.search(line)
            if match and line.count("(") != line.count(")"):
                findings.append(error(
                    f"`{match.group(1)}` condition does not close on one line - EnfusionScript "
                    f"requires the whole condition on a single line",
                    path, number,
                ))

    return findings
