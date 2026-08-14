"""
 *  Data-file hygiene.
 *
 *  Two things, both of which fail silently in-game.
 *
 *  A UTF-8 BOM makes DayZ reject a JSON file and fall back to defaults with no error - and
 *  PowerShell 5.1's `-Encoding utf8` writes one by default, so this guards against the local
 *  toolchain rather than against carelessness.
 *
 *  Malformed JSON does the same thing: JsonFileLoader reports the failure to nobody the admin
 *  will read, and the feature just behaves as though the file were absent.
"""

from __future__ import annotations

import json

from Checks._source import Finding, error, read_bytes, tracked

NAME = "data"
SUMMARY = "tracked JSON parses; no UTF-8 BOM on any data file"

BOM = b"\xef\xbb\xbf"
TEXT_ASSETS = ("*.json", "*.csv", "*.xml", "*.layout")


def run() -> list[Finding]:
    findings: list[Finding] = []

    for path in tracked(*TEXT_ASSETS):
        if read_bytes(path).startswith(BOM):
            findings.append(error(
                "starts with a UTF-8 BOM - DayZ rejects the file and silently falls back to "
                "defaults; re-save as UTF-8 without BOM",
                path, 1,
            ))

    for path in tracked("*.json"):
        raw = read_bytes(path)
        try:
            json.loads(raw.decode("utf-8-sig"))
        except UnicodeDecodeError as exc:
            findings.append(error(f"is not valid UTF-8: {exc}", path))
        except json.JSONDecodeError as exc:
            findings.append(error(f"is not valid JSON: {exc.msg}", path, exc.lineno))

    return findings
