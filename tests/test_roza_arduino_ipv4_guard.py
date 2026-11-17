#!/usr/bin/env python3
"""Host test for the pinned Arduino-ESP32 3.2.1 IPv4-only AsyncUDP guard."""
from __future__ import annotations

import contextlib
import importlib.util
import io
import re
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/prepare-roza-arduino-usb.py"
SDKCONFIG = ROOT / "integration/nucleoos-bridge/sdkconfig.defaults"

spec = importlib.util.spec_from_file_location("roza_arduino_prepare", SCRIPT)
if spec is None or spec.loader is None:
    raise SystemExit("Impossible de charger le script de préparation Arduino.")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

FIXTURE = """bool AsyncUDP::listenMulticast(const ip_addr_t *addr, uint16_t port) {
  ip_addr_t bind_addr;
#if CONFIG_LWIP_IPV6
  if (IP_IS_V6(addr)) {
    IP_SET_TYPE(&bind_addr, IPADDR_TYPE_V6);
    ip6_addr_set_any(&bind_addr.u_addr.ip6);
  } else {
#endif
    IP_SET_TYPE(&bind_addr, IPADDR_TYPE_V4);
    ip4_addr_set_any(&bind_addr.u_addr.ip4);
#if CONFIG_LWIP_IPV6
  }
#endif
  return listen(&bind_addr, port);
}
"""

patched = module.patch_asyncudp_ipv4_only(FIXTURE)
if module.ASYNCUDP_NEW_BLOCK not in patched:
    raise SystemExit("Le bloc IPv4-only attendu n'a pas été généré.")
if "ip4_addr_set_any(&bind_addr);" not in patched:
    raise SystemExit("Le chemin IPv4-only n'utilise pas ip4_addr_t directement.")
if "ip6_addr_set_any(&bind_addr.u_addr.ip6);" not in patched:
    raise SystemExit("La branche IPv6 amont doit rester intacte.")
if module.patch_asyncudp_ipv4_only(patched) != patched:
    raise SystemExit("Le patch AsyncUDP n'est pas idempotent.")

try:
    module.patch_asyncudp_ipv4_only(FIXTURE.replace(
        "ip4_addr_set_any(&bind_addr.u_addr.ip4);",
        "ip4_addr_set_any(&bind_addr);",
    ))
except SystemExit:
    pass
else:
    raise SystemExit("Le patch doit refuser une source amont inattendue.")

sdkconfig = SDKCONFIG.read_text(encoding="utf-8")
if not re.search(r"(?m)^CONFIG_LWIP_IPV6=n\s*$", sdkconfig):
    raise SystemExit("Le profil ROZA doit conserver CONFIG_LWIP_IPV6=n.")
if re.search(r"(?m)^CONFIG_LWIP_IPV6=y\s*$", sdkconfig):
    raise SystemExit("Le profil ROZA ne doit pas réactiver IPv6.")

workflow = (ROOT / ".github/workflows/roza-nucleoos-bridge.yml").read_text(encoding="utf-8")
if "python3 tests/test_roza_arduino_ipv4_guard.py" not in workflow:
    raise SystemExit("Le test IPv4-only n'est pas exécuté par la CI de fusion.")


def test_full_prepare_script_is_idempotent() -> None:
    with tempfile.TemporaryDirectory(prefix="roza-arduino-prepare-") as tmp_name:
        root = Path(tmp_name)
        script_copy = root / "scripts" / "prepare-roza-arduino-usb.py"
        script_copy.parent.mkdir(parents=True)
        script_copy.write_text(SCRIPT.read_text(encoding="utf-8"), encoding="utf-8")

        managed = (
            root
            / "integration/nucleoos-bridge/managed_components/espressif__arduino-esp32"
        )
        (managed / "libraries/AsyncUDP/src").mkdir(parents=True)
        (root / "nucleoos/firmware/components/nucleo_usbmsc").mkdir(parents=True)
        (root / "integration/arduino-idf-smoke/components/launcher_platform").mkdir(parents=True)
        (root / "launcher/src").mkdir(parents=True)

        cmake = managed / "CMakeLists.txt"
        cmake.write_text(
            "set(CORE_SRCS\n"
            "  cores/esp32/USBMSC.cpp\n"
            ")\n"
            "set(ARDUINO_ALL_LIBRARIES\n"
            ")\n"
            "idf_component_register(INCLUDE_DIRS ${includedirs} ${priv_includes} ${srcs} ${requires} ${priv_requires})\n",
            encoding="utf-8",
        )
        asyncudp = managed / "libraries/AsyncUDP/src/AsyncUDP.cpp"
        asyncudp.write_text(module.ASYNCUDP_OLD_BLOCK + "\n", encoding="utf-8")
        (root / "nucleoos/firmware/components/nucleo_usbmsc/CMakeLists.txt").write_text(
            "REQUIRES esp_tinyusb\n", encoding="utf-8"
        )
        (root / "nucleoos/firmware/components/nucleo_usbmsc/nucleo_usbmsc.c").write_text(
            "tinyusb_msc_storage_init_sdmmc();\n"
            "tinyusb_driver_install();\n",
            encoding="utf-8",
        )
        (root / "integration/nucleoos-bridge/sdkconfig.defaults").write_text(
            "CONFIG_TINYUSB_MSC_ENABLED=y\n"
            "CONFIG_WL_SECTOR_SIZE_512=y\n"
            "CONFIG_LWIP_IPV6=n\n",
            encoding="utf-8",
        )
        (root / "integration/arduino-idf-smoke/components/launcher_platform/idf_component.yml").write_text(
            'dependencies:\n  espressif/arduino-esp32: "3.2.1"\n',
            encoding="utf-8",
        )

        original_file = module.__file__
        module.__file__ = str(script_copy)
        output = io.StringIO()
        try:
            with contextlib.redirect_stdout(output):
                if module.main() != 0:
                    raise SystemExit("Le premier passage du script de préparation a échoué.")
            first_cmake = cmake.read_text(encoding="utf-8")
            first_asyncudp = asyncudp.read_text(encoding="utf-8")
            with contextlib.redirect_stdout(output):
                if module.main() != 0:
                    raise SystemExit("Le second passage du script de préparation a échoué.")
            if cmake.read_text(encoding="utf-8") != first_cmake:
                raise SystemExit("Le second passage a dupliqué ou modifié les gardes CMake.")
            if asyncudp.read_text(encoding="utf-8") != first_asyncudp:
                raise SystemExit("Le second passage a modifié à nouveau AsyncUDP.")
            if first_cmake.count(module.USB_MARKER) != 1:
                raise SystemExit("La garde USB n'est pas unique après deux passages.")
            if first_cmake.count(module.FATFS_MARKER) != 1:
                raise SystemExit("La garde FatFs n'est pas unique après deux passages.")
            if module.ASYNCUDP_NEW_BLOCK not in first_asyncudp:
                raise SystemExit("La garde AsyncUDP IPv4-only manque après deux passages.")
        finally:
            module.__file__ = original_file


test_full_prepare_script_is_idempotent()

print("Arduino-ESP32 3.2.1 IPv4-only guard: OK")
