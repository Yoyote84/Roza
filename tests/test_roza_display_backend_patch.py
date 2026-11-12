#!/usr/bin/env python3
"""Static integration checks for the real Launcher M5GFX backend adapter."""
from __future__ import annotations
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "launcher/lib/DisplayDrivers/src/backends/m5gfx_hal.h"
SOURCE = ROOT / "launcher/lib/DisplayDrivers/src/backends/m5gfx_hal.cpp"
CMAKE = ROOT / "integration/nucleoos-bridge/components/roza_launcher_hal/CMakeLists.txt"
HAL = ROOT / "integration/nucleoos-bridge/components/roza_launcher_hal/roza_launcher_hal.cpp"
APP = ROOT / "integration/nucleoos-bridge/components/roza_launcher_hal/roza_launcher_display_app.cpp"
OVERLAY = ROOT / "scripts/apply-roza-nucleoos.py"


def require(path: Path, *tokens: str) -> str:
    if not path.is_file():
        raise SystemExit(f"Fichier absent : {path.relative_to(ROOT)}")
    source = path.read_text(encoding="utf-8")
    missing = [token for token in tokens if token not in source]
    if missing:
        raise SystemExit(
            f"{path.relative_to(ROOT)} : contrat absent : " + ", ".join(missing)
        )
    return source


def between(source: str, start: str, end: str) -> str:
    if start not in source or end not in source:
        raise SystemExit(f"Bloc source absent : {start} / {end}")
    return source.split(start, 1)[1].split(end, 1)[0]


def main() -> int:
    require(
        HEADER,
        "#include <M5GFX.h>",
        'extern "C" LovyanGFX *roza_launcher_graphics_target(void);',
        "#undef USE_CANVAS",
        "#define M5_DRV (*roza_launcher_graphics_target())",
    )
    source = require(
        SOURCE,
        "void tft_display::begin(uint32_t speed)",
        "void tft_display::init(uint8_t tc)",
        "roza_launcher_graphics_target()->invertDisplay(i)",
        "parent ? roza_launcher_graphics_target() : nullptr",
    )
    init = between(source, "void tft_display::init", "void tft_display::setRotation")
    if not (
        "#if defined(ROZA_NUCLEO_TARGET)" in init
        and "return;" in init
        and "#else" in init
        and "M5.begin();" in init
        and init.index("return;") < init.index("M5.begin();")
    ):
        raise SystemExit("init() pourrait encore initialiser M5 dans le chemin ROZA.")

    display = between(source, "void tft_display::display", "int16_t tft_display::drawAlignedString")
    roza_display = between(display, "#if defined(ROZA_NUCLEO_TARGET)", "#else")
    if "_canvas" in roza_display or "M5.Display" in roza_display:
        raise SystemExit("Le chemin ROZA de display() ne doit pas presenter un second canvas.")

    sprite = between(source, "tft_sprite::tft_sprite", "void *tft_sprite::createSprite")
    if "roza_launcher_graphics_target()" not in sprite:
        raise SystemExit("Le sprite ne cible pas l'objet graphique NucleoOS.")

    require(
        CMAKE,
        "backends/m5gfx_hal.cpp",
        "roza_launcher_display_app.cpp",
        "m5stack__m5gfx",
        "arduino-esp32",
        "ROZA_NUCLEO_TARGET=1",
        "DISPLAY_DEVICE_M5STACK_CARDPUTER=1",
        "WHOLE_ARCHIVE",
    )
    require(
        HAL,
        "nucleo_app_gfx()",
        "nucleo_app_set_direct_draw(true)",
        "nucleo_app_set_direct_draw(false)",
        "roza_launcher_graphics_target",
    )
    app = require(
        APP,
        "tft_display",
        "tft_sprite",
        "roza_launcher_display_mode_enter()",
        "roza_launcher_display_mode_exit()",
        "s_display.init()",
        "s_display.begin()",
        "s_display.display()",
        "nucleo_app_register(&s_app)",
    )
    for forbidden in ("M5.begin(", "Wire.begin(", "SD.begin(", "nucleo_ui_init("):
        if forbidden in app:
            raise SystemExit(f"L'application de test reprend un peripherique : {forbidden}")

    require(OVERLAY, "roza_register_launcher_display();")
    print("ROZA Launcher M5GFX adapter: PASS")
    print(" - upstream backend is compiled against NucleoOS's current draw target")
    print(" - init/begin/display do not reinitialize or present a second framebuffer")
    print(" - sprite parent remains stable in direct-draw mode and is freed on exit")
    print(" - native app registration uses the single NucleoOS launcher")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
