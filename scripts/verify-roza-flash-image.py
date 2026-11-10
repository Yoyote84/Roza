#!/usr/bin/env python3
"""Fail-closed validation of the complete ROZA Cardputer ADV first-install image."""
from __future__ import annotations

import argparse
import hashlib
import json
import shlex
from pathlib import Path

FLASH_BYTES = 8 * 1024 * 1024
FIXED_FILES = {
    "bootloader/bootloader.bin": 0x000000,
    "partition_table/partition-table.bin": 0x008000,
}
EXPECTED_LAYOUT = {
    "launcher_nvs_old": ("data", "0x40", 0x009000, 0x005000),
    "otadata": ("data", "ota", 0x00E000, 0x002000),
    "phy_init": ("data", "phy", 0x011000, 0x001000),
    "nvs": ("data", "nvs", 0x013000, 0x006000),
    "nvs_keys": ("data", "nvs_keys", 0x019000, 0x001000),
    "ota_0": ("app", "ota_0", 0x020000, 0x380000),
    "ota_1": ("app", "ota_1", 0x3A0000, 0x380000),
    "nucleo_nap": ("data", "littlefs", 0x720000, 0x040000),
    "cfg": ("data", "spiffs", 0x760000, 0x0A0000),
}


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def fail(message: str) -> None:
    raise SystemExit("Validation image ROZA : ECHEC — " + message)


def resolve_flash_file(build: Path, raw_path: str) -> Path:
    candidate = Path(raw_path)
    attempts = [candidate] if candidate.is_absolute() else [build / candidate]
    if not candidate.is_absolute() and candidate.parts and candidate.parts[0] == build.name:
        attempts.append(build.parent / candidate)
    for attempt in attempts:
        resolved = attempt.resolve()
        try:
            resolved.relative_to(build)
        except ValueError:
            continue
        if resolved.is_file():
            return resolved
    fail("fichier référence par flash_args absent ou hors du build : " + raw_path)


def parse_flash_args(path: Path, build: Path) -> tuple[dict[str, tuple[int, Path]], str]:
    tokens = shlex.split(path.read_text(encoding="utf-8"))
    entries: dict[str, tuple[int, Path]] = {}
    flash_size = ""
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token.startswith("--"):
            option = token.replace("_", "-").lower()
            if index + 1 < len(tokens) and not tokens[index + 1].startswith("--"):
                value = tokens[index + 1]
                if option == "--flash-size":
                    flash_size = value
                index += 2
            else:
                index += 1
            continue
        try:
            offset = int(token, 0)
        except ValueError:
            fail("argument inattendu dans flash_args : " + token)
        if index + 1 >= len(tokens):
            fail("offset sans fichier dans flash_args : " + token)
        file_path = resolve_flash_file(build, tokens[index + 1])
        relative = file_path.relative_to(build).as_posix()
        if relative in entries:
            fail("fichier flash dupliqué : " + relative)
        entries[relative] = (offset, file_path)
        index += 2
    return entries, flash_size


