#!/usr/bin/env python3
"""Fail-closed checks for the pinned ROZA Cardputer ADV fusion contract.

This is a source/configuration gate, not a hardware test or a release-image validator.
"""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
NUCLEO = ROOT / "nucleoos"
PINNED_LAUNCHER = "4b06de691df15b0fe6a9f09f56d1d107bbf32b12"
PINNED_NUCLEOOS = "cb0f1a5356ddd6f96ec6b31bbcba5d55f52d8123"
AUTHORIZED_BRANCH = "migration/roza-chanina-launcher"
errors: list[str] = []


def read(rel: str) -> str:
    path = NUCLEO / rel if rel.startswith("firmware/") else ROOT / rel
    if not path.is_file():
        errors.append(f"missing file: {rel}")
        return ""
    return path.read_text(encoding="utf-8")


def require(rel: str, *tokens: str) -> None:
    source = read(rel)
    for token in tokens:
        if token not in source:
            errors.append(f"{rel}: required contract missing: {token}")


def forbid(rel: str, *tokens: str) -> None:
    source = read(rel)
    for token in tokens:
        if token in source:
            errors.append(f"{rel}: forbidden duplicate owner/source: {token}")


def git_head(path: Path) -> str:
    result = subprocess.run(
        ["git", "-C", str(path), "rev-parse", "HEAD"],
        check=False, capture_output=True, text=True,
    )
    if result.returncode:
        errors.append(f"cannot read pinned Git revision in {path}")
        return ""
    return result.stdout.strip()


def check_partitions() -> None:
    rel = "integration/nucleoos-bridge/partitions.csv"
    source = read(rel)
    rows: dict[str, tuple[str, str, int, int]] = {}
    for raw in source.splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line or line.lower().startswith("name,"):
            continue
        cols = [part.strip() for part in line.split(",")]
        if len(cols) < 5:
            errors.append(f"{rel}: malformed row: {raw}")
            continue
        try:
            rows[cols[0]] = (cols[1], cols[2], int(cols[3], 0), int(cols[4], 0))
        except ValueError:
            errors.append(f"{rel}: invalid numeric partition field: {raw}")

    expected = {
        "ota_0": ("app", "ota_0", 0x020000, 0x380000),
        "ota_1": ("app", "ota_1", 0x3A0000, 0x380000),
        "nucleo_nap": ("data", "littlefs", 0x720000, 0x040000),
        "cfg": ("data", "spiffs", 0x760000, 0x0A0000),
    }
    for label, value in expected.items():
        if rows.get(label) != value:
            errors.append(f"{rel}: {label} must be {value}, got {rows.get(label)}")
    if "factory" in rows:
        errors.append(f"{rel}: unexpected factory partition in the dual-OTA layout")
    if max((off + size for _, _, off, size in rows.values()), default=0) != 0x800000:
        errors.append(f"{rel}: partition map must end exactly at 8 MiB (0x800000)")


