#!/usr/bin/env python3
"""Patch the pinned Launcher DisplayDrivers M5GFX backend for NucleoOS ownership."""
from __future__ import annotations
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
DISPLAY_ROOT = ROOT / "launcher/lib/DisplayDrivers"
PINNED_DISPLAY_DRIVERS = "79d2dc757d0684a3f63ab027920edd152972f93f"
HEADER = DISPLAY_ROOT / "src/backends/m5gfx_hal.h"
SOURCE = DISPLAY_ROOT / "src/backends/m5gfx_hal.cpp"


def git_head(path: Path) -> str:
    result = subprocess.run(
        ["git", "-C", str(path), "rev-parse", "HEAD"],
        check=False, capture_output=True, text=True,
    )
    if result.returncode:
        raise SystemExit(f"Impossible de lire la revision Git : {path}")
    return result.stdout.strip()


def replace_exact(path: Path, old: str, new: str) -> bool:
    if not path.is_file():
        raise SystemExit(f"Source DisplayDrivers absente : {path}")
    source = path.read_text(encoding="utf-8")
    old_count = source.count(old)
    new_count = source.count(new)
    if old_count == 1 and new_count == 0:
        path.write_text(source.replace(old, new, 1), encoding="utf-8")
        return True
    if new_count == 1 and old_count == 0:
        return False
    if new_count == 1 and old_count == 1 and old in new:
        return False
    raise SystemExit(
        f"Patch refuse pour {path.relative_to(ROOT)} : "
        f"source={old_count}, deja-applique={new_count}; revue requise."
    )


def verify_patch() -> None:
    header = HEADER.read_text(encoding="utf-8")
    source = SOURCE.read_text(encoding="utf-8")
    required_header = (
        "#include <M5GFX.h>",
        'extern "C" LovyanGFX *roza_launcher_graphics_target(void);',
        "#undef USE_CANVAS",
        "#define M5_DRV (*roza_launcher_graphics_target())",
    )
    required_source = (
        "void tft_display::begin(uint32_t speed)",
        "void tft_display::init(uint8_t tc)",
        "roza_launcher_graphics_target()->invertDisplay(i)",
        "tft_sprite::tft_sprite(tft_display *parent)",
        "parent ? roza_launcher_graphics_target() : nullptr",
    )
    missing = [item for item in required_header if item not in header]
    missing += [item for item in required_source if item not in source]
    if missing:
        raise SystemExit("Adaptateur M5GFX ROZA incomplet : " + ", ".join(missing))

    init = source.split("void tft_display::init", 1)[1].split(
        "void tft_display::setRotation", 1
    )[0]
    if not (
        "#if defined(ROZA_NUCLEO_TARGET)" in init
        and "return;" in init
        and "#else" in init
        and "M5.begin();" in init
        and init.index("return;") < init.index("M5.begin();")
    ):
        raise SystemExit("Le chemin ROZA de init() doit sortir avant M5.begin().")

    begin = source.split("void tft_display::begin", 1)[1].split(
        "void tft_display::init", 1
    )[0]
    if "#if defined(ROZA_NUCLEO_TARGET)" not in begin:
        raise SystemExit("begin() ne protege pas le mode ROZA.")

    display = source.split("void tft_display::display", 1)[1].split(
        "int16_t tft_display::drawAlignedString", 1
    )[0]
    roza_display = display.split("#if defined(ROZA_NUCLEO_TARGET)", 1)[1].split(
        "#else", 1
    )[0]
    if "_canvas" in roza_display or "M5.Display" in roza_display:
        raise SystemExit("Le mode ROZA ne doit ni pousser un canvas ni acceder a M5.Display.")

    sprite = source.split("tft_sprite::tft_sprite", 1)[1].split(
        "void *tft_sprite::createSprite", 1
    )[0]
    if "roza_launcher_graphics_target()" not in sprite:
        raise SystemExit("Les sprites doivent utiliser la cible graphique NucleoOS.")


