#!/usr/bin/env python3
"""Assemble ESP32-S3 flash regions into one Cardputer ADV image."""
from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path

FLASH_SIZE = 8 * 1024 * 1024
BOOTLOADER_OFFSET = 0x0000
PARTITION_TABLE_OFFSET = 0x8000
PARTITION_TABLE_SIZE = 0x1000
APP_OFFSET = 0x10000


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write_checksum_manifest(path: Path, files: list[Path]) -> None:
    """Write a self-contained SHA-256 manifest for one downloadable artifact."""
    path.write_text(
        "".join(f"{sha256(file)}  {file.name}\\n" for file in files),
        encoding="utf-8",
    )

def check_partition_table(data: bytes) -> int:
    factory_size = 0
    found_factory = False
    for offset in range(0, len(data) - 31, 32):
        magic = struct.unpack_from("<H", data, offset)[0]
        if magic == 0xEBEB:  # partition-table MD5 marker
            break
        if magic != 0x50AA:
            if data[offset:offset + 32] == b"\xff" * 32:
                break
            raise SystemExit(f"Invalid partition-table entry at 0x{offset:x}")
        _magic, ptype, subtype, address, size, _label, _flags = struct.unpack_from(
            "<HBBII16sI", data, offset
        )
        if ptype == 0x00 and subtype == 0x00:  # factory app
            if address != APP_OFFSET:
                raise SystemExit(
                    f"Factory app offset is 0x{address:x}, expected 0x{APP_OFFSET:x}"
                )
            factory_size = size
            found_factory = True
    if not found_factory:
        raise SystemExit("No factory app partition found in partitions.bin")
    return factory_size


EXPECTED_PARTITIONS = (
    (0x01, 0x02, 0x9000, 0x5000, "nvs"),
    (0x01, 0x01, 0xE000, 0x1000, "phy_init"),
    (0x00, 0x00, 0x10000, 0x7E0000, "factory"),
)
ESP32S3_CHIP_ID = 9


def validate_partition_table(data: bytes) -> int:
    """Require the exact known ROZA Cardputer ADV partition layout."""
    if not data or len(data) > PARTITION_TABLE_SIZE or len(data) % 32:
        raise SystemExit("Invalid partition-table length")
    entries = []
    for offset in range(0, len(data) - 31, 32):
        magic = struct.unpack_from("<H", data, offset)[0]
        if magic == 0xEBEB:  # ESP-IDF partition-table MD5 marker
            break
        if data[offset:offset + 32] == b"\xff" * 32:
            break
        if magic != 0x50AA:
            raise SystemExit(f"Invalid partition-table entry at 0x{offset:x}")
        _magic, ptype, subtype, address, size, raw_label, _flags = struct.unpack_from(
            "<HBBII16sI", data, offset
        )
        label = raw_label.split(b"\0", 1)[0].decode("ascii", errors="strict")
        if size == 0 or address + size > FLASH_SIZE:
            raise SystemExit(f"Invalid partition bounds for {label or 'unnamed'}")
        entries.append((ptype, subtype, address, size, label))

    if tuple(entries) != EXPECTED_PARTITIONS:
        raise SystemExit(
            "Unsupported partition table: expected the exact ROZA Cardputer ADV "
            "NVS/PHY/factory layout"
        )
    ranges = sorted((address, address + size, label)
                    for _ptype, _subtype, address, size, label in entries)
    for previous, current in zip(ranges, ranges[1:]):
        if previous[1] > current[0]:
            raise SystemExit(f"Partitions overlap: {previous[2]} and {current[2]}")
    return EXPECTED_PARTITIONS[-1][3]


