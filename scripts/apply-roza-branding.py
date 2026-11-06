#!/usr/bin/env python3
"""Apply the reviewed ROZA CHANINA overlay to the pinned CardputerADV-Launcher source."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
LAUNCHER = ROOT / "launcher"

PATCHES = {
    "src/display.cpp": [
        ('#define HUD_BG 0x0140 // dark green', "static void drawRozaWordmark(int x, int y, uint16_t fg, uint16_t bg, uint8_t scale = 1) {\n    (void)bg;\n    if (scale < 1) scale = 1;\n    // Hand-drawn 5x7 Cyrillic capitals: Р О З А.\n    static const uint8_t glyphs[4][7] = {\n        {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}, // Р\n        {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}, // О\n        {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110}, // З\n        {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}  // А\n    };\n    for (int g = 0; g < 4; ++g) {\n        for (int row = 0; row < 7; ++row) {\n            for (int col = 0; col < 5; ++col) {\n                if (glyphs[g][row] & (1U << (4 - col))) {\n                    tft->fillRect(x + (g * 6 + col) * scale, y + row * scale, scale, scale, fg);\n                }\n            }\n        }\n    }\n}\n\n#define HUD_BG 0xDEB2 // ROZA paper edge"),
        ('tft->drawString("AMASETTI", 4, 3, 1);', 'drawRozaWordmark(4, 3, FGCOLOR, HUD_BG, 1);'),
        ('const String title = "Launcher";\n    const int32_t titleX = (tftWidth - tft->textWidth(title, 1)) / 2;\n    drawOutlinedText(title, titleX, tftHeight / 2 - 10, FGCOLOR, BGCOLOR, 3);',
         'drawRozaWordmark((tftWidth - 23 * _fg) / 2, tftHeight / 2 - 10, FGCOLOR, BGCOLOR, _fg);'),
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
        ('bool autoConnect = true;', 'bool autoConnect = false;'),
    ],
    "src/settings.cpp": [
        ('{"Default",\n         [&]() {\n             FGCOLOR = 0x07E0;\n             BGCOLOR = 0x0000;\n             ALCOLOR = 0xF800;\n             odd_color = 0x30c5;\n             even_color = 0x32e5;\n         }, _CLR_(FGCOLOR, 0x07E0)},',
         '{"PO3A",\n         [&]() {\n             FGCOLOR = 0x2104;\n             BGCOLOR = 0xEEF8;\n             ALCOLOR = 0xA145;\n             odd_color = 0xE6D4;\n             even_color = 0xDEB2;\n         }, _CLR_(FGCOLOR, 0x2104)},'),
        ('    FGCOLOR = 0x07E0;\n    BGCOLOR = 0x0000;\n    ALCOLOR = 0xF800;\n    odd_color = 0x30c5;\n    even_color = 0x32e5;',
         '    FGCOLOR = 0x2104;\n    BGCOLOR = 0xEEF8;\n    ALCOLOR = 0xA145;\n    odd_color = 0xE6D4;\n    even_color = 0xDEB2;'),
        ('    nvs_get_u16(h, "even_color", &even_color);\n    lnvs::getBool(h, "dev_mode", dev_mode);',
         '    nvs_get_u16(h, "even_color", &even_color);\n    String rozaTheme = lnvs::getString(h, "roza_theme", 24);\n    if (rozaTheme != "po3a-paper-v1") {\n        FGCOLOR = 0x2104;\n        BGCOLOR = 0xEEF8;\n        ALCOLOR = 0xA145;\n        odd_color = 0xE6D4;\n        even_color = 0xDEB2;\n        lnvs::Handle themeStore("launcher", true);\n        if (themeStore) {\n            nvs_handle_t themeHandle = themeStore.raw();\n            nvs_set_u16(themeHandle, "FGCOLOR", FGCOLOR);\n            nvs_set_u16(themeHandle, "BGCOLOR", BGCOLOR);\n            nvs_set_u16(themeHandle, "ALCOLOR", ALCOLOR);\n            nvs_set_u16(themeHandle, "odd_color", odd_color);\n            nvs_set_u16(themeHandle, "even_color", even_color);\n            lnvs::setString(themeHandle, "roza_theme", "po3a-paper-v1");\n            themeStore.commit();\n        }\n    }\n    lnvs::getBool(h, "dev_mode", dev_mode);'),
        ('    JsonObject setting = settings[0];\n    int count = applySettingsFromRoot(setting);\n    getWifiFromNVS(true);',
         '    JsonObject setting = settings[0];\n    int count = applySettingsFromRoot(setting);\n    if (setting["roza_theme"] != "po3a-paper-v1") {\n        FGCOLOR = 0x2104;\n        BGCOLOR = 0xEEF8;\n        ALCOLOR = 0xA145;\n        odd_color = 0xE6D4;\n        even_color = 0xDEB2;\n        setting["roza_theme"] = "po3a-paper-v1";\n        ++count;\n    }\n    getWifiFromNVS(true);'),
        ('    dev_mode = false;\n    autoConnect = true;', '    dev_mode = false;\n    autoConnect = false;'),
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
    "src/onlineLauncher.cpp": [
        ('keyboard(pwd, 63, "Network Password:")', 'keyboard(pwd, 63, "Mot de passe Wi-Fi :")'),
        ('launcherConsolePrintf("wifiConnect: ssid->%s, pwd->%s\\n", ssid.c_str(), pwd.c_str());',
         'launcherConsolePrintf("wifiConnect: saved SSID %s (password hidden)\\n", ssid.c_str());'),
        ('tftprint("Connecting to: " + ssid + ".", 10);', 'tftprint("Connexion a : " + ssid + ".", 10);'),
        ('displayError("Wrong Password");', 'displayError("Mot de passe incorrect");'),
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

def apply_wifi_picker() -> int:
    """Show scanned access points first; saved-network auto-connect is an explicit choice."""
    path = LAUNCHER / "src/onlineLauncher.cpp"
    if not path.is_file():
        raise SystemExit(f"Fichier Launcher absent : {path}")
    source = path.read_text(encoding="utf-8")
    start_marker = "bool connectWifi() {"
    end_marker = "\nbool ensureWifiConnected("
    if source.count(start_marker) != 1 or source.count(end_marker) != 1:
        raise SystemExit("Fonction connectWifi introuvable ou ambigue; revue requise.")
    start = source.index(start_marker)
    end = source.index(end_marker, start)
    current = source[start:end]
    if "ROZA-WIFI-PICKER-V1" in current:
        return 0
    if "launcherWifiScan(networks)" not in current or "if (!autoConnect) continue;" not in current:
        raise SystemExit("Source Wi-Fi inattendue; refus du patch automatique.")

    replacement = r"""
