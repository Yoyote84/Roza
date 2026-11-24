#!/usr/bin/env python3
"""Vérifications structurelles de la migration BLE NimBLE de ROZA.

Ce test garantit la présence des interfaces et chemins de code attendus.
Il ne remplace pas la compilation PlatformIO ni les essais radio sur Cardputer ADV.
"""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
PLATFORM = (ROOT / "platformio.ini").read_text(encoding="utf-8")
BT_CPP = (ROOT / "src" / "bt.cpp").read_text(encoding="utf-8")
BT_H = (ROOT / "src" / "bt.h").read_text(encoding="utf-8")
COMPAT = (ROOT / "src" / "bt_nimble_compat.h").read_text(encoding="utf-8")


def check(condition: bool, message: str) -> None:
    if not condition:
        print(f"ECHEC: {message}", file=sys.stderr)
        raise SystemExit(1)
    print(f"OK: {message}")


def without_comments(source: str) -> str:
    source = re.sub(r"/\*[\s\S]*?\*/", "", source)
    return re.sub(r"//[^\n]*", "", source)


active = without_comments(BT_CPP)

check("h2zero/NimBLE-Arduino@2.5.2" in PLATFORM,
      "NimBLE-Arduino 2.5.2 est épinglé")
check("-DCONFIG_BT_NIMBLE_EXT_ADV=1" in PLATFORM,
      "les annonces et scans BLE 5 étendus sont activés")
check('#include "bt_nimble_compat.h"' in BT_CPP,
      "bt.cpp utilise la façade de compatibilité ROZA")
check("class RozaBleScan" in COMPAT and "NimBLEScanCallbacks" in COMPAT,
      "le scanner NimBLE et son pont de callbacks sont présents")
check("class RozaBleHidDevice" in COMPAT and "NimBLEHIDDevice" in COMPAT,
      "le clavier HID NimBLE est présent")
check("class RozaBleAdvertising" in COMPAT and "NimBLEExtAdvertising" in COMPAT,
      "la publicité BLE legacy/étendue est présente")

# Les anciennes fonctions publiques du module doivent toujours être définies.
public_functions = (
    "btScan", "btKeyboardStart", "btKeyboardStop", "btKeyboardSendKey",
    "btKeyboardType", "btKeyboardImeMode", "btKeyboardToggleIme",
    "btKeyboardSendRaw", "btKeyUsageFor", "btMediaStart", "btMediaSend",
    "btRadarStart", "btRadarStop", "btRadarUpdate", "btRadarToggleBeep",
    "btDumpRun", "btExtScanRun", "btStreamStart", "btStreamStop",
    "btStreamIsActive", "btStreamPump", "btStreamDevCount",
    "btStreamPktCount", "btExit", "btReleaseForOtherApps",
    "btHoldsHeap", "btAlternations", "btMemoryLow", "btMemoryCritical",
)
for name in public_functions:
    check(re.search(rf"\b{name}\s*\(", BT_H) is not None,
          f"interface publique conservée : {name}")
    check(re.search(rf"\b{name}\s*\(", BT_CPP) is not None,
          f"implémentation présente : {name}")

# Contrôle que les parcours avancés sont bien adaptés à NimBLE.
for token, label in (
    ("NimBLEScan::SCAN_ALL", "scan 1M + Coded PHY"),
    ("class BtExtScanCallbacks", "capture étendue BTEXT"),
    ("class BtsScanCallbacks", "flux BLE du mode PC"),
    ("xQueueSend(q, &report, 0)", "mise en file des annonces brutes"),
    ("ridDecode(", "décodage OpenDroneID conservé"),
    ("btRadarStart(", "radar RSSI conservé"),
    ("btMediaSend(", "commandes multimédias HID conservées"),
    ("btKeyboardSendRaw(", "envoi clavier HID conservé"),
    ("RozaBleDevice::deinit(true)", "libération complète de la pile BLE"),
):
    check(token in BT_CPP, label)

# Aucun appel direct à l'ancienne pile Bluedroid ne doit subsister dans le code actif.
legacy_patterns = (
    r"#\s*include\s*[<\"](?:BLEDevice|BLEServer|BLEUtils|BLEHIDDevice|BLE2902|BLESecurity)\.h",
    r"\besp_bt_main\.h\b",
    r"\besp_gap_ble_api\.h\b",
    r"\besp_bluedroid_\w+",
    r"\besp_bt_controller_\w+",
    r"\besp_ble_gap_\w+",
    r"(?<!Nim)\bBLEDevice::",
    r"\bBLESecurity\b",
)
for pattern in legacy_patterns:
    check(re.search(pattern, active) is None,
          f"aucun appel direct Bluedroid : {pattern}")

print("Vérification structurelle BLE terminée. Tests RF/HID réels requis sur Cardputer ADV.")
