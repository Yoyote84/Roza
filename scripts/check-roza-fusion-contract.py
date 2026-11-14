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
PINNED_DISPLAY_DRIVERS = "79d2dc757d0684a3f63ab027920edd152972f93f"
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
            label = cols[0]
            if label in rows:
                errors.append(f"{rel}: duplicate partition label: {label}")
                continue
            rows[label] = (cols[1], cols[2], int(cols[3], 0), int(cols[4], 0))
        except ValueError:
            errors.append(f"{rel}: invalid numeric partition field: {raw}")

    # Freeze the complete NucleoOS-owned 8 MiB layout. Checking only the OTA slots
    # allowed accidental movement of NVS/otadata/keys to pass CI, which could make
    # an otherwise valid image lose settings or fail to boot after an update.
    expected = {
        "launcher_nvs_old": ("data", "0x40", 0x009000, 0x005000),
        "nvs": ("data", "nvs", 0x013000, 0x006000),
        "otadata": ("data", "ota", 0x00E000, 0x002000),
        "phy_init": ("data", "phy", 0x011000, 0x001000),
        "nvs_keys": ("data", "nvs_keys", 0x019000, 0x001000),
        "ota_0": ("app", "ota_0", 0x020000, 0x380000),
        "ota_1": ("app", "ota_1", 0x3A0000, 0x380000),
        "nucleo_nap": ("data", "littlefs", 0x720000, 0x040000),
        "cfg": ("data", "spiffs", 0x760000, 0x0A0000),
    }
    if set(rows) != set(expected):
        errors.append(
            f"{rel}: partition labels must be exactly {sorted(expected)}, got {sorted(rows)}"
        )
    for label, value in expected.items():
        if rows.get(label) != value:
            errors.append(f"{rel}: {label} must be {value}, got {rows.get(label)}")

    # Independently validate geometry so future edits cannot hide overlaps,
    # zero-sized entries, unaligned app slots or an address beyond the 8 MiB chip.
    ranges: list[tuple[int, int, str]] = []
    for label, (kind, subtype, offset, size) in rows.items():
        if size <= 0:
            errors.append(f"{rel}: {label} has a non-positive size")
            continue
        end = offset + size
        if offset < 0x9000 or end > 0x800000:
            errors.append(
                f"{rel}: {label} range 0x{offset:x}..0x{end:x} is outside usable 8 MiB flash"
            )
        if offset % 0x1000 or size % 0x1000:
            errors.append(f"{rel}: {label} is not aligned to the 4 KiB flash sector")
        if kind == "app" and (offset % 0x10000 or size % 0x10000):
            errors.append(f"{rel}: app partition {label} is not 64 KiB aligned")
        ranges.append((offset, end, label))

    ranges.sort()
    for previous, current_range in zip(ranges, ranges[1:]):
        if current_range[0] < previous[1]:
            errors.append(
                f"{rel}: partitions {previous[2]} and {current_range[2]} overlap"
            )
    if max((end for _, end, _ in ranges), default=0) != 0x800000:
        errors.append(f"{rel}: partition map must end exactly at 8 MiB (0x800000)")


