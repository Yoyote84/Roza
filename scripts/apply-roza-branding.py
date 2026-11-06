#!/usr/bin/env python3
"""Apply the reviewed ROZA CHANINA overlay to the pinned CardputerADV-Launcher source."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LAUNCHER = ROOT / "launcher"

PATCHES = {
    "src/display.cpp": [
        ('#define HUD_BG 0x0140 // dark green', '#define HUD_BG 0x31A5 // ROZA ink'),
        ('tft->drawString("AMASETTI", 4, 3, 1);', 'tft->drawString("ROZA", 4, 3, 1);'),
        ('const String title = "Launcher";', 'const String title = "ROZA CHANINA";'),
        ('tft->drawCentreString("M5Stack Cardputer ADV", tftWidth / 2, tftHeight / 2 + 22, 1);',
         'tft->drawCentreString("INSTRUMENT PERSONNEL", tftWidth / 2, tftHeight / 2 + 22, 1);'),
        ('tft->drawCentreString("-=Launcher=-", tftWidth / 2, panelHeight() > 200 ? 20 : 10, 1);',
         'tft->drawCentreString("-- ROZA CHANINA --", tftWidth / 2, panelHeight() > 200 ? 20 : 10, 1);'),
        ('"; . move   enter ok   \x60 back"', '"; . nav  enter OK  \x60 retour"'),
        ('drawListFrame(index, count, start, BGCOLOR, "Launcher");',
         'drawListFrame(index, count, start, BGCOLOR, "ROZA CHANINA");'),
        ('drawHud("Launcher");', 'drawHud("ROZA CHANINA");'),
        ('"No options available"', '"Aucune option disponible"'),
    ],
    "src/main.cpp": [
        ('tiny ? "Launch from SDCard" : "Launch from or mng SDCard"',
         'tiny ? "Fichiers microSD" : "Gerer les fichiers microSD"'),
        ('"Online Installer"', '"Installation en ligne"'),
        ('"Cardputer Apps"', '"Applications ROZA"'),
        ('tiny ? "Start WebUI" : "Start Web User Interface"',
         'tiny ? "Interface Web" : "Administration Web"'),
        ('tiny ? "SD->USB" : "SD->USB Interface"',
         'tiny ? "Stockage USB" : "Acces USB a la microSD"'),
        ('"Partition Manager."', '"Gestion des partitions"'),
        ('tiny ? "Change Settings." : "Change Launcher Settings."',
         'tiny ? "Reglages" : "Parametres ROZA"'),
    ],
    "src/settings.cpp": [
        ('{"Default",\n         [&]() {\n             FGCOLOR = 0x07E0;\n             BGCOLOR = 0x0000;\n             ALCOLOR = 0xF800;\n             odd_color = 0x30c5;\n             even_color = 0x32e5;\n         }, _CLR_(FGCOLOR, 0x07E0)},',
         '{"ROZA",\n         [&]() {\n             FGCOLOR = 0xEEF8;\n             BGCOLOR = 0x31A5;\n             ALCOLOR = 0x6388;\n             odd_color = 0x4244;\n             even_color = 0x5B29;\n         }, _CLR_(FGCOLOR, 0xEEF8)},'),
        ('    FGCOLOR = 0x07E0;\n    BGCOLOR = 0x0000;\n    ALCOLOR = 0xF800;\n    odd_color = 0x30c5;\n    even_color = 0x32e5;',
         '    FGCOLOR = 0xEEF8;\n    BGCOLOR = 0x31A5;\n    ALCOLOR = 0x6388;\n    odd_color = 0x4244;\n    even_color = 0x5B29;'),
    ],
    "src/appStore.cpp": [
        ('displayRedStripe("Getting app list");', 'displayRedStripe("Lecture du catalogue ROZA");'),
        ('displayError("Repo not found (private?)");', 'displayError("Depot introuvable ou prive");'),
        ('displayError("GitHub rate limit, retry later");', 'displayError("Limite GitHub atteinte, reessayer plus tard");'),
        ('displayError("No apps in this repo");', 'displayError("Aucune application dans ce depot");'),
        ('displayRedStripe("Downloading " + app.name);', 'displayRedStripe("Telechargement " + app.name);'),
        ('{"Add repository", [&]() {', '{"Ajouter un depot", [&]() {'),
        ('{"Remove repository", [&]() {', '{"Retirer un depot", [&]() {'),
        ('{"Main Menu", [&]() { done = true; }}', '{"Menu principal", [&]() { done = true; }}'),
        ('{"Cancel",                         [&]() {}            },', '{"Annuler",                        [&]() {}            },'),
        ('{"Install v" + versionStr(chosen),', '{"Installer v" + versionStr(chosen),'),
        ('{"Back", [&]() { back = true; }}', '{"Retour", [&]() { back = true; }}'),
        ('displayError("Use owner/repo");', 'displayError("Format attendu : proprietaire/depot");'),
    ],
}

def apply_file(relative: str, replacements: list[tuple[str, str]]) -> int:
    path = LAUNCHER / relative
    if not path.is_file():
        raise SystemExit(f"Fichier Launcher absent : {path}")
    text = path.read_text(encoding="utf-8")
    changed = 0
    for old, new in replacements:
        count_old = text.count(old)
        count_new = text.count(new)
        if count_old == 1:
            text = text.replace(old, new, 1)
            changed += 1
        elif count_old == 0 and count_new == 1:
            continue
        else:
            raise SystemExit(
                f"Patch non applique pour {relative}: "
                f"occurrences source={count_old}, deja-patche={count_new}; revue requise."
            )
    if changed:
        path.write_text(text, encoding="utf-8")
    return changed

def main() -> int:
    if not LAUNCHER.is_dir() or not (LAUNCHER / "platformio.ini").is_file():
        print("Sous-module launcher absent. Clone avec --recurse-submodules.", file=sys.stderr)
        return 2
    total = 0
    for relative, replacements in PATCHES.items():
        count = apply_file(relative, replacements)
        print(f"{relative}: {count} personnalisation(s)")
        total += count
    print("Identite ROZA deja appliquee." if total == 0 else f"ROZA CHANINA applique ({total} modifications).")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