def main() -> int:
    branch = os.environ.get("GITHUB_REF_NAME", "")
    if branch and branch != AUTHORIZED_BRANCH:
        errors.append(f"workflow ran on unauthorized branch: {branch}")

    if git_head(ROOT / "launcher") != PINNED_LAUNCHER:
        errors.append("Launcher submodule is not at the approved pinned commit")
    if git_head(NUCLEO) != PINNED_NUCLEOOS:
        errors.append("NucleoOS submodule is not at the approved pinned commit")

    # One NucleoOS-owned hardware stack and one native application registry.
    require(
        "integration/nucleoos-bridge/main/CMakeLists.txt",
        '"${NUCLEOOS_MAIN_DIR}/main.c"',
        "nucleo_app",
        "nucleo_storage",
        "nucleo_sdcontent",
    )
    forbid(
        "integration/nucleoos-bridge/main/CMakeLists.txt",
        "src/sd_functions.cpp",
        "src/onlineLauncher.cpp",
        "setupSdCard(",
    )
    require(
        "firmware/components/nucleo_app/CMakeLists.txt",
        '"app_files.cpp"', '"app_store.cpp"', '"app_updates.cpp"',
    )
    require(
        "firmware/components/nucleo_app/nucleo_app.cpp",
        "nucleo_register_files();", "nucleo_register_store();", "nucleo_register_updates();",
    )
    require(
        "scripts/apply-roza-nucleoos.py",
        "nucleo_app_register_builtins();",
        "roza_register_launcher_partitions();",
        "roza_register_launcher_sd();",
        "nucleo_store_apps_sync();",
    )

    require(
        "firmware/components/nucleo_storage/nucleo_i18n.c",
        'static char s_lang[6] = "fr";',
        "norm_lang(s_lang, sizeof s_lang, lg->valuestring)",
    )

    # The Store is the one app-package system: signed catalog/packages, hash checks and atomic SD swap.
    require(
        "firmware/components/nucleo_store/store_engine.c",
        "st_install(", "sdc_sha256_update", "rename(tmp, live)", "ST_E_VERIFY",
    )
    require(
        "firmware/components/nucleo_store/store_verify.c",
        "MBEDTLS_ECP_DP_SECP256R1", "mbedtls_ecdsa_read_signature",
    )
    require(
        "firmware/components/nucleo_store/nucleo_store.c",
        "nucleo_storage.h", "nucleo_sdcontent_http_get", "st_verify",
    )

    # The upstream OTA engine exists, but its NucleoOS release feed must stay blocked in ROZA.
    # Only a future ROZA-owned, signed release channel may re-enable this path.
    require(
        "firmware/components/nucleo_app/nucleo_update.c",
        "static void fail_install(const char *msg, uint32_t tk) __attribute__((noreturn));",
        "esp_ota_write(", "esp_ota_set_boot_partition(", "mbedtls_sha256_update", "fwsig.h",
    )
    require(
        "scripts/apply-roza-nucleoos.py",
        "def verify_roza_update_gate()",
        '"https://updates.roza.invalid/"',
        "NucleoOS release tags are not ROZA release tags",
        "goto out_silent;",
        'nvs_erase_key(s_nvs, "install")',
        "verify_roza_update_gate()",
        "ROZA update channel pending",
        "ROZA update channel not configured",
    )

    # Hardware map and storage mount must remain the Cardputer ADV wiring / shared SD owner.
    require(
        "firmware/components/nucleo_board/include/nucleo_board.h",
        '#define NUCLEO_SD_MOUNT      "/sd"',
        "#define NUCLEO_SD_PIN_SCLK   40",
        "#define NUCLEO_SD_PIN_MISO   39",
        "#define NUCLEO_SD_PIN_MOSI   14",
        "#define NUCLEO_SD_PIN_CS     12",
    )
    require("firmware/components/nucleo_storage/nucleo_storage.c", "esp_vfs_fat_sdspi_mount", "install_write_retry();")
    require(
        "integration/nucleoos-bridge/components/roza_launcher_sd/roza_launcher_sd.cpp",
        "mbedtls_sha256_starts", "mbedtls_sha256_update",
        '"%s.sha256"', "sha_reference_valid", "SHA %.16s %s",
    )
    require(
        "integration/nucleoos-bridge/components/roza_launcher_sd/CMakeLists.txt",
        "mbedtls",
    )
    forbid(
        "integration/nucleoos-bridge/components/roza_launcher_sd/roza_launcher_sd.cpp",
        "esp_partition_erase_range(", "esp_ota_write(", 'fopen(sidecar_path, "wb")',
    )

    check_partitions()

    if errors:
        print("ROZA fusion contract: FAILED")
        for error in errors:
            print(f" - {error}")
        return 1

    print("ROZA fusion contract: PASS")
    print(" - approved Launcher and NucleoOS revisions")
    print(" - native Files and signed Store remain registered; firmware OTA stays safely blocked")
    print(" - French is the first-boot language; saved language is preserved")
    print(" - one NucleoOS-owned SD/storage stack; no Launcher second SD mount")
    print(" - Cardputer ADV pin map and 8 MiB dual-OTA partition layout")
    print("This gate does not validate runtime behavior or authorize flashing.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
