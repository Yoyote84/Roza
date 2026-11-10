#!/usr/bin/env python3
"""Apply ROZA theme, French first-boot language and native Launcher-feature hooks to pinned NucleoOS sources."""

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
NUCLEO_ROOT = ROOT / "nucleoos"
PINNED_NUCLEOOS = "cb0f1a5356ddd6f96ec6b31bbcba5d55f52d8123"

THEME_FILE = NUCLEO_ROOT / "firmware/components/nucleo_ui/nucleo_theme.cpp"
UI_FILE = NUCLEO_ROOT / "firmware/components/nucleo_ui/nucleo_ui.cpp"
I18N_FILE = NUCLEO_ROOT / "firmware/components/nucleo_storage/nucleo_i18n.c"
MAIN_FILE = NUCLEO_ROOT / "firmware/main/main.c"
UPDATE_ENGINE_FILE = NUCLEO_ROOT / "firmware/components/nucleo_app/nucleo_update.c"
UPDATE_UI_FILE = NUCLEO_ROOT / "firmware/components/nucleo_app/app_updates.cpp"

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

def verify_roza_update_gate() -> None:
    engine = UPDATE_ENGINE_FILE.read_text(encoding="utf-8")
    ui = UPDATE_UI_FILE.read_text(encoding="utf-8")
    required_engine = (
        'https://updates.roza.invalid/',
        'NucleoOS release tags are not ROZA release tags',
        'goto out_silent;',
        'fail_install(UT("Canal de mise a jour ROZA non configure"',
        'nvs_erase_key(s_nvs, "install")',
    )
    required_ui = (
        'ROZA update channel pending',
        'ROZA update channel not configured',
        'ROZA updates (signed release channel pending)',
    )
    missing = [item for item in required_engine if item not in engine]
    missing += [item for item in required_ui if item not in ui]
    if missing:
        raise SystemExit("Verrou OTA ROZA incomplet : " + ", ".join(missing))
    if "https://indecenti.github.io/NucleoOs/" in engine:
        raise SystemExit("Le moteur ROZA reference encore le flux OTA NucleoOS.")
    if "indecenti.github.io/NucleoOs" in ui:
        raise SystemExit("L'interface ROZA expose encore le lien de mise a jour NucleoOS.")


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
    i18n_changed = replace_exact(I18N_FILE, [
        (
            '// DEFAULT IS ENGLISH: a fresh device (no settings.json / no ui.language yet) boots in English, and the\n'
            '// first-run wizard asks the user to pick their language as its very first step.\n'
            'static char s_lang[6] = "en";              // active OS language code (default English)',
            '// DEFAULT IS FRENCH for ROZA: a fresh device starts in French. An existing settings.json ui.language\n'
            '// is loaded below and always wins, so a saved user choice is never overwritten.\n'
            'static char s_lang[6] = "fr";              // active OS language code (default French)',
        ),
    ])

    main_changed = replace_exact(MAIN_FILE, [
        ('nucleo_ui_message("NucleoOS",', 'nucleo_ui_message("ROZA",'),
        ('nucleo_ui_home("NucleoOS",', 'nucleo_ui_home("ROZA",'),
        ('extern void nucleo_store_apps_sync(void);   // nucleo_app/store_apps.cpp: store apps join the launcher',
         'extern void nucleo_store_apps_sync(void);   // nucleo_app/store_apps.cpp: store apps join the launcher\nextern void roza_register_launcher_partitions(void); // native read-only Launcher partition view\nextern void roza_register_launcher_sd(void); // native read-only SD firmware preflight\nextern void roza_register_launcher_diagnostics(void); // shared native HAL diagnostic'),
        ('            nucleo_app_register_builtins();          // including a native game installed from the store\n            nucleo_store_apps_sync();',
         '            nucleo_app_register_builtins();          // including a native game installed from the store\n            roza_register_launcher_partitions();\n            roza_register_launcher_sd();\n            roza_register_launcher_diagnostics();\n            nucleo_store_apps_sync();'),
        ('        nucleo_app_register_builtins();      bootmark("app-builtins");',
         '        nucleo_app_register_builtins();\n        roza_register_launcher_partitions();\n        roza_register_launcher_sd();\n        roza_register_launcher_diagnostics();\n        bootmark("app-builtins");'),
    ])
    updater_changed = replace_exact(UPDATE_ENGINE_FILE, [
        ('#define UPD_BASE        "https://indecenti.github.io/NucleoOs/"',
         '#define UPD_BASE        "https://updates.roza.invalid/"   // fail-closed until ROZA publishes its own signed release channel'),
        ('static bool best_known_latest_typed(char *out, size_t cap, bool *security)\n{\n',
         'static bool best_known_latest_typed(char *out, size_t cap, bool *security)\n{\n    // NucleoOS release tags are not ROZA release tags. Never expose or act on its cached feed.\n    if (out && cap) out[0] = 0;\n    if (security) *security = false;\n    return false;\n'),
        ('static void install_task(void *arg)\n{\n    (void)arg;\n    char want[65];',
         'static void install_task(void *arg)\n{\n    (void)arg;\n    // Fail before Wi-Fi, OTA-slot erase, download or boot-partition changes.\n    fail_install(UT("Canal de mise a jour ROZA non configure", "ROZA update channel is not configured", "Canal de actualizacion ROZA no configurado", "Canal de mise a jour ROZA non configure", "ROZA-Update-Kanal nicht konfiguriert"), 0);\n    char want[65];'),
        ('static void check_task(void *arg)\n{\n    const bool from_boot = (bool)(uintptr_t)arg;\n\n    if (from_boot) {',
         'static void check_task(void *arg)\n{\n    const bool from_boot = (bool)(uintptr_t)arg;\n\n    // Keep the Store refresh path, but never query the separate NucleoOS release feed.\n    if (!from_boot)\n        st_set_phase(UPD_CHECK_FAIL, UT("Canal de mise a jour ROZA non configure", "ROZA update channel is not configured", "Canal de actualizacion ROZA no configurado", "Canal de mise a jour ROZA non configure", "ROZA-Update-Kanal nicht konfiguriert"));\n    else\n        st_set_phase(UPD_IDLE, NULL);\n    goto out_silent;\n\n    if (from_boot) {'),
        ('void nucleo_update_arm_boot_install(void)\n{\n    if (!nvs_ready()) return;\n    nvs_set_u8(s_nvs, "install", 1);\n    nvs_commit(s_nvs);\n}',
         'void nucleo_update_arm_boot_install(void)\n{\n    // ROZA updates are not published yet; never persist an upstream NucleoOS install request.\n}'),
        ('bool nucleo_update_boot_install_armed(void)\n{\n    if (!nvs_ready()) return false;\n    uint8_t v = 0;\n    return nvs_get_u8(s_nvs, "install", &v) == ESP_OK && v == 1;\n}',
         'bool nucleo_update_boot_install_armed(void)\n{\n    // Consume any stale flag left by an earlier NucleoOS build; it must not trigger a ROZA boot install.\n    if (nvs_ready() && nvs_erase_key(s_nvs, "install") == ESP_OK) nvs_commit(s_nvs);\n    return false;\n}'),
    ])
    updates_ui_changed = replace_exact(UPDATE_UI_FILE, [
        ('const char *row1 = PT("Controlla ora", "Check now", "Comprobar ahora", "Verifier maintenant", "Jetzt pruefen");',
         'const char *row1 = PT("Canale ROZA in preparazione", "ROZA update channel pending", "Canal ROZA en preparacion", "Canal ROZA en preparation", "ROZA-Update-Kanal in Vorbereitung");'),
        ('        nucleo_app_set_hint(PT("ENTER controlla  ESC esci", "ENTER check  ESC exit",\n                               "ENTER comprobar  ESC salir", "ENTER verifier  ESC quitter",\n                               "ENTER pruefen  ESC beenden"));',
         '        nucleo_app_set_hint(PT("Canale ROZA non configurato", "ROZA update channel not configured",\n                               "Canal ROZA no configurado", "Canal ROZA non configure",\n                               "ROZA-Update-Kanal nicht konfiguriert"));'),
        ('        "Check and install NucleoOS releases",',
         '        "ROZA updates (signed release channel pending)",'),
        ('            center("indecenti.github.io/NucleoOs", 68, 1, FG);',
         '            center(PT("Canale ROZA non disponible", "ROZA channel unavailable", "Canal ROZA no disponible", "Canal ROZA indisponible", "ROZA-Kanal nicht verfuegbar"), 68, 1, FG);'),
        ('            d.setTextColor(C_BLUE, cb); char u[40]; lg::fit(u, sizeof u, "indecenti.github.io/NucleoOs", w); d.drawString(u, x, y + 28);',
         '            d.setTextColor(MUTED, cb); d.drawString(PT("Canal ROZA non disponible", "ROZA channel unavailable", "Canal ROZA no disponible", "Canal ROZA indisponible", "ROZA-Kanal nicht verfuegbar"), x, y + 28);'),
    ])
    verify_roza_update_gate()
    print(
        "Surcouche ROZA appliquee — "
        f"theme={'modifie' if theme_changed else 'deja present'}, "
        f"splash={'modifie' if ui_changed else 'deja present'}, "
        f"apps ROZA={'modifiees' if main_changed else 'deja presentes'}, "
        f"verrou OTA={'modifie' if updater_changed or updates_ui_changed else 'deja present'}."
    )
    print("Langue francaise par defaut appliquee ; les choix deja enregistres sont conserves.")
    print("ROZA ajoute les inspecteurs Partitions et Firmware SD sans remplacer les apps NucleoOS.")
    print(
        "Mise a jour NucleoOS amont bloquee dans ROZA : aucun controle reseau ni flash OTA "
        "avant publication d'un canal ROZA signe."
    )
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
