#!/usr/bin/env python3
"""Apply the reviewed ROZA CHANINA overlay to the pinned CardputerADV-Launcher source."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LAUNCHER = ROOT / "launcher"

PATCHES = {
    "src/display.cpp": [
        ('#define HUD_BG 0x0140 // dark green', '#define HUD_BG 0xDEB2 // ROZA paper edge'),
        ('tft->drawString("AMASETTI", 4, 3, 1);', 'tft->drawString("PO3A", 4, 3, 1);'),
        ('const String title = "Launcher";', 'const String title = "PO3A";'),
        ('tft->drawCentreString("M5Stack Cardputer ADV", tftWidth / 2, tftHeight / 2 + 22, 1);',
         'tft->drawCentreString("CARNET DE MEMOIRE", tftWidth / 2, tftHeight / 2 + 22, 1);'),
        ('tft->drawCentreString("-=Launcher=-", tftWidth / 2, panelHeight() > 200 ? 20 : 10, 1);',
         'tft->drawCentreString("PO3A / CARNET", tftWidth / 2, panelHeight() > 200 ? 20 : 10, 1);'),
        ('"; . move   enter ok   \x60 back"', '"; . feuilleter  entrer ouvrir"'),
        ('drawListFrame(index, count, start, BGCOLOR, "Launcher");',
         'drawListFrame(index, count, start, BGCOLOR, "CARNET");'),
        ('drawHud("Launcher");', 'drawHud("CARNET");'),
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
         '{"PO3A",\n         [&]() {\n             FGCOLOR = 0x2104;\n             BGCOLOR = 0xEEF8;\n             ALCOLOR = 0xA145;\n             odd_color = 0xE6D4;\n             even_color = 0xDEB2;\n         }, _CLR_(FGCOLOR, 0x2104)},'),
        ('    FGCOLOR = 0x07E0;\n    BGCOLOR = 0x0000;\n    ALCOLOR = 0xF800;\n    odd_color = 0x30c5;\n    even_color = 0x32e5;',
         '    FGCOLOR = 0x2104;\n    BGCOLOR = 0xEEF8;\n    ALCOLOR = 0xA145;\n    odd_color = 0xE6D4;\n    even_color = 0xDEB2;'),
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

def apply_main_menu() -> int:
    path = LAUNCHER / "src/display.cpp"
    if not path.is_file():
        raise SystemExit(f"Fichier Launcher absent : {path}")
    source = path.read_text(encoding="utf-8")
    start_marker = "void drawMainMenu("
    end_marker = "/***************************************************************************************\n** Function name: launcherBootAppShortcuts"
    if source.count(start_marker) != 1 or source.count(end_marker) != 1:
        raise SystemExit("Zone du menu principal introuvable ou ambigue; revue requise.")
    start = source.index(start_marker)
    end = source.index(end_marker, start)
    current = source[start:end]
    if "PO3A-CARNET-MENU-V1" in current:
        return 0
    if 'drawHud("CARNET");' not in current or "drawListRow(i," not in current:
        raise SystemExit("Source du menu inattendue; refus du patch automatique.")

    replacement = r"""
// PO3A-CARNET-MENU-V1: archival index page, preserving Launcher actions and input.
void drawMainMenu(std::vector<MenuOptions> &opt, int index, bool forceFullRedraw) {
    static int lastIndex = -1;
    static int lastStart = -1;
    static int bat = -1;
    static bool wifi = false;
    static bool sd = false;

    const int count = static_cast<int>(opt.size());
    if (count < 1) {
        displayRedStripe("Aucune option disponible");
        return;
    }
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;

    const int rows = LIST_ROWS;
    const int start = listStart(index, count);
    const int visible = count - start < rows ? count - start : rows;
    const bool partial = !forceFullRedraw && start == lastStart && index != lastIndex && lastIndex >= 0;
    const int prev = lastIndex;
    lastIndex = index;
    lastStart = start;

    if (!partial) {
        drawListFrame(index, count, start, BGCOLOR, "CARNET");
        bat = getBattery();
        wifi = launcherWifiIsConnected();
        sd = sdcardMounted;

        // Notebook binding marks along the left edge.
        tft->drawLine(8, LIST_TOP, 8, tftHeight - LIST_FOOT_H - 2, ALCOLOR);
        for (int y = LIST_TOP + 5; y < tftHeight - LIST_FOOT_H - 2; y += 24) {
            tft->drawPixel(5, y, ALCOLOR);
        }
    } else {
        drawListPosition(index, count);
    }

    for (int i = 0; i < visible; ++i) {
        ALIVIATE_TASK;
        const int n = start + i;
        if (partial && n != index && n != prev) continue;

        const bool selected = n == index;
        const int y = LIST_TOP + i * LIST_ROW_H;
        const uint16_t rowBg = selected ? ALCOLOR : BGCOLOR;
        tft->fillRect(12, y, tftWidth - 20, LIST_ROW_H - 2, rowBg);
        if (!selected) {
            tft->drawLine(12, y + LIST_ROW_H - 2, tftWidth - 9, y + LIST_ROW_H - 2, 0xD6B2);
        }

        String number = String(n + 1);
        String name = opt[n].name.substring(0, 5);
        String desc = opt[n].text;
        const int descLimit = (tftWidth - 88) / LW;
        if (static_cast<int>(desc.length()) > descLimit) desc = desc.substring(0, descLimit);

        tft->setTextSize(1);
        tft->setTextColor(selected ? BGCOLOR : ALCOLOR, rowBg);
        tft->drawString(number, 17, y + 3, 1);
        tft->setTextColor(selected ? BGCOLOR : (opt[n].active ? FGCOLOR : DARKGREY), rowBg);
        tft->drawString(name, 36, y + 3, 1);
        tft->setTextColor(selected ? BGCOLOR : LIST_DIM(FGCOLOR), rowBg);
        tft->drawString(desc, 78, y + 3, 1);
    }

    const int currentBattery = getBattery();
    const bool currentWifi = launcherWifiIsConnected();
    if (bat != currentBattery || wifi != currentWifi || sd != sdcardMounted) {
        drawHud("CARNET");
        bat = currentBattery;
        wifi = currentWifi;
        sd = sdcardMounted;
    }

    tft->setTextSize(1);
    tft->display(false);
}
"""
    source = source[:start] + replacement.strip() + "\n" + source[end:]
    path.write_text(source, encoding="utf-8")
    return 1

def main() -> int:
    if not LAUNCHER.is_dir() or not (LAUNCHER / "platformio.ini").is_file():
        print("Sous-module launcher absent. Clone avec --recurse-submodules.", file=sys.stderr)
        return 2
    total = 0
    for relative, replacements in PATCHES.items():
        count = apply_file(relative, replacements)
        print(f"{relative}: {count} personnalisation(s)")
        total += count
    menu_changes = apply_main_menu()\n    total += menu_changes\n    print("Identite PO3A deja appliquee." if total == 0 else f"Identite PO3A appliquee ({total} modifications).")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
