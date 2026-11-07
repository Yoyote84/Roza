#!/usr/bin/env python3
"""Apply the ROZA theme and native Launcher-feature hooks to pinned NucleoOS sources."""

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
NUCLEO_ROOT = ROOT / "nucleoos"
PINNED_NUCLEOOS = "cb0f1a5356ddd6f96ec6b31bbcba5d55f52d8123"

THEME_FILE = NUCLEO_ROOT / "firmware/components/nucleo_ui/nucleo_theme.cpp"
UI_FILE = NUCLEO_ROOT / "firmware/components/nucleo_ui/nucleo_ui.cpp"
MAIN_FILE = NUCLEO_ROOT / "firmware/main/main.c"

THEME_GLOBALS_OLD = """uint16_t THEME_BG = 0x0841;
uint16_t THEME_FG = 0xFFFF;
uint16_t THEME_MUTED = 0x8C71;
uint16_t THEME_DIM = 0x4410;
uint16_t THEME_LINE = 0x2945;
uint16_t THEME_INK = 0x0000;
uint16_t THEME_SEL = 0x4D9F;
uint16_t THEME_ACC = 0x4D9F;
"""

THEME_GLOBALS_NEW = """uint16_t THEME_BG = 0xF77C;
uint16_t THEME_FG = 0x2924;
uint16_t THEME_MUTED = 0x83CD;
uint16_t THEME_DIM = 0xB553;
uint16_t THEME_LINE = 0xD677;
uint16_t THEME_INK = 0x2924;
uint16_t THEME_SEL = 0x7927;
uint16_t THEME_ACC = 0xA148;
"""

ROZA_THEME = '''static const nucleo_theme_t s_themes[] = {
    {
        "roza", "ROZA Papier",
        0xF77C, // bg: papier clair
        0x2924, // fg: encre sombre
        0x83CD, // muted
        0xB553, // dim
        0xD677, // line
        0x2924, // ink
        0x7927, // sel: bordeaux
        0xA148  // acc: rose bordeaux
    },
    {
        "classic", "Nucleo Classic",'''

def replace_exact(path: Path, edits: list[tuple[str, str]]) -> bool:
    if not path.is_file():
        raise SystemExit(f"Source NucleoOS absente : {path}")
    source = path.read_text(encoding="utf-8")
    changed = False
    for old, new in edits:
        old_count = source.count(old)
        new_count = source.count(new)
        # Some insertions deliberately retain the original declaration/call inside the
        # replacement block. Recognize that exact larger block before trying the old text,
        # otherwise a second CI invocation would append the hook a second time.
        if new_count == 1 and old_count == 1 and old in new:
            continue
        if old_count == 1 and new_count == 0:
            source = source.replace(old, new, 1)
            changed = True
        elif old_count == 0 and new_count == 1:
            continue
        else:
            raise SystemExit(
                f"Patch refuse pour {path.relative_to(ROOT)} : "
                f"source={old_count}, deja-applique={new_count}; revue requise."
            )
    if changed:
        path.write_text(source, encoding="utf-8")
    return changed

def verify_pinned_source() -> None:
    result = subprocess.run(
        ["git", "-C", str(NUCLEO_ROOT), "rev-parse", "HEAD"],
        check=False, capture_output=True, text=True,
    )
    actual = result.stdout.strip()
    if result.returncode != 0 or actual != PINNED_NUCLEOOS:
        raise SystemExit(
            "Version NucleoOS inattendue : "
            f"attendu {PINNED_NUCLEOOS}, obtenu {actual or 'introuvable'}. "
            "Aucune surcouche appliquee."
        )

def main() -> int:
    verify_pinned_source()
    theme_changed = replace_exact(THEME_FILE, [
        (THEME_GLOBALS_OLD, THEME_GLOBALS_NEW),
        ('static char s_current_theme[24] = "classic";',
         'static char s_current_theme[24] = "roza";'),
        ('static const nucleo_theme_t s_themes[] = {\n    {\n        "classic", "Nucleo Classic",',
         ROZA_THEME),
    ])
    ui_changed = replace_exact(UI_FILE, [
        ('cv.drawString("NucleoOS", (int)SPL_CX, 92);',
         'cv.drawString("ROZA", (int)SPL_CX, 92);'),
        ('float uw = cv.textWidth("NucleoOS") * 0.5f * spl_clamp((t - 1.05f) / 0.5f, 0, 1);',
         'float uw = cv.textWidth("ROZA") * 0.5f * spl_clamp((t - 1.05f) / 0.5f, 0, 1);'),
        ('cv.drawString("web-native os", (int)SPL_CX, 117);',
         'cv.drawString("CARDPUTER ADV", (int)SPL_CX, 117);'),
    ])
    main_changed = replace_exact(MAIN_FILE, [
        ('nucleo_ui_message("NucleoOS",', 'nucleo_ui_message("ROZA",'),
        ('nucleo_ui_home("NucleoOS",', 'nucleo_ui_home("ROZA",'),
        'extern void nucleo_store_apps_sync(void);   // nucleo_app/store_apps.cpp: store apps join the launcher\nextern void roza_register_launcher_partitions(void); // native read-only Launcher partition view\nextern void roza_register_launcher_sd(void); // native read-only SD firmware preflight'),
         'extern void nucleo_store_apps_sync(void);   // nucleo_app/store_apps.cpp: store apps join the launcher\nextern void roza_register_launcher_partitions(void); // native read-only Launcher partition view\nextern void roza_register_launcher_sd(void); // native read-only SD firmware preflight'),
        (            nucleo_app_register_builtins();          // including a native game installed from the store\n            roza_register_launcher_partitions();\n            roza_register_launcher_sd();\n            nucleo_store_apps_sync();'),
         '            nucleo_app_register_builtins();          // including a native game installed from the store\n            roza_register_launcher_partitions();\n            roza_register_launcher_sd();\n            nucleo_store_apps_sync();'),
        (        nucleo_app_register_builtins();\n        roza_register_launcher_partitions();\n        roza_register_launcher_sd();\n        bootmark("app-builtins");'),
         '        nucleo_app_register_builtins();\n        roza_register_launcher_partitions();\n        roza_register_launcher_sd();\n        bootmark("app-builtins");'),
    ])
    print(
        "Surcouche ROZA appliquee — "
        f"theme={'modifie' if theme_changed else 'deja present'}, "
        f"splash={'modifie' if ui_changed else 'deja present'}, "
        f"accueil et apps Partitions/Firmware SD={'modifies' if main_changed else 'deja presents'}."
    )
    print("ROZA ajoute les inspecteurs Partitions et Firmware SD sans remplacer les apps NucleoOS.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
