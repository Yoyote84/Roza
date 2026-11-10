#!/usr/bin/env python3
"""Create a reproducible manifest for the ROZA Cardputer ADV device-test candidate."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path

PINNED = {
    "launcher": "4b06de691df15b0fe6a9f09f56d1d107bbf32b12",
    "nucleoos": "cb0f1a5356ddd6f96ec6b31bbcba5d55f52d8123",
}
FLASH_BYTES = 8 * 1024 * 1024


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def git_head(path: Path) -> str:
    p = subprocess.run(["git", "-C", str(path), "rev-parse", "HEAD"],
                       capture_output=True, text=True)
    if p.returncode:
        raise SystemExit("Impossible de vérifier la révision de " + str(path))
    return p.stdout.strip()


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--build-dir", type=Path, required=True)
    ap.add_argument("--partition-csv", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    args = ap.parse_args()
    root = Path.cwd().resolve()
    build = args.build_dir.resolve()
    files = {
        "firmware_candidate": build / "ROZA-Cardputer-ADV-FULL.bin",
        "merged_image_audit": build / "ROZA-ESP32S3-FLASH-PREVIEW.bin",
        "application": build / "roza_nucleoos_launcher_bridge.bin",
        "elf": build / "roza_nucleoos_launcher_bridge.elf",
        "bootloader": build / "bootloader/bootloader.bin",
        "partition_table": build / "partition_table/partition-table.bin",
        "ota_data_initial": build / "ota_data_initial.bin",
        "flash_args": build / "flash_args",
        "partition_csv": args.partition_csv.resolve(),
        "installer": root / "tools/installer/flash.sh",
        "install_guide": root / "docs/INSTALL-ROZA-FR.md",
        "launcher_license": root / "launcher/LICENSE",
        "nucleoos_license": root / "nucleoos/LICENSE",
        "radiolib_license": root / "third_party/licenses/RadioLib-7.8.0-MIT.txt",
    }
    records = {}
    for name, path in files.items():
        if not path.is_file():
            raise SystemExit("Artefact manquant : " + str(path))
        records[name] = {
            "path": path.relative_to(root).as_posix(),
            "size_bytes": path.stat().st_size,
            "sha256": digest(path),
        }

    rows = []
    for raw in files["partition_csv"].read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line or line.lower().startswith("name,"):
            continue
        cols = [x.strip() for x in line.split(",")]
        if len(cols) < 5:
            raise SystemExit("Ligne de partition invalide : " + raw)
        try:
            offset, size = int(cols[3], 0), int(cols[4], 0)
        except ValueError:
            raise SystemExit("Offset/taille invalide : " + raw)
        if offset < 0 or size <= 0 or offset + size > FLASH_BYTES:
            raise SystemExit("Partition hors des 8 Mio : " + raw)
        rows.append({"name": cols[0], "type": cols[1], "subtype": cols[2],
                     "offset": offset, "size": size, "end": offset + size})
    ordered = sorted(rows, key=lambda x: x["offset"])
    for a, b in zip(ordered, ordered[1:]):
        if a["end"] > b["offset"]:
            raise SystemExit("Chevauchement de partitions : " + a["name"] + "/" + b["name"])
    by_name = {x["name"]: x for x in ordered}
    required_names = {"launcher_nvs_old", "otadata", "phy_init", "nvs", "nvs_keys",
                      "ota_0", "ota_1", "nucleo_nap", "cfg"}
    if set(by_name) != required_names:
        raise SystemExit("Labels de partitions ROZA inattendus.")
    checks = {
        "launcher_nvs_old": (0x9000, 0x5000),
        "otadata": (0xE000, 0x2000),
        "phy_init": (0x11000, 0x1000),
        "nvs": (0x13000, 0x6000),
        "nvs_keys": (0x19000, 0x1000),
        "ota_0": (0x20000, 0x380000),
        "ota_1": (0x3A0000, 0x380000),
    }
    for name, expected in checks.items():
        if (by_name[name]["offset"], by_name[name]["size"]) != expected:
            raise SystemExit(f"Partition {name} inattendue.")
    if max(x["end"] for x in ordered) != FLASH_BYTES:
        raise SystemExit("La table ne couvre pas exactement les 8 Mio.")
    for name in PINNED:
        actual = git_head(root / name)
        if actual != PINNED[name]:
            raise SystemExit(f"Révision {name} inattendue : {actual}")

    capacity = min(by_name["ota_0"]["size"], by_name["ota_1"]["size"])
    used = records["application"]["size_bytes"]
    if used > capacity:
        raise SystemExit(f"Image ({used}) trop grande pour un slot ({capacity}).")
    if records["firmware_candidate"]["sha256"] != records["merged_image_audit"]["sha256"]:
        raise SystemExit("L'image candidate diffère du binaire fusionné vérifié.")

    manifest_data = {
        "schema": "roza-device-test-manifest/v1",
        "product": "РO3A / ROZA",
        "build_scope": "Firmware unique NucleoOS avec surcouche ROZA française, applications natives, GNSS et SX1262 RX-only. Les fonctions Launcher redondantes sont assurées par les services natifs NucleoOS; aucun second setup/loop Arduino ni montage SD parallèle.",
        "hardware": {"board": "M5Stack Cardputer ADV", "chip": "ESP32-S3FN8",
                     "flash_bytes": FLASH_BYTES, "psram": False, "display": "ST7789 240x135",
                     "keyboard": "TCA8418", "sd": "SDSPI", "cap": "M5Stack Cap LoRa-1262"},
        "source": {"roza_commit": os.getenv("GITHUB_SHA", "local"),
                   "roza_ref": os.getenv("GITHUB_REF", "local"),
                   "launcher_commit": git_head(root / "launcher"),
                   "nucleoos_commit": git_head(root / "nucleoos"),
                   "radiolib": "7.8.0", "esp_idf": "v5.4.4"},
        "partition_layout": {
            "rows": ordered, "slot_capacity_bytes": capacity,
            "application_bytes": used, "free_bytes": capacity-used,
            "free_percent": round((capacity-used)*100/capacity, 2),
            "legacy_launcher_nvs_reserved": "0x9000-0xDFFF",
            "new_nucleoos_nvs": "0x13000-0x18FFF",
            "otadata": "0xE000-0xFFFF",
            "otadata_policy": "initialized by image to select ota_0; previous slot selection is not retained",
            "preflash_erase_ranges": ["0x11000-0x11FFF", "0x13000-0x18FFF", "0x19000-0x19FFF"],
            "legacy_nvs_is_not_mounted": True,
            "legacy_nvs_backup_restored_after_merged_image_write": True,
            "legacy_nvs_restore_verified_by_readback": True,
            "partition_csv": records["partition_csv"],
        },
        "flash_image": {
            "artifact": records["firmware_candidate"],
            "format": "ESP-IDF esptool merged raw binary",
            "chip": "esp32s3", "write_offset": "0x0",
            "flash_capacity_bytes": FLASH_BYTES,
            "flash_image_is_padded_to_full_flash": False,
            "flash_args": records["flash_args"],
            "components": [
                {"name": "bootloader", "offset": "0x0"},
                {"name": "partition_table", "offset": "0x8000"},
                {"name": "otadata_initial", "offset": "0xE000"},
                {"name": "application", "offset": "0x20000"},
            ],
            "sha256": records["firmware_candidate"]["sha256"],
        },
        "feature_matrix": {
            "single_native_runtime": True,
            "nucleoos_hardware_ownership": ["display", "keyboard", "i2c", "sd", "usb"],
            "native_capabilities": [
                "Files / microSD", "Store avec verification des signatures",
                "Wi-Fi manuel", "date/heure manuelle et fuseau Europe/Paris",
                "WebUI", "USB MSC", "jeux et utilitaires NucleoOS",
                "Partitions en lecture seule", "preflight Firmware SD",
                "Diagnostic ROZA", "GNSS NMEA", "SX1262 reception LoRa"
            ],
            "launcher_policy": "Les fonctions qui se recouvrent sont fournies par les services natifs NucleoOS; pas de second menu, setup(), loop(), montage SD ou installateur de paquets Launcher parallèle.",
            "firmware_ota": "disabled until a ROZA-owned signed release channel and rollback policy are published",
            "roza_signed_store_catalog": "not configured; no private signing key is committed",
            "battery_percent": "voltage-based estimate, not a coulomb-counting fuel-gauge measurement",
            "micro_sd_filesystem": "FAT32; exFAT is not enabled in this build",
            "lora": {"mode": "receive-only", "frequency_mhz": 868.0,
                     "antenna_switch": "PI4IOE5V6408 P0 on shared ADV I2C bus"},
        },
        "artifacts": records,
        "release": {
            "class": "complete-software-device-test-candidate",
            "firmware_image_generated": True,
            "release_ready": False,
            "safe_to_flash": False,
            "hardware_tested": False,
            "blockers": [
                "premier demarrage et peripheriques doivent etre essayes sur un Cardputer ADV reel",
                "canal de firmware ROZA signe non publie; OTA firmware reste desactive",
                "catalogue de paquets signe propre a ROZA non configure; le Store natif et sa verification restent disponibles",
            ],
            "installation": {
                "type": "initial USB serial flash, not OTA",
                "legacy_nvs_backup_required": True,
                "legacy_nvs_restored_exactly_after_merged_image_write": True,
                "legacy_nvs_restored_after_contiguous_merge_bin_write": True,
                "legacy_nvs_readback_compared_byte_for_byte": True,
                "new_nvs_initialized_empty": True,
                "otadata_reset": True,
                "whole_flash_erase": False,
                "erase_regions": ["0x11000-0x11FFF", "0x13000-0x18FFF", "0x19000-0x19FFF"],
                "micro_sd_contents_erased": False,
            },
        },
    }
    out = args.output.resolve()
    out.write_text(json.dumps(manifest_data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({
        "output": str(out),
        "application_bytes": used,
        "slot_bytes": capacity,
        "free_bytes": capacity-used,
        "free_percent": manifest_data["partition_layout"]["free_percent"],
        "firmware_candidate_bytes": records["firmware_candidate"]["size_bytes"],
        "firmware_candidate_sha256": records["firmware_candidate"]["sha256"],
        "device_test_candidate": True,
        "safe_to_flash": False,
    }, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
