#!/usr/bin/env python3
"""Static contract checks for the Launcher console / NucleoOS USB boundary."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "launcher/src/idf/launcher_platform.cpp"
PATCH = ROOT / "scripts/apply-roza-launcher-platform.py"


def main() -> int:
    if not SOURCE.is_file() or not PATCH.is_file():
        raise SystemExit("Source Launcher ou adaptateur ROZA absent.")

    source = SOURCE.read_text(encoding="utf-8")
    patch = PATCH.read_text(encoding="utf-8")

    required_source = (
        '#include <esp_log.h>',
        'esp_log_writev(ESP_LOG_INFO, kLauncherLogTag, fmt, args)',
        'esp_log_write(ESP_LOG_INFO, kLauncherLogTag, "%s", text)',
        'void launcherConsoleBegin(unsigned long baud) { (void)baud; }',
        'void launcherConsoleFlush() {}',
        'void launcherConsoleEnd() {}',
    )
    missing = [item for item in required_source if item not in source]
    if missing:
        raise SystemExit("Backend console incomplet : " + ", ".join(missing))

    forbidden = (
        '#include <Arduino.h>',
        '#include <HardwareSerial.h>',
        'Serial.begin(',
        'Serial.end(',
        'Serial.flush(',
        'Serial.write(',
        'Serial.vprintf(',
        'Serial.setTxTimeoutMs(',
    )
    present = [item for item in forbidden if item in source]
    if present:
        raise SystemExit("Le backend Launcher reprend l'USB NucleoOS : " + ", ".join(present))

    if 'PINNED_LAUNCHER = "4b06de691df15b0fe6a9f09f56d1d107bbf32b12"' not in patch:
        raise SystemExit("L'adaptateur ne verrouille pas la revision Launcher approuvee.")
    if "old_count == 1 and new_count == 0" not in patch or "old_count == 0 and new_count == 1" not in patch:
        raise SystemExit("L'adaptateur ne garantit pas un patch exact et idempotent.")

    print("ROZA Launcher console ownership: PASS")
    print(" - Launcher diagnostics use the ESP-IDF log pipeline")
    print(" - Serial/USB CDC is never started, flushed, written or stopped by Launcher")
    print(" - the patch is pinned and idempotent")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
