#!/usr/bin/env python3
"""Host test for the pinned Arduino-ESP32 3.2.1 IPv4-only AsyncUDP guard."""
from __future__ import annotations

import importlib.util
import re
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

print("Arduino-ESP32 3.2.1 IPv4-only guard: OK")