def read_partitions(path: Path) -> dict[str, tuple[str, str, int, int]]:
    rows: dict[str, tuple[str, str, int, int]] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line or line.lower().startswith("name,"):
            continue
        cols = [item.strip() for item in line.split(",")]
        if len(cols) < 5:
            fail("ligne de partition invalide : " + raw)
        try:
            if cols[0] in rows:
                fail("partition dupliquée : " + cols[0])
            rows[cols[0]] = (cols[1], cols[2], int(cols[3], 0), int(cols[4], 0))
        except ValueError:
            fail("offset/taille de partition invalide : " + raw)
    return rows


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--image", type=Path, required=True)
    parser.add_argument("--flash-args", type=Path, required=True)
    parser.add_argument("--partition-csv", type=Path, required=True)
    args = parser.parse_args()

    build = args.build_dir.resolve()
    image_path = (build / args.image).resolve()
    if not image_path.is_file():
        fail("image fusionnée introuvable : " + str(image_path))
    if image_path.stat().st_size <= 0 or image_path.stat().st_size > FLASH_BYTES:
        fail("taille de l'image hors limite 8 Mio")
    flash_args = args.flash_args.resolve()
    if not flash_args.is_file():
        fail("flash_args absent")

    rows = read_partitions(args.partition_csv.resolve())
    if set(rows) != set(EXPECTED_LAYOUT):
        fail("labels de partitions inattendus : " + repr(sorted(rows)))
    for name, expected in EXPECTED_LAYOUT.items():
        if rows[name] != expected:
            fail(f"{name}: {rows[name]}, attendu {expected}")
    ranges = sorted((offset, offset + size, name)
                    for name, (_, _, offset, size) in rows.items())
    for previous, current in zip(ranges, ranges[1:]):
        if current[0] < previous[1]:
            fail(f"partitions superposées : {previous[2]} / {current[2]}")
    if ranges[-1][1] != FLASH_BYTES:
        fail("la table ne couvre pas exactement les 8 Mio")
    for name, (_, _, offset, size) in rows.items():
        if offset % 0x1000 or size <= 0 or size % 0x1000 or offset + size > FLASH_BYTES:
            fail(f"alignement/limite invalide pour {name}")
        if name.startswith("ota_") and (offset % 0x10000 or size % 0x10000):
            fail(f"alignement OTA invalide pour {name}")

    expected_files = dict(FIXED_FILES)
    expected_files["ota_data_initial.bin"] = rows["otadata"][2]
    expected_files["roza_nucleoos_launcher_bridge.bin"] = rows["ota_0"][2]
    entries, flash_size = parse_flash_args(flash_args, build)
    if flash_size.upper() != "8MB":
        fail("flash_args ne cible pas explicitement 8MB : " + (flash_size or "absent"))
    if set(entries) != set(expected_files):
        fail("fichiers flash inattendus : " + repr(sorted(entries)))
    image_bytes = image_path.read_bytes()
    max_end = 0
    verified = []
    for relative, expected_offset in expected_files.items():
        actual_offset, source_path = entries[relative]
        if actual_offset != expected_offset:
            fail(f"{relative}: offset {actual_offset:#x}, attendu {expected_offset:#x}")
        payload = source_path.read_bytes()
        end = actual_offset + len(payload)
        if end > FLASH_BYTES:
            fail(relative + " dépasse la flash de 8 Mio")
        if relative == "roza_nucleoos_launcher_bridge.bin" and len(payload) > rows["ota_0"][3]:
            fail("l'application dépasse ota_0")
        if end > len(image_bytes) or image_bytes[actual_offset:end] != payload:
            fail("contenu fusionné différent de la source : " + relative)
        max_end = max(max_end, end)
        verified.append({
            "file": relative,
            "offset": f"0x{actual_offset:X}",
            "size_bytes": len(payload),
            "sha256": sha256(source_path),
        })

    if len(image_bytes) != max_end:
        fail(f"taille fusionnée {len(image_bytes)} différente de la fin réelle {max_end}")
    print(json.dumps({
        "status": "PASS",
        "chip": "esp32s3",
        "flash_bytes": FLASH_BYTES,
        "merged_image": image_path.name,
        "merged_size_bytes": len(image_bytes),
        "merged_sha256": sha256(image_path),
        "write_offset": "0x0",
        "verified_components": verified,
        "partition_contract": {
            "launcher_nvs_old_reserved": "0x9000-0xDFFF",
            "installer_restores_legacy_nvs_after_contiguous_merge_image": True,
            "installer_verifies_legacy_nvs_by_readback": True,
            "new_nucleoos_nvs": "0x13000-0x18FFF",
            "otadata": "0xE000-0xFFFF",
            "otadata_initialized_to": "ota_0",
            "preflash_erase_ranges": ["0x11000-0x11FFF", "0x13000-0x18FFF", "0x19000-0x19FFF"],
            "whole_flash_erase": False,
        },
        "device_test_candidate": True,
        "hardware_tested": False,
        "safe_to_flash": False,
    }, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
