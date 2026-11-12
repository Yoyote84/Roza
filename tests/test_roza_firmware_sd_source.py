#!/usr/bin/env python3
"""Source-level safety contract for the read-only ROZA Firmware SD inspector."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "integration/nucleoos-bridge/components/roza_launcher_sd/roza_launcher_sd.cpp"


def require(source: str, token: str) -> None:
    if token not in source:
        raise SystemExit(f"Contrat Firmware SD absent : {token}")


def main() -> int:
    source = SOURCE.read_text(encoding="utf-8")
    for number, line in enumerate(source.splitlines(), 1):
        if line.lstrip().startswith('#include "'):
            if line.count('"') != 2 or not line.rstrip().endswith('"'):
                raise SystemExit(f"Include C++ mal forme ligne {number}: {line}")

    for token in (
        '#include "esp_image_format.h"',
        "constexpr uint32_t kBootloaderOffset = ESP_BOOTLOADER_OFFSET;",
        "esp_partition_table_verify(table, false, &verified_count)",
        "roza_partition_layout_validate(entries, count)",
        "const bool has_table_magic",
        "Table de partitions incomplete",
        "measure_image(file, out->file_size, kBootloaderOffset,",
        "measure_image(file, out->file_size, image_offset, available,",
        "s_info.parsed && s_info.full_image",
        'fopen(path, "rb")',
    ):
        require(source, token)

    analyze = source.split("static bool analyze_firmware", 1)[1].split(
        "static void select_entry", 1
    )[0]
    if analyze.index("out->full_image = true;") < analyze.index(
        "if (!measure_image(file, out->file_size, image_offset, available,"
    ):
        raise SystemExit("Firmware SD marque une image complete avant la validation de l'application.")

    for forbidden in (
        'fopen(path, "wb")',
        "esp_flash_write(",
        "esp_flash_erase_region(",
        "esp_partition_erase_range(",
        "esp_ota_write(",
        "esp_ota_set_boot_partition(",
        "esp_vfs_fat_sdspi_mount(",
        "SD.begin(",
    ):
        if forbidden in source:
            raise SystemExit(f"Firmware SD ne doit pas ecrire ni initialiser le stockage: {forbidden}")

    print("ROZA Firmware SD read-only contract: PASS")
    print(" - quoted C++ includes are well formed")
    print(" - merged images require bootloader, exact partition table and ota_0 image framing")
    print(" - truncated partition tables are rejected instead of being misread as app binaries")
    print(" - incomplete candidates are never labelled as complete")
    print(" - no SD mount, flash write, erase or OTA write path exists")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
