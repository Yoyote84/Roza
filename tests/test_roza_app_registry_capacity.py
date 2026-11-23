#!/usr/bin/env python3
"""Verify ROZA's native-app registry leaves useful capacity for NucleoOS Store apps."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "nucleoos/firmware/components/nucleo_app/include/nucleo_app.h"
SOURCE = ROOT / "nucleoos/firmware/components/nucleo_app/nucleo_app.cpp"
PATCH = ROOT / "scripts/apply-roza-nucleoos.py"
MIN_STORE_SLOTS = 12


def main() -> int:
    header = HEADER.read_text(encoding="utf-8")
    source = SOURCE.read_text(encoding="utf-8")
    patch = PATCH.read_text(encoding="utf-8")

    capacity_match = re.search(r"^#define NUCLEO_APP_MAX (\d+)$", header, re.MULTILINE)
    if not capacity_match:
        raise SystemExit("NUCLEO_APP_MAX absent or malformed")
    capacity = int(capacity_match.group(1))

    builtin = re.search(
        r"void nucleo_app_register_builtins\(void\)\s*\{(.*?)\n\}",
        source,
        re.DOTALL,
    )
    if not builtin:
        raise SystemExit("nucleo_app_register_builtins body not found")
    builtin_calls = re.findall(r"nucleo_register_[A-Za-z0-9_]+\(\)", builtin.group(1))
    roza_apps = set(re.findall(r"roza_register_([A-Za-z0-9_]+)\(\);", patch))
    if len(roza_apps) != 6:
        raise SystemExit(f"Expected six native ROZA app hooks, found {sorted(roza_apps)}")

    dynamic_room = capacity - len(builtin_calls) - len(roza_apps)
    if dynamic_room < MIN_STORE_SLOTS:
        raise SystemExit(
            f"Store registry room too small: {dynamic_room} slots "
            f"({capacity} capacity, {len(builtin_calls)} built-ins, {len(roza_apps)} ROZA apps)"
        )

    print(
        "ROZA app registry capacity: PASS "
        f"({capacity}, {len(builtin_calls)} built-ins, {len(roza_apps)} ROZA apps, "
        f"{dynamic_room} dynamic Store slots)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
