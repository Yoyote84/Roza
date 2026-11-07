#!/usr/bin/env python3
"""Apply a guarded, build-only Arduino USB-MSC ownership adjustment for ROZA.

NucleoOS and Arduino-ESP32 share CONFIG_TINYUSB_MSC_ENABLED, but Arduino's
USBMSC.cpp additionally requires the Arduino TinyUSB HAL. In this integration
profile NucleoOS already supplies the actual MSC service, so the Arduino
USBMSC.cpp object must not be compiled unless CONFIG_TINYUSB_ENABLED is active.
This script changes only the downloaded managed-component CMake file.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


MARKER = "# ROZA: gate Arduino USBMSC.cpp on Arduino's own TinyUSB HAL."
ANCHOR = "set(ARDUINO_ALL_LIBRARIES"
SOURCE_ENTRY = "cores/esp32/USBMSC.cpp"
GUARD = """# ROZA: gate Arduino USBMSC.cpp on Arduino's own TinyUSB HAL.
# CONFIG_TINYUSB_MSC_ENABLED is also enabled by NucleoOS's esp_tinyusb.
# In this bridge NucleoOS owns the real MSC implementation; Arduino's
# USBMSC.cpp additionally requires CONFIG_TINYUSB_ENABLED. Do not compile
# that second MSC object unless the Arduino TinyUSB HAL is actually enabled.
if(CONFIG_TINYUSB_MSC_ENABLED AND NOT CONFIG_TINYUSB_ENABLED)
    list(REMOVE_ITEM CORE_SRCS cores/esp32/USBMSC.cpp)
endif()

"""


def fail(message: str) -> "None":
    print(f"ROZA USB integration guard: ERROR: {message}", file=sys.stderr)
    raise SystemExit(1)


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

    for required in (managed_cmake, nucleo_cmake, nucleo_source, sdkconfig, launcher_src, arduino_manifest):
        if not required.exists():
            fail(f"required input is missing: {required.relative_to(root)}")

    manifest_text = arduino_manifest.read_text(encoding="utf-8")
    if not re.search(r'espressif/arduino-esp32\s*:\s*["\']3\.2\.1["\']', manifest_text):
        fail("the guard is validated only for the pinned Arduino-ESP32 3.2.1 component")

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
    if MARKER in cmake_text:
        if GUARD not in cmake_text:
            fail("ROZA marker exists but the expected CMake guard is missing or altered")
        print("ROZA USB integration guard: already applied; no changes needed.")
        return 0

    if cmake_text.count(SOURCE_ENTRY) != 1:
        fail(f"expected exactly one {SOURCE_ENTRY!r} entry in Arduino CORE_SRCS")
    if cmake_text.count(ANCHOR) != 1:
        fail(f"expected exactly one {ANCHOR!r} CMake insertion point")

    patched = cmake_text.replace(ANCHOR, GUARD + ANCHOR, 1)
    if patched == cmake_text or MARKER not in patched:
        fail("could not insert the guarded CORE_SRCS condition")

    managed_cmake.write_text(patched, encoding="utf-8")
    check = managed_cmake.read_text(encoding="utf-8")
    if check.count(MARKER) != 1 or GUARD not in check:
        fail("post-write verification failed")

    print(
        "ROZA USB integration guard applied: Arduino USBMSC.cpp is gated by "
        "CONFIG_TINYUSB_ENABLED; NucleoOS's real esp_tinyusb MSC remains enabled."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