def main() -> int:
    actual = git_head(DISPLAY_ROOT)
    if actual != PINNED_DISPLAY_DRIVERS:
        raise SystemExit(
            f"DisplayDrivers inattendu : attendu {PINNED_DISPLAY_DRIVERS}, obtenu {actual}. "
            "Aucun patch applique."
        )

    header_changed = False
    header_changed |= replace_exact(
        HEADER,
        """#include <M5Unified.h>
#include <SPI.h>
""",
        """#if defined(ROZA_NUCLEO_TARGET)
#include <Arduino.h>
#include <M5GFX.h>
#include <SPI.h>
extern "C" LovyanGFX *roza_launcher_graphics_target(void);
#else
#include <M5Unified.h>
#include <SPI.h>
#endif
""",
    )
    header_changed |= replace_exact(
        HEADER,
        """#if defined(USE_CANVAS)
extern M5Canvas _canvas;
    #define M5_DRV _canvas
#else
    #define M5_DRV M5.Display
#endif
""",
        """#if defined(ROZA_NUCLEO_TARGET)
#undef USE_CANVAS
#define M5_DRV (*roza_launcher_graphics_target())
#elif defined(USE_CANVAS)
extern M5Canvas _canvas;
    #define M5_DRV _canvas
#else
    #define M5_DRV M5.Display
#endif
""",
    )

    source_changed = False
    source_edits = [
        (
            """#if defined(USE_CANVAS)
M5Canvas _canvas(&M5.Display);
#endif
""",
            """#if defined(USE_CANVAS) && !defined(ROZA_NUCLEO_TARGET)
M5Canvas _canvas(&M5.Display);
#endif
""",
        ),
        (
            """void tft_display::begin(uint32_t speed) {
    (void)speed;
#if defined(USE_CANVAS)
    _canvas.createSprite(M5.Display.width(), M5.Display.height());
#endif
}
""",
            """void tft_display::begin(uint32_t speed) {
    (void)speed;
#if defined(ROZA_NUCLEO_TARGET)
    // NucleoOS owns the panel and the shared framebuffer; allocate no second canvas.
#elif defined(USE_CANVAS)
    _canvas.createSprite(M5.Display.width(), M5.Display.height());
#endif
}
""",
        ),
        (
            """void tft_display::init(uint8_t tc) {
    (void)tc;
    M5.begin();
}
""",
            """void tft_display::init(uint8_t tc) {
#if defined(ROZA_NUCLEO_TARGET)
    (void)tc;
    // NucleoOS initialized M5 and the panel once, before entering the app registry.
    return;
#else
    (void)tc;
    M5.begin();
#endif
}
""",
        ),
        (
            """void tft_display::setRotation(uint8_t r) {
    M5_DRV.setRotation(r);
    _rotation = r;
}
""",
            """void tft_display::setRotation(uint8_t r) {
#if defined(ROZA_NUCLEO_TARGET)
    (void)r; // Panel orientation belongs to NucleoOS.
#else
    M5_DRV.setRotation(r);
    _rotation = r;
#endif
}
""",
        ),
        (
            """void tft_display::invertDisplay(bool i) {
    (void)i;
    M5.Display.invertDisplay(i);
}
""",
            """void tft_display::invertDisplay(bool i) {
#if defined(ROZA_NUCLEO_TARGET)
    roza_launcher_graphics_target()->invertDisplay(i);
#else
    M5.Display.invertDisplay(i);
#endif
}
""",
        ),
        (
            """uint8_t tft_display::getRotation() const { return _rotation; }
""",
            """uint8_t tft_display::getRotation() const {
#if defined(ROZA_NUCLEO_TARGET)
    return roza_launcher_graphics_target()->getRotation();
#else
    return _rotation;
#endif
}
""",
        ),
        (
            """lgfx::LGFX_Device *tft_display::native() { return nullptr; }
""",
            """lgfx::LGFX_Device *tft_display::native() {
#if defined(ROZA_NUCLEO_TARGET)
    return static_cast<lgfx::LGFX_Device *>(roza_launcher_graphics_target());
#else
    return nullptr;
#endif
}
""",
        ),
        (
            """void tft_display::display(bool fullRefresh) {
    (void)fullRefresh;
#if defined(E_PAPER_DISPLAY)
    M5.Display.waitDisplay();
    M5.Display.display();
    M5.Display.waitDisplay();
#endif
#if defined(USE_CANVAS)
    RUN_ON_MUTEX(_canvas.pushSprite(0, 0));
#endif
}
""",
            """void tft_display::display(bool fullRefresh) {
#if defined(ROZA_NUCLEO_TARGET)
    (void)fullRefresh;
    // NucleoOS owns the app draw transaction and frame presentation.
#else
    (void)fullRefresh;
#if defined(E_PAPER_DISPLAY)
    M5.Display.waitDisplay();
    M5.Display.display();
    M5.Display.waitDisplay();
#endif
#if defined(USE_CANVAS)
    RUN_ON_MUTEX(_canvas.pushSprite(0, 0));
#endif
#endif
}
""",
        ),
        (
            """tft_sprite::tft_sprite(tft_display *parent) : lgfx::LGFX_Sprite(parent ? &M5.Display : nullptr) {}
""",
            """#if defined(ROZA_NUCLEO_TARGET)
tft_sprite::tft_sprite(tft_display *parent)
    : lgfx::LGFX_Sprite(parent ? roza_launcher_graphics_target() : nullptr) {}
#else
tft_sprite::tft_sprite(tft_display *parent) : lgfx::LGFX_Sprite(parent ? &M5.Display : nullptr) {}
#endif
""",
        ),
    ]
    for old, new in source_edits:
        source_changed |= replace_exact(SOURCE, old, new)

    verify_patch()
    print(
        "DisplayDrivers M5GFX adapte a NucleoOS — "
        f"header={'modifie' if header_changed else 'deja adapte'}, "
        f"backend={'modifie' if source_changed else 'deja adapte'}."
    )
    print("Aucun M5.begin(), canvas plein ecran ou changement de rotation dans le chemin ROZA.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