def validate_esp32s3_image(data: bytes) -> None:
    """Reject truncated, malformed or non-ESP32-S3 application images."""
    if len(data) < 24 or data[0] != 0xE9 or not 1 <= data[1] <= 16:
        raise SystemExit("Invalid or truncated ESP firmware image header")
    chip_id = struct.unpack_from("<H", data, 12)[0]
    if chip_id != ESP32S3_CHIP_ID:
        raise SystemExit(
            f"Wrong firmware target: chip_id={chip_id}, expected ESP32-S3 (9)"
        )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()

    build, output = args.build_dir, args.output_dir
    files = {
        "bootloader": build / "bootloader.bin",
        "partitions": build / "partitions.bin",
        "firmware": build / "firmware.bin",
    }
    for name, path in files.items():
        if not path.is_file() or path.stat().st_size == 0:
            raise SystemExit(f"Missing or empty {name}: {path}")

    boot_data = files["bootloader"].read_bytes()
    partition_data = files["partitions"].read_bytes()
    if len(boot_data) > PARTITION_TABLE_OFFSET:
        raise SystemExit("Bootloader overlaps the partition-table region")
    if len(partition_data) > PARTITION_TABLE_SIZE:
        raise SystemExit("Partition table exceeds its reserved 4 KiB region")
    if PARTITION_TABLE_OFFSET + len(partition_data) > APP_OFFSET:
        raise SystemExit("Partition table overlaps the factory application")
    app_capacity = validate_partition_table(partition_data)
    app_data = files["firmware"].read_bytes()
    validate_esp32s3_image(app_data)
    if len(app_data) > app_capacity:
        raise SystemExit(
            f"Firmware ({len(app_data)} bytes) exceeds factory partition ({app_capacity} bytes)"
        )
    if APP_OFFSET + len(app_data) > FLASH_SIZE:
        raise SystemExit("Firmware image exceeds 8 MiB flash")

    output.mkdir(parents=True, exist_ok=True)
    image_path = output / "roza-cardputer-adv-8mb.bin"
    image_end = max(
        BOOTLOADER_OFFSET + len(boot_data),
        PARTITION_TABLE_OFFSET + len(partition_data),
        APP_OFFSET + len(app_data),
    )
    image = bytearray(b"\xff" * image_end)
    image[BOOTLOADER_OFFSET:BOOTLOADER_OFFSET + len(boot_data)] = boot_data
    image[PARTITION_TABLE_OFFSET:PARTITION_TABLE_OFFSET + len(partition_data)] = partition_data
    image[APP_OFFSET:APP_OFFSET + len(app_data)] = app_data
    image_path.write_bytes(image)

    # App-only update: exact payload for the factory slot. Flash at APP_OFFSET,
    # without chip erase, and only over ROZA with this same partition layout.
    update_path = output / "ROZA_UPDATE.bin"
    update_path.write_bytes(app_data)
    if image_path.read_bytes() != image or update_path.read_bytes() != app_data:
        raise SystemExit("Package read-back verification failed")

    checksum = sha256(image_path)
    update_checksum = sha256(update_path)
    partition_checksum = hashlib.sha256(partition_data).hexdigest()
    write_checksum_manifest(output / "SHA256SUMS-flash.txt", [image_path])
    write_checksum_manifest(output / "SHA256SUMS-update.txt", [update_path])
    (output / "UPDATE-INFO.txt").write_text(
        "ROZA — M5Stack Cardputer ADV application-only update\n"
        "Target: ESP32-S3FN8 / 8 MiB flash / no PSRAM\n"
        "File: ROZA_UPDATE.bin (application payload only)\n"
        "Flash address: 0x10000\n"
        "Operation: write application partition only; DO NOT erase the chip\n"
        "Required: existing ROZA installation with the same factory partition layout\n"
        f"Required partition table SHA-256: {partition_checksum}\n"
        f"Application SHA-256: {update_checksum}\n"
        "Preflight: ESP32-S3 image header and exact partition layout validated.\n"
        f"Factory app partition: offset 0x{APP_OFFSET:05x}, size 0x{app_capacity:x} ({app_capacity} bytes)\n"
        "NVS/settings and microSD are outside this file and are not intentionally erased.\n"
        "Do not use on M5Launcher, another firmware, or a changed partition layout.\n"
        "If unsure, use the full image and expect settings to reset.\n",
        encoding="utf-8",
    )
    (output / "FLASH-INFO.txt").write_text(
        "ROZA — M5Stack Cardputer ADV\n"
        "Chip: ESP32-S3FN8 / 8 MiB flash / no PSRAM\n"
        "Image: roza-cardputer-adv-8mb.bin\n"
        "Flash address: 0x0000 (single merged image)\n"
        "Included regions: bootloader 0x0000, partition table 0x8000, app 0x10000\n"
        "NVS at 0x9000 is erased by this full merged image; ROZA preferences reset on each flash.\n"
        "microSD contents are not modified.\n"
        f"App image: {len(app_data)} bytes / partition capacity: {app_capacity} bytes\n"
        f"Combined image: {len(image)} bytes\n"
        "Use only on M5Stack Cardputer ADV.\n",
        encoding="utf-8",
    )
    print(f"Flash image: {image_path} ({len(image)} bytes)")
    print(f"SHA-256: {checksum}")
    print(f"Factory app partition: {app_capacity} bytes; app image: {len(app_data)} bytes")


if __name__ == "__main__":
    main()
