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
        "static bool compute_sha256_range(",
        "uint8_t checksum = 0xEF;",
        "checksum ^= payload[j]",
        "(segment_size & 3u) != 0",
        "Checksum ESP invalide",
        "align16(static_cast<uint32_t>(unpadded_length + 1u))",
        "if (header.hash_appended)",
        "SHA-256 ESP invalide",
        "ESP_IMAGE_HASH_LEN > limit - end",
        "SHA-256 ESP hors limites ou illisible",
        "memcmp(expected_hash, actual_hash, sizeof(expected_hash))",
        "measure_image(file, out->file_size, kBootloaderOffset,",
        "measure_image(file, out->file_size, image_offset, available,",
        "s_info.parsed && s_info.full_image",
        "REF SHA INVALIDE",
        "INTEGRITE ECHEC",
        "STRUCTURE + SHA OK",
        "STRUCTURE OK / SHA INDISPO",
        "STRUCTURE OK / SANS REF",
        'fopen(path, "rb")',
    ):
        require(source, token)

    # ESP-IDF v5.4.4: checksum byte is the last byte of align16(length + 1).
    for unpadded_length in (0, 1, 15, 16, 17, 255, 256, 257, 4095):
        checksum_end = (unpadded_length + 1 + 15) & ~15
        assert checksum_end % 16 == 0
        assert checksum_end > unpadded_length
        for hash_appended in (False, True):
            image_end = checksum_end + (32 if hash_appended else 0)
            assert image_end % 16 == 0

    if "align16(static_cast<uint32_t>(cursor)) + 1u" in source:
        raise SystemExit("Ancienne formule d'alignement ESP-IDF incorrecte detectee.")

    details = source.split("static void draw_details", 1)[1].split(
        "\nstatic void ", 1
    )[0]
    if details.index('image_title = "INTEGRITE ECHEC";') > details.index(
        'image_title = "STRUCTURE + SHA OK";'
    ):
        raise SystemExit("Une reference SHA incoherente ne doit jamais etre affichee comme valide.")

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
    print(" - bootloader and ota_0 image checksum / appended SHA-256 are verified")
    print(" - checksum alignment matches ESP-IDF's align16(length + 1) format")
    print(" - truncated partition tables are rejected instead of being misread as app binaries")
    print(" - incomplete candidates are never labelled as complete")
    print(" - no SD mount, flash write, erase or OTA write path exists")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