def main() -> int:
    branch = os.environ.get("GITHUB_REF_NAME", "")
    if branch and branch != AUTHORIZED_BRANCH:
        errors.append(f"workflow ran on unauthorized branch: {branch}")

    if git_head(ROOT / "launcher") != PINNED_LAUNCHER:
        errors.append("Launcher submodule is not at the approved pinned commit")
    if git_head(ROOT / "launcher/lib/DisplayDrivers") != PINNED_DISPLAY_DRIVERS:
        errors.append("DisplayDrivers nested submodule revision is not the approved M5GFX backend commit.")
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

    # Keep the requested offline date/time editor and French first-run timezone.
    require(
        "firmware/components/nucleo_setup/clock_zones.h",
        '#define CLOCK_ZONE_DEFAULT_ID "Europe/Paris"',
    )
    require(
        "firmware/components/nucleo_setup/clock_zones.c",
        '"Europe/Paris"',
        '"CET-1CEST,M3.5.0,M10.5.0/3"',
    )
    require(
        "firmware/components/nucleo_setup/nucleo_setup.c",
        "void nucleo_setup_set_datetime(int year, int mon, int day, int hour, int min)",
        "tm.tm_isdst = -1;",
        "time_t t = mktime(&tm);",
        "settimeofday(&tv, NULL);",
        'NUCLEO_SD_MOUNT "/system/time.json"',
    )
    require(
        "firmware/components/nucleo_app/app_wifi.cpp",
        "nucleo_setup_set_datetime(s_dtv[0], s_dtv[1], s_dtv[2], s_dtv[3], s_dtv[4]);",
        "Date et heure reglees",
    )

    # The native French UI must have real French literals on the core screens,
    # not merely store "fr" while silently falling back to Italian/English.
    require(
        "firmware/components/nucleo_storage/include/nucleo_i18n.h",
        "TR5(it_, en_, es_, fr_, de_)",
    )
    require(
        "firmware/components/nucleo_storage/nucleo_i18n.c",
        "if (l[0] == 'f' && l[1] == 'r') return fr;",
    )
    require(
        "firmware/components/nucleo_app/app_files.cpp",
        "Chemin trop long",
        "pas d'app pour ce type",
    )
    require(
        "firmware/components/nucleo_app/app_store.cpp",
        "Installer",
        "Mettre a jour",
        "Installee",
    )
    require(
        "firmware/components/nucleo_app/launcher_render.cpp",
        "Bureau",
        "Outils",
        "Systeme",
        "Connecter",
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
    # Shared compatibility facade: every peripheral remains NucleoOS-owned.
    require("integration/nucleoos-bridge/CMakeLists.txt", "components/roza_launcher_hal")
    require("integration/nucleoos-bridge/CMakeLists.txt", "components/roza_gnss")
    require("integration/nucleoos-bridge/main/CMakeLists.txt", "roza_gnss")
    require("scripts/apply-roza-nucleoos.py", "roza_register_gnss();")
    require("integration/nucleoos-bridge/components/roza_gnss/CMakeLists.txt", "roza_gnss.cpp", "roza_gnss_nmea.cpp", "driver", "nucleo_app", "nucleo_setup")
    require("integration/nucleoos-bridge/components/roza_gnss/roza_gnss.cpp", "uart_driver_install", "UART_NUM_1", "uart_set_pin", "115200", "GNSS_TX_PIN = 13", "GNSS_RX_PIN = 15", "roza_gnss_parse_sentence", "nucleo_app_register", "nucleo_setup_set_datetime", "roza_gnss_utc_epoch")
    forbid("integration/nucleoos-bridge/components/roza_gnss/roza_gnss.cpp", "RadioLib", "SX1262", "radio.begin(", "spi_bus_initialize(")
    require("integration/nucleoos-bridge/components/roza_gnss/roza_gnss_nmea.cpp", "roza_gnss_parse_sentence", "roza_gnss_utc_epoch", "719468", "GGA", "RMC", "checksum")
    require("tests/roza_gnss_nmea_test.cpp", "roza_gnss_utc_epoch", "764426119")
    require("integration/nucleoos-bridge/main/CMakeLists.txt", "roza_launcher_hal")
    require(
        "integration/nucleoos-bridge/components/roza_launcher_hal/CMakeLists.txt",
        "roza_launcher_input.cpp", "roza_launcher_hal.cpp", "roza_launcher_diagnostics.cpp",
        "nucleo_app", "nucleo_kbd", "nucleo_power", "nucleo_storage",
    )
    require(
        "integration/nucleoos-bridge/components/roza_launcher_hal/include/roza_launcher_hal.h",
        "roza_launcher_display_target", "roza_launcher_key_from_callback",
        "roza_launcher_battery_percent", "roza_launcher_battery_millivolts",
        "roza_launcher_storage_ready", "roza_launcher_set_brightness",
    )
    require(
        "integration/nucleoos-bridge/components/roza_launcher_hal/roza_launcher_hal.cpp",
        "nucleo_app_gfx()", "nucleo_kbd_mods()", "nucleo_power_battery_pct()",
        "nucleo_power_battery_mv()", "nucleo_storage_info()", "nucleo_app_set_brightness(",
    )
    forbid(
        "integration/nucleoos-bridge/components/roza_launcher_hal/roza_launcher_hal.cpp",
        "M5.begin(", "Wire.begin(", "nucleo_kbd_init(", "nucleo_ui_init(", "SD.begin(",
    )
    require(
        "integration/nucleoos-bridge/components/roza_launcher_hal/roza_launcher_input.cpp",
        "case NK_UP:", "case NK_DOWN:", "case NK_LEFT:", "case NK_RIGHT:",
        "case NK_ENTER:", "case NK_BACK:", "case NK_DEL:", "case NK_TAB:", "case NK_CHAR:",
        "NK_MOD_SHIFT", "NK_MOD_CTRL", "NK_MOD_ALT", "NK_MOD_GUI", "NK_MOD_FN",
    )
    require(
        "integration/nucleoos-bridge/components/roza_launcher_hal/roza_launcher_diagnostics.cpp",
        "roza_launcher_display_target()", "roza_launcher_key_from_callback(",
        "roza_launcher_battery_percent()", "roza_launcher_storage_ready()",
        "roza_register_launcher_diagnostics",
    )
    require("scripts/apply-roza-nucleoos.py", "roza_register_launcher_diagnostics")
    forbid(
        "integration/nucleoos-bridge/components/roza_launcher_sd/roza_launcher_sd.cpp",
        "esp_partition_erase_range(", "esp_ota_write(", 'fopen(sidecar_path, "wb")',
    )

    require("scripts/apply-roza-nucleoos.py", "roza_register_gnss();", "roza_register_lora();")
    require("integration/nucleoos-bridge/CMakeLists.txt", "components/roza_lora")
    require("integration/nucleoos-bridge/main/CMakeLists.txt", "roza_lora")
    require("integration/nucleoos-bridge/components/roza_lora/CMakeLists.txt",
            "jgromes__radiolib", "esp_driver_spi", "esp_driver_i2c", "nucleo_kbd")
    require("integration/nucleoos-bridge/components/roza_lora/idf_component.yml",
            "jgromes/radiolib", "7.8.0")
    require("integration/nucleoos-bridge/components/roza_lora/roza_lora.cpp",
            "SPI2_HOST", "nucleo_kbd_i2c_bus()", "IOE_ADDR=0x43",
            "(id & 0xE0) != 0xA0", "!update_reg(0x05,0x01,0x00)",
            "PIN_SCK=40", "PIN_MISO=39", "PIN_MOSI=14",
            "PIN_NSS=5", "PIN_DIO1=4", "PIN_RESET=3", "PIN_BUSY=6",
            "startReceive()", "setPacketReceivedAction", "getPacketLength()",
            "readData(", "getRSSI()", "getSNR()", "radio_enter", "radio_exit")
    forbid("integration/nucleoos-bridge/components/roza_lora/roza_lora.cpp",
           "s_radio.startTransmit(", "s_radio.transmit(",
           "spi_bus_initialize(", "i2c_new_master_bus(")

    require(
        "tools/installer/flash.sh",
        'if release.get("hardware_tested") is not True:',
        'if release.get("safe_to_flash") is not True:',
        'if install.get("whole_flash_erase") is not False:',
        'read-flash 0x9000 0x5000 "$BACKUP"',
        '0x9000 "$BACKUP"',
        'read-flash 0x9000 0x5000 "$VERIFY"',
        'cmp -s "$BACKUP" "$VERIFY"',
    )
    forbid(
        "tools/installer/flash.sh",
        'if release.get("safe_to_flash") is not False or release.get("hardware_tested") is not False:',
    )
    require(
        ".github/workflows/roza-nucleoos-bridge.yml",
        "Refuse flashing of an unvalidated device candidate",
        "Test installer approval and refusal paths without hardware access",
        "REFUS DE FLASH",
    )
    require(
        "tests/test_flash_installer_guard.py",
        "approved manifest reaches explicit confirmation only",
        "no serial read, erase or write command was executed",
        "whole-flash erase manifest rejected",
    )

    require(
        "firmware/components/nucleo_app/include/nucleo_app.h",
        "#define NUCLEO_APP_MAX 80",
    )
    require(
        "integration/nucleoos-bridge/components/roza_lora/roza_lora.cpp",
        "Reception active", "Dernier paquet", "Reception seule",
    )

    # Launcher console diagnostics must not take ownership of NucleoOS USB CDC.
    require(
        "launcher/src/idf/launcher_platform.cpp",
        "#include <esp_log.h>",
        'esp_log_writev(ESP_LOG_INFO, kLauncherLogTag, fmt, args)',
        'esp_log_write(ESP_LOG_INFO, kLauncherLogTag, "%s", text)',
        "void launcherConsoleBegin(unsigned long baud) { (void)baud; }",
        "void launcherConsoleFlush() {}",
        "void launcherConsoleEnd() {}",
    )
    forbid(
        "launcher/src/idf/launcher_platform.cpp",
        "#include <Arduino.h>",
        "#include <HardwareSerial.h>",
        "Serial.begin(",
        "Serial.end(",
        "Serial.flush(",
        "Serial.write(",
        "Serial.vprintf(",
        "Serial.setTxTimeoutMs(",
    )
    require(
        "scripts/apply-roza-launcher-platform.py",
        'PINNED_LAUNCHER = "4b06de691df15b0fe6a9f09f56d1d107bbf32b12"',
        "verify_patch()",
        "esp_log_writev",
    )

    # The real Launcher M5GFX backend is compiled in a Nucleo-owned target mode.
    require(
        "integration/nucleoos-bridge/components/roza_launcher_hal/CMakeLists.txt",
        "backends/m5gfx_hal.cpp", "roza_launcher_display_app.cpp",
        "m5stack__m5gfx", "arduino-esp32", "ROZA_NUCLEO_TARGET=1",
        "DISPLAY_DEVICE_M5STACK_CARDPUTER=1", "WHOLE_ARCHIVE",
    )
    require(
        "launcher/lib/DisplayDrivers/src/backends/m5gfx_hal.h",
        "#include <M5GFX.h>", "#undef USE_CANVAS",
        "#define M5_DRV (*roza_launcher_graphics_target())",
    )
    require(
        "launcher/lib/DisplayDrivers/src/backends/m5gfx_hal.cpp",
        "roza_launcher_graphics_target()->invertDisplay(i)",
        "parent ? roza_launcher_graphics_target() : nullptr",
        "NucleoOS owns the app draw transaction and frame presentation.",
    )
    require(
        "integration/nucleoos-bridge/components/roza_launcher_hal/include/roza_launcher_hal.h",
        "roza_launcher_display_mode_enter", "roza_launcher_display_mode_exit",
    )
    require(
        "integration/nucleoos-bridge/components/roza_launcher_hal/roza_launcher_hal.cpp",
        "roza_launcher_graphics_target", "nucleo_app_set_direct_draw(true)",
        "nucleo_app_set_direct_draw(false)",
    )
    require(
        "integration/nucleoos-bridge/components/roza_launcher_hal/roza_launcher_display_app.cpp",
        "tft_display", "tft_sprite", "roza_launcher_display_mode_enter()",
        "roza_launcher_display_mode_exit()", "nucleo_app_register(&s_app)",
    )
    forbid(
        "integration/nucleoos-bridge/components/roza_launcher_hal/roza_launcher_display_app.cpp",
        "M5.begin(", "Wire.begin(", "SD.begin(", "nucleo_ui_init(",
    )
    require(
        "scripts/apply-roza-displaydrivers.py",
        "PINNED_DISPLAY_DRIVERS", "ROZA_NUCLEO_TARGET", "M5GFX.h",
        "verify_patch()", "NucleoOS owns the panel",
    )
    require("scripts/apply-roza-nucleoos.py", "roza_register_launcher_display();")
    require(
        "tests/test_roza_display_backend_patch.py",
        "upstream backend is compiled against NucleoOS's current draw target",
        "sprite parent remains stable in direct-draw mode",
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
    print(" - Cardputer ADV pin map, LoRa-1262 receive-only integration and 8 MiB partition layout")
    print(" - legacy Launcher NVS reserved, NucleoOS NVS clean and separate")
    print(" - installer validates approval flags, NVS recovery metadata and exact erase ranges")
    print("This gate does not validate runtime behavior or authorize flashing.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
