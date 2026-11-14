#!/usr/bin/env python3
"""Route Launcher console diagnostics through NucleoOS's ESP-IDF log owner."""
from __future__ import annotations

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
LAUNCHER_ROOT = ROOT / "launcher"
SOURCE = LAUNCHER_ROOT / "src/idf/launcher_platform.cpp"
PINNED_LAUNCHER = "4b06de691df15b0fe6a9f09f56d1d107bbf32b12"

UPSTREAM = """#include "launcher_platform.h"

#include <Arduino.h>
#include <HardwareSerial.h>
#include <cstdarg>
#include <cstring>

void launcherConsolePrintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    Serial.vprintf(fmt, args);
    va_end(args);
}

void launcherConsolePrint(const char *text) { Serial.print(text); }

void launcherConsolePrintln(const char *text) { Serial.println(text); }

void launcherConsolePrintLong(const char *text) {
    if (!text) return;
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.setTxTimeoutMs(2000);
#endif
    const uint8_t *payload = reinterpret_cast<const uint8_t *>(text);
    size_t remaining = strlen(text);
    uint32_t deadline = launcherMillis() + 2000;
    while (remaining > 0) {
        size_t chunk = remaining > 64 ? 64 : remaining;
        size_t written = Serial.write(payload, chunk);
        if (written > 0) {
            payload += written;
            remaining -= written;
            deadline = launcherMillis() + 2000;
        } else {
            if ((int32_t)(launcherMillis() - deadline) >= 0) break;
            launcherDelayMs(1);
        }
    }
    Serial.println();
    Serial.flush();
#if ARDUINO_USB_CDC_ON_BOOT
    Serial.setTxTimeoutMs(0);
#endif
}

void launcherConsoleBegin(unsigned long baud) { Serial.begin(baud); }

void launcherConsoleFlush() { Serial.flush(); }

void launcherConsoleEnd() { Serial.end(); }
"""

ROZA = """#include "launcher_platform.h"

#include <esp_log.h>
#include <cstdarg>

namespace {
constexpr const char *kLauncherLogTag = "launcher";

void launcherLogText(const char *text)
{
    if (text) esp_log_write(ESP_LOG_INFO, kLauncherLogTag, "%s", text);
}
} // namespace

/*
 * NucleoOS owns USB CDC for the entire device. Launcher diagnostics use the
 * ESP-IDF logger instead, so they enter NucleoOS's bounded RAM log ring and
 * never start, flush, or stop the shared USB console.
 */
void launcherConsolePrintf(const char *fmt, ...)
{
    if (!fmt) return;
    va_list args;
    va_start(args, fmt);
    esp_log_writev(ESP_LOG_INFO, kLauncherLogTag, fmt, args);
    va_end(args);
}

void launcherConsolePrint(const char *text) { launcherLogText(text); }

void launcherConsolePrintln(const char *text) { launcherLogText(text); }

void launcherConsolePrintLong(const char *text) { launcherLogText(text); }

// USB CDC lifecycle is initialized once by NucleoOS, never by a Launcher library.
void launcherConsoleBegin(unsigned long baud) { (void)baud; }

void launcherConsoleFlush() {}

void launcherConsoleEnd() {}
"""


def git_head(path: Path) -> str:
    result = subprocess.run(
        ["git", "-C", str(path), "rev-parse", "HEAD"],
        check=False, capture_output=True, text=True,
    )
    if result.returncode:
        raise SystemExit(f"Impossible de lire la revision Git : {path}")
    return result.stdout.strip()


def verify_patch() -> None:
    if not SOURCE.is_file():
        raise SystemExit(f"Source Launcher absente : {SOURCE}")
    source = SOURCE.read_text(encoding="utf-8")
    required = (
        '#include <esp_log.h>',
        'esp_log_writev(ESP_LOG_INFO, kLauncherLogTag, fmt, args)',
        'esp_log_write(ESP_LOG_INFO, kLauncherLogTag, "%s", text)',
        'void launcherConsoleBegin(unsigned long baud) { (void)baud; }',
        'void launcherConsoleFlush() {}',
        'void launcherConsoleEnd() {}',
    )
    missing = [item for item in required if item not in source]
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
    if missing or present:
        details = []
        if missing:
            details.append("adaptation incomplete: " + ", ".join(missing))
        if present:
            details.append("USB ownership violation: " + ", ".join(present))
        raise SystemExit("; ".join(details))


def main() -> int:
    actual = git_head(LAUNCHER_ROOT)
    if actual != PINNED_LAUNCHER:
        raise SystemExit(
            f"Launcher inattendu : attendu {PINNED_LAUNCHER}, obtenu {actual}. "
            "Aucun patch applique."
        )

    source = SOURCE.read_text(encoding="utf-8") if SOURCE.is_file() else ""
    old_count = source.count(UPSTREAM)
    new_count = source.count(ROZA)
    if old_count == 1 and new_count == 0:
        SOURCE.write_text(source.replace(UPSTREAM, ROZA, 1), encoding="utf-8")
        changed = True
    elif old_count == 0 and new_count == 1:
        changed = False
    else:
        raise SystemExit(
            "Patch console Launcher refuse : "
            f"source upstream={old_count}, adaptation ROZA={new_count}; revue requise."
        )

    verify_patch()
    print(
        "Console Launcher routee vers ESP-IDF — "
        + ("source adaptee." if changed else "source deja adaptee.")
    )
    print("NucleoOS conserve l'unique propriete du cycle USB CDC.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