// ROZA-WIFI-PICKER-V1: user chooses an access point before any connection attempt.
bool connectWifi() {
    RAM_LOG("connectWifi-start");
    returnToMenu = false;
    displayRedStripe("Recherche des reseaux Wi-Fi...");
    std::vector<LauncherWifiAp> networks;
    int nets = launcherWifiScan(networks);
    if (nets < 0) {
        displayError("Echec du scan Wi-Fi");
        return false;
    }

    // Strongest access point first; show one entry per SSID.
    std::sort(networks.begin(), networks.end(), [](const LauncherWifiAp &a, const LauncherWifiAp &b) {
        return a.rssi > b.rssi;
    });
    std::vector<LauncherWifiAp> uniqueNetworks;
    for (const LauncherWifiAp &ap : networks) {
        if (ap.ssid.empty()) continue;
        bool seen = false;
        for (const LauncherWifiAp &existing : uniqueNetworks) {
            if (existing.ssid == ap.ssid) {
                seen = true;
                break;
            }
        }
        if (!seen) uniqueNetworks.push_back(ap);
    }

    options.clear();

    // Keep automatic connection available as an explicit action only.
    bool hasVisibleSavedNetwork = false;
    if (autoConnect) {
        for (const LauncherWifiAp &ap : uniqueNetworks) {
            String candidate = ap.ssid.c_str();
            String knownPwd;
            if (getWifiCredential(candidate, knownPwd)) {
                hasVisibleSavedNetwork = true;
                break;
            }
        }
    }
    if (hasVisibleSavedNetwork) {
        options.push_back({"Connexion auto (reseaux enregistres)", [&]() {
            for (const LauncherWifiAp &ap : uniqueNetworks) {
                String candidate = ap.ssid.c_str();
                String knownPwd;
                if (!getWifiCredential(candidate, knownPwd)) continue;
                launcherConsolePrintf("Wi-Fi: tentative automatique sur %s\n", candidate.c_str());
                if (wifiConnect(candidate, static_cast<int>(ap.authmode))) break;
                if (returnToMenu) break;
            }
        }});
    }

    for (const LauncherWifiAp &ap : uniqueNetworks) {
        String networkSsid = ap.ssid.c_str();
        int authMode = static_cast<int>(ap.authmode);
        String label = networkSsid;
        String knownPwd;
        if (getWifiCredential(networkSsid, knownPwd)) label += " [enregistre]";
        label += " (" + String(static_cast<int>(ap.rssi)) + " dBm)";
        options.push_back({label, [=]() { wifiConnect(networkSsid, authMode); }});
    }

    options.push_back({"Reseau masque", [=]() {
        String hiddenSsid = keyboard("", 32, "Nom du reseau Wi-Fi :");
        if (hiddenSsid != String(KEY_ESCAPE)) wifiConnect(hiddenSsid, 8);
    }});
    options.push_back({"Menu principal", [=]() { returnToMenu = true; }});

    if (uniqueNetworks.empty()) displayError("Aucun reseau Wi-Fi detecte", false);
    loopOptions(options);
    return launcherWifiIsConnected();
}
"""
    source = source[:start] + replacement.strip() + "\n" + source[end:]
    path.write_text(source, encoding="utf-8")
    return 1

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
    menu_changes = apply_main_menu()
    total += menu_changes
    wifi_changes = apply_wifi_picker()
    total += wifi_changes
    print(f"Parcours Wi-Fi ROZA: {wifi_changes} personnalisation(s)")
    print("Identite ROZA deja appliquee." if total == 0 else f"Identite ROZA appliquee ({total} modifications).")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
