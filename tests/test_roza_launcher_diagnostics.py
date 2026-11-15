#!/usr/bin/env python3
"""Static checks for the native Launcher diagnostics app tick lifecycle."""
from __future__ import annotations
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "integration/nucleoos-bridge/components/roza_launcher_hal/roza_launcher_diagnostics.cpp"


def between(source: str, start: str, end: str) -> str:
    if start not in source or end not in source:
        raise SystemExit(f"Bloc source absent : {start} / {end}")
    return source.split(start, 1)[1].split(end, 1)[0]


def main() -> int:
    if not SOURCE.is_file():
        raise SystemExit("Application native de diagnostic absente.")
    source = SOURCE.read_text(encoding="utf-8")
    tick = between(source, "static void diagnostic_tick(void)", "static void diagnostic_enter(void)")
    required = (
        "roza_launcher_battery_percent()",
        "roza_launcher_battery_millivolts()",
        "roza_launcher_storage_ready()",
        "if (battery == s_last_battery_percent",
        "millivolts == s_last_battery_millivolts",
        "storage_ready == s_last_storage_ready",
        "s_last_battery_percent = battery;",
        "s_last_battery_millivolts = millivolts;",
        "s_last_storage_ready = storage_ready;",
        "nucleo_app_request_draw();",
    )
    missing = [token for token in required if token not in tick]
    if missing:
        raise SystemExit("Rafraichissement natif incomplet : " + ", ".join(missing))
    if tick.count("nucleo_app_request_draw();") != 1:
        raise SystemExit("Le tick doit demander au plus un redraw par changement d'etat.")
    if tick.index("nucleo_app_request_draw();") < tick.index("return;"):
        raise SystemExit("Le redraw doit etre ignore quand batterie et SD sont inchanges.")

    enter = between(source, "static void diagnostic_enter(void)", "static void diagnostic_key(int key, char ch)")
    if "diagnostic_tick();" not in enter or "s_last_battery_percent = -101;" not in enter:
        raise SystemExit("L'etat initial du diagnostic n'est pas force au premier tick.")

    app = between(source, "static const nucleo_app_def_t s_app", "} // namespace")
    if "diagnostic_enter, diagnostic_key, diagnostic_tick, diagnostic_draw" not in app:
        raise SystemExit("Le callback on_tick natif n'est pas enregistre.")
    print("ROZA native diagnostics tick: PASS")
    print(" - battery percentage, voltage and SD mount are polled through NucleoOS services")
    print(" - redraw is requested only when one of those values changes")
    print(" - the callback is wired into nucleo_app_def_t::on_tick")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
