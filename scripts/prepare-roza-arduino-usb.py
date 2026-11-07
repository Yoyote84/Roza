#!/usr/bin/env python3
"""Apply guarded, build-only Arduino-ESP32 3.2.1 compatibility adjustments.

NucleoOS owns the real USB MSC implementation through esp_tinyusb. The shared
CONFIG_TINYUSB_MSC_ENABLED Kconfig symbol must not accidentally enable Arduino's
separate USBMSC.cpp without Arduino's own TinyUSB HAL.

ESP-IDF 5.4.4 also fixes FatFs sectors at 512 bytes in this bridge profile:
SD sectors are 512 bytes and CONFIG_WL_SECTOR_SIZE_512 is enabled. In that
fixed-sector configuration FATFS has no ssize member. Arduino 3.2.1 tests its
legacy _MAX_SS macro and needs it aligned to 512 so it selects the fixed-sector
code path. Both adjustments are made only to the downloaded managed component.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


USB_MARKER = "# ROZA: gate Arduino USBMSC.cpp on Arduino's own TinyUSB HAL."
USB_ANCHOR = "set(ARDUINO_ALL_LIBRARIES"
USB_SOURCE_ENTRY = "cores/esp32/USBMSC.cpp"
USB_GUARD = """# ROZA: gate Arduino USBMSC.cpp on Arduino's own TinyUSB HAL.
# CONFIG_TINYUSB_MSC_ENABLED is also enabled by NucleoOS's esp_tinyusb.
# In this bridge NucleoOS owns the real MSC implementation; Arduino's
# USBMSC.cpp additionally requires CONFIG_TINYUSB_ENABLED. Do not compile
# that second MSC object unless the Arduino TinyUSB HAL is actually enabled.
if(CONFIG_TINYUSB_MSC_ENABLED AND NOT CONFIG_TINYUSB_ENABLED)
    list(REMOVE_ITEM CORE_SRCS cores/esp32/USBMSC.cpp)
endif()

"""

FATFS_MARKER = "# ROZA: align Arduino's legacy FatFs sector-size test with ESP-IDF."
FATFS_ANCHOR = (
    "idf_component_register(INCLUDE_DIRS \${includedirs} PRIV_INCLUDE_DIRS "
    "\${priv_includes} SRCS \${srcs} REQUIRES \${requires} PRIV_REQUIRES "
    "\${priv_requires})"
)
FATFS_GUARD = """# ROZA: align Arduino's legacy FatFs sector-size test with ESP-IDF.
# ESP-IDF 5.4.4 sets FF_SS_SDCARD=512; ROZA also pins CONFIG_WL_SECTOR_SIZE_512=y.
# Therefore FF_MIN_SS == FF_MAX_SS == 512 and FATFS intentionally has no ssize.
# Arduino-ESP32 3.2.1 checks the legacy _MAX_SS macro; without it the #if
# takes the variable-sector branch and references the absent FATFS.ssize field.
if(NOT CONFIG_WL_SECTOR_SIZE_512)
    message(FATAL_ERROR
        "ROZA Arduino FatFs compatibility requires CONFIG_WL_SECTOR_SIZE_512"
    )
endif()
target_compile_definitions(\${COMPONENT_LIB} PRIVATE _MAX_SS=512)

