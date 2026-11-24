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
CRYPTO_CPP = (ROOT / "src" / "crypto_engines.cpp").read_text(encoding="utf-8")
BLE_CONFIG = (ROOT / "src" / "roza_nimble_config.h").read_text(encoding="utf-8")
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
check("#include <HIDTypes.h>" in BT_CPP,
      "les macros HID officielles restent incluses après le remplacement de Bluedroid")
check("class RozaBleAdvertising" in COMPAT and "NimBLEExtAdvertising" in COMPAT,
      "la publicité BLE legacy/étendue est présente")


check("-include src/roza_nimble_config.h" in PLATFORM,
      "la configuration d'événements HCI est injectée dans toutes les unités compilées")
check("#include <sdkconfig.h>" in BLE_CONFIG and
      "#define CONFIG_BT_NIMBLE_TRANSPORT_EVT_SIZE 257" in BLE_CONFIG,
      "la taille des événements HCI est alignée à 257 octets pour BLE étendu")
check("#include <mbedtls/version.h>" in CRYPTO_CPP and
      "#if MBEDTLS_VERSION_NUMBER >= 0x03000000" in CRYPTO_CPP,
      "la couche crypto détecte Mbed TLS 3")
for old_name, new_name in (
    ("mbedtls_md5_starts_ret", "mbedtls_md5_starts"),
    ("mbedtls_md5_update_ret", "mbedtls_md5_update"),
    ("mbedtls_md5_finish_ret", "mbedtls_md5_finish"),
    ("mbedtls_sha256_starts_ret", "mbedtls_sha256_starts"),
    ("mbedtls_sha256_update_ret", "mbedtls_sha256_update"),
    ("mbedtls_sha256_finish_ret", "mbedtls_sha256_finish"),
):
    check(f"#define {old_name} {new_name}" in CRYPTO_CPP,
          f"compatibilité API Mbed TLS : {old_name} → {new_name}")
check("static bool derive128Key(" in CRYPTO_CPP and "if (!derive128Key(key, key128)) return false;" in CRYPTO_CPP,
      "les dérivations AES vérifient les erreurs MD5")
check("md5_crypt_error:" in CRYPTO_CPP and "ROZA_MD5_CHECK" in CRYPTO_CPP,
      "MD5-crypt vérifie les retours et libère les contextes en cas d'erreur")

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