"""


def fail(message: str) -> "None":
    print(f"ROZA Arduino compatibility guard: ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


def add_once(text: str, marker: str, anchor: str, addition: str, label: str) -> str:
    if marker in text:
        if addition not in text:
            fail(f"{label} marker exists but its expected CMake block is missing or altered")
        return text
    if text.count(anchor) != 1:
        fail(f"expected exactly one {label} insertion point")
    if label == "USB":
        result = text.replace(anchor, addition + anchor, 1)
    else:
        result = text.replace(anchor, anchor + "\n\n" + addition, 1)
    if result == text or marker not in result:
        fail(f"could not insert the {label} compatibility block")
    return result


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    managed_cmake = (
        root
        / "integration/nucleoos-bridge/managed_components/espressif__arduino-esp32/CMakeLists.txt"
    )
    nucleo_cmake = root / "nucleoos/firmware/components/nucleo_usbmsc/CMakeLists.txt"
    nucleo_source = root / "nucleoos/firmware/components/nucleo_usbmsc/nucleo_usbmsc.c"
    sdkconfig = root / "integration/nucleoos-bridge/sdkconfig.defaults"
    launcher_src = root / "launcher/src"
    arduino_manifest = (
        root
        / "integration/arduino-idf-smoke/components/launcher_platform/idf_component.yml"
    )

    required_paths = (
        managed_cmake,
        nucleo_cmake,
        nucleo_source,
        sdkconfig,
        launcher_src,
        arduino_manifest,
    )
    for required in required_paths:
        if not required.exists():
            fail(f"required input is missing: {required.relative_to(root)}")

    manifest_text = arduino_manifest.read_text(encoding="utf-8")
    if not re.search(r'espressif/arduino-esp32\s*:\s*["\']3\.2\.1["\']', manifest_text):
        fail("the guards are validated only for the pinned Arduino-ESP32 3.2.1 component")

    nucleo_cmake_text = nucleo_cmake.read_text(encoding="utf-8")
    nucleo_source_text = nucleo_source.read_text(encoding="utf-8")
    sdkconfig_text = sdkconfig.read_text(encoding="utf-8")

    if "esp_tinyusb" not in nucleo_cmake_text:
        fail("NucleoOS nucleo_usbmsc no longer declares esp_tinyusb")
    if "tinyusb_msc_storage_init_sdmmc" not in nucleo_source_text:
        fail("NucleoOS's real SD-backed MSC initialization was not found")
    if "tinyusb_driver_install" not in nucleo_source_text:
        fail("NucleoOS's real TinyUSB driver installation was not found")
    if not re.search(r"(?m)^CONFIG_TINYUSB_MSC_ENABLED=y\s*$", sdkconfig_text):
        fail("the bridge profile no longer enables NucleoOS TinyUSB MSC")
    if not re.search(r"(?m)^CONFIG_WL_SECTOR_SIZE_512=y\s*$", sdkconfig_text):
        fail("the bridge profile no longer pins wear-levelling sectors to 512 bytes")

    # Do not omit Arduino's object if any Launcher source starts using the
    # Arduino USBMSC class; in that case it must be integrated deliberately.
    suffixes = {".c", ".cc", ".cpp", ".h", ".hh", ".hpp", ".ino"}
    usages: list[str] = []
    for path in launcher_src.rglob("*"):
        if not path.is_file() or path.suffix.lower() not in suffixes:
            continue
        try:
            content = path.read_text(encoding="utf-8", errors="ignore")
        except OSError as exc:
            fail(f"cannot inspect Launcher source {path}: {exc}")
        if re.search(r"\bUSBMSC\b|USBMSC\.h", content):
            usages.append(str(path.relative_to(root)))
    if usages:
        fail(
            "Launcher now references Arduino USBMSC; do not exclude its object. "
            "Integrate that API with the shared USB owner first. Found: "
            + ", ".join(usages[:10])
        )

    cmake_text = managed_cmake.read_text(encoding="utf-8")
    if cmake_text.count(USB_SOURCE_ENTRY) != 1:
        fail(f"expected exactly one {USB_SOURCE_ENTRY!r} entry in Arduino CORE_SRCS")
    if cmake_text.count(FATFS_ANCHOR) != 1:
        fail("Arduino component registration anchor changed; refusing a blind FatFs patch")

    cmake_text = add_once(cmake_text, USB_MARKER, USB_ANCHOR, USB_GUARD, "USB")
    cmake_text = add_once(cmake_text, FATFS_MARKER, FATFS_ANCHOR, FATFS_GUARD, "FatFs")

    managed_cmake.write_text(cmake_text, encoding="utf-8")
    check = managed_cmake.read_text(encoding="utf-8")
    for marker, block, label in (
        (USB_MARKER, USB_GUARD, "USB"),
        (FATFS_MARKER, FATFS_GUARD, "FatFs"),
    ):
        if check.count(marker) != 1 or block not in check:
            fail(f"post-write verification failed for {label}")

    print(
        "ROZA compatibility guards applied: NucleoOS retains real TinyUSB MSC; "
        "Arduino USBMSC is conditional; Arduino FatFs uses the verified fixed 512-byte sector path."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
