#!/usr/bin/env python3
"""Apply the ROZA CHANINA overlay to the pinned upstream Launcher 3.0.0 source."""

from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
LAUNCHER = ROOT / "launcher"
PINNED_LAUNCHER = "4b06de691df15b0fe6a9f09f56d1d107bbf32b12"

LOGO_HELPER = r"""
// ROZA bitmap wordmark: Cyrillic Р О З А, independent of font glyph coverage.
static void drawRozaWordmark(int x, int y, uint16_t color, uint8_t scale = 3) {
    static const uint8_t glyphs[4][7] = {
        {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}, // Р
        {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}, // О
        {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110}, // З
        {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}  // А
    };
    if (scale < 1) scale = 1;
    for (int glyph = 0; glyph < 4; ++glyph) {
        for (int row = 0; row < 7; ++row) {
            for (int col = 0; col < 5; ++col) {
                if (glyphs[glyph][row] & (1U << (4 - col))) {
                    tft->fillRect(x + (glyph * 6 + col) * scale,
                                  y + row * scale, scale, scale, color);
                }
            }
        }
    }
}
"""

PATCHES = {
    "src/display.cpp": [
        ("tft_display *tft = new tft_display();",
         "tft_display *tft = new tft_display();\n\n" + LOGO_HELPER.strip()),
        (
            'const String title = "Launcher";\n    const int32_t titleX = (tftWidth - tft->textWidth(title, 1)) / 2;\n\n    drawOutlinedText(title, titleX, tftHeight / 2 - 10, FGCOLOR, BGCOLOR, 3);',
            "drawRozaWordmark((tftWidth - 23 * 3) / 2, tftHeight / 2 - 10, FGCOLOR, 3);",
        ),
        (
            'tft->drawCentreString("-=Launcher=-", tftWidth / 2, panelHeight() > 200 ? 20 : 10, 1);',
            'tft->drawCentreString("ROZA / CARNET", tftWidth / 2, panelHeight() > 200 ? 20 : 10, 1);',
        ),
        ('"No options available"', '"Aucune option disponible"'),
        (
            '        // Draw Launcher version and battery value\n        // Short panels have no room for the version next to the battery gauge.\n        if (panelHeight() < 200) tft->drawString("Launcher", 12 + RES, 12);\n        else tft->drawString("Launcher " + String(LAUNCHER), 12 + RES, 12);',
            '        // ROZA CHANINA identity: compact Cyrillic bitmap plus notebook label.\n        drawRozaWordmark(12 + RES, 8, ALCOLOR, 1);\n        tft->setTextColor(FGCOLOR, BGCOLOR);\n        tft->drawString("CARNET", 42 + RES, 12, 1);'
        ),
        (
            '        uint16_t itemColor = opt[i].active ? opt[i].color : DARKGREY;\n        uint16_t selectedColor = opt[i].active ? opt[i].color : LIGHTGREY;',
            '        uint16_t itemColor = opt[i].active ? ALCOLOR : DARKGREY;\n        uint16_t selectedColor = opt[i].active ? ALCOLOR : LIGHTGREY;'
        ),
    ],
    "src/main.cpp": [
        ('tiny ? "Launch from SDCard" : "Launch from or mng SDCard"',
         'tiny ? "Ouvrir carte SD" : "Fichiers et applis SD"'),
        ('{"OTA", "Online Installer",',
         '{"OTA", "Installer en ligne",'),
        ('tiny ? "Start WebUI" : "Start Web User Interface"',
         '"Interface Web ROZA"'),
        ('tiny ? "SD->USB" : "SD->USB Interface"',
         'tiny ? "SD vers USB" : "Stockage microSD en USB"'),
        ('"Partition Manager."',
         '"Gestion des partitions"'),
        ('{tiny ? "PM" : "PMan", "Partition Manager.",',
         '{"PM", "Gestion partitions",'),
        ('tiny ? "Change Settings." : "Change Launcher Settings."',
         '"Reglages ROZA"'),
    ],
    "src/settings.cpp": [
        (
            '{"Default",\n         [&]() {\n             FGCOLOR = 0x07E0;\n             BGCOLOR = 0x0000;\n             ALCOLOR = 0xF800;\n             odd_color = 0x30c5;\n             even_color = 0x32e5;\n         }, _CLR_(FGCOLOR, 0x07E0)},',
            '{"ROZA - Papier",\n         [&]() {\n             FGCOLOR = 0x2903;\n             BGCOLOR = 0xF75A;\n             ALCOLOR = 0x7947;\n             odd_color = 0xE6D8;\n             even_color = 0xFF9C;\n         }, _CLR_(FGCOLOR, 0x2903)},',
        ),
        (
            '    FGCOLOR = 0x07E0;\n    BGCOLOR = 0x0000;\n    ALCOLOR = 0xF800;\n    odd_color = 0x30c5;\n    even_color = 0x32e5;',
            '    FGCOLOR = 0x2903;\n    BGCOLOR = 0xF75A;\n    ALCOLOR = 0x7947;\n    odd_color = 0xE6D8;\n    even_color = 0xFF9C;',
        ),
        ('    autoConnect = true;', '    autoConnect = false;'),
        (
            '    nvs_get_u16(h, "even_color", &even_color);',
            '    nvs_get_u16(h, "even_color", &even_color);\n    // Migrate only the untouched upstream default palette; preserve custom themes.\n    if (FGCOLOR == 0x07E0 && BGCOLOR == 0x0000 && ALCOLOR == 0xF800 &&\n        odd_color == 0x30c5 && even_color == 0x32e5) {\n        FGCOLOR = 0x2903;\n        BGCOLOR = 0xF75A;\n        ALCOLOR = 0x7947;\n        odd_color = 0xE6D8;\n        even_color = 0xFF9C;\n        launcherConsolePrintln("ROZA: palette papier et bordeaux activee.");\n    }'
        ),
        (
            '    int count = applySettingsFromRoot(setting);',
            '    int count = applySettingsFromRoot(setting);\n    // Apply the ROZA palette to untouched upstream defaults from SD config too.\n    if (FGCOLOR == 0x07E0 && BGCOLOR == 0x0000 && ALCOLOR == 0xF800 &&\n        odd_color == 0x30c5 && even_color == 0x32e5) {\n        FGCOLOR = 0x2903;\n        BGCOLOR = 0xF75A;\n        ALCOLOR = 0x7947;\n        odd_color = 0xE6D8;\n        even_color = 0xFF9C;\n        count++;\n    }'
        ),
    ],
    "src/onlineLauncher.cpp": [
        ('keyboard(pwd, 63, "Network Password:")',
         'keyboard(pwd, 63, "Mot de passe Wi-Fi :")'),
        ('launcherConsolePrintf("wifiConnect: ssid->%s, pwd->%s\\n", ssid.c_str(), pwd.c_str());',
         'launcherConsolePrintf("wifiConnect: SSID %s (mot de passe masque)\\n", ssid.c_str());'),
        ('tftprint("Connecting to: " + ssid + ".", 10);',
         'tftprint("Connexion a : " + ssid + ".", 10);'),
        ('displayError("Wrong Password");',
         'displayError("Mot de passe incorrect");'),
        ('displayRedStripe("Scanning...");',
         'displayRedStripe("Recherche des reseaux Wi-Fi...");'),
        ('displayError("WiFi scan failed");',
         'displayError("Scan Wi-Fi impossible");'),
    ],
    "webUi/index.html": [
        ('<html lang="en">', '<html lang="fr">'),
        ('<h1>&#11835; LAUNCHER &#11835;</h1>', '<h1>✦ РОЗА ✦</h1>'),
        ('<h3>Installed Firmwares</h3>', '<h3>Firmwares installes</h3>'),
        ('&#9881; Config', '&#9881; Reglages ROZA'),
        ('&#8593; OTA', '&#8593; Mise a jour'),
        ('&#128193; Files', '&#128193; Fichiers'),
        ('&#128202; Partitions', '&#128202; Partitions'),
        ('Firmware list &#8599;', 'Catalogue des firmwares &#8599;'),
        ('NVS Config', 'Parametres internes (NVS)'),
        ('&#128279; Erase BLE Bonds', '&#128279; Effacer les associations BLE'),
        ('<h3>Uploading</h3>', '<h3>Televersement en cours</h3>'),
        ('<span class="mtitle">Config</span>', '<span class="mtitle">Parametres</span>'),
        ('&#8635; Reboot', '&#8635; Redemarrer'),
        ('SD Pins', 'Broches microSD'),
        ('WiFi', 'Wi-Fi'),
        ('User/Pass', 'Compte'),
        ('&#10006; Logout', '&#10006; Deconnexion'),
        ('class="mtitle">Resize</span>', 'class="mtitle">Redimensionner</span>'),
        ('&#128190; Apply', '&#128190; Appliquer'),
    ],
    "webUi/login.html": [
        ('<html lang="en">', '<html lang="fr">'),
        ('<title>Launcher Login</title>', '<title>ROZA — Connexion</title>'),
        (
            'body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;background:#202124;color:#00dd00;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif}\n.login-card{background:#303134;border:1px solid #00dd00;border-radius:8px;box-shadow:0 0 12px #000;max-width:320px;width:100%;padding:24px;}\n.login-card h1{margin:0 0 16px;text-align:center;font-size:22px;}\n.login-card label{display:block;margin:10px 0 4px;color:#e0d204;font-size:14px;}\n.login-card input{width:100%;padding:8px;border:1px solid #00dd00;border-radius:4px;background:#202124;color:#00dd00;}\n.login-card button{width:100%;margin-top:18px;padding:10px;}\n#notice{display:none;margin-bottom:12px;padding:8px;border:1px solid #e57373;border-radius:4px;color:#e57373;text-align:center;font-weight:600;}',
            'body{margin:0;min-height:100vh;display:flex;align-items:center;justify-content:center;background:var(--bg);color:var(--fg);font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif}\n.login-card{background:var(--s1);border:1px solid var(--br);border-radius:10px;box-shadow:0 8px 24px rgba(48,32,24,.14);max-width:320px;width:100%;padding:24px;}\n.login-card h1{margin:0 0 16px;text-align:center;font-size:22px;color:var(--ac);letter-spacing:3px}\n.login-card label{display:block;margin:10px 0 4px;color:var(--ac);font-size:14px;}\n.login-card input{width:100%;padding:8px;border:1px solid var(--br);border-radius:4px;background:var(--bg);color:var(--fg);}\n.login-card button{width:100%;margin-top:18px;padding:10px;background:var(--ac);color:#fff}\n#notice{display:none;margin-bottom:12px;padding:8px;border:1px solid var(--ac);border-radius:4px;color:var(--ac);text-align:center;font-weight:600;}',
        ),
        ('-= Launcher =-', '✦ РОЗА ✦'),
        ('for="username">Username</label>', 'for="username">Identifiant</label>'),
        ('for="password">Password</label>', 'for="password">Mot de passe</label>'),
        ('>Login</button>', '>Se connecter</button>'),
        ('Usuario ou senha invalidos.', 'Identifiant ou mot de passe invalide.'),
        ('Sessao encerrada com sucesso.', 'Session fermee.'),
    ],
    "webUi/style.css": [
        (
            ':root{--bg:#0f0f0f;--s1:#181818;--s2:#222;--br:#303030;--fg:#d4d4d4;--dim:#888;--ac:#3ddc84;--yw:#e0d204;--r:6px}',
            ':root{--bg:#eee5d2;--s1:#f8f1e1;--s2:#e5d9c1;--br:#cabb9e;--fg:#302522;--dim:#75665a;--ac:#7b2439;--yw:#b98b4c;--r:8px}',
        ),
    ],
}

def replace_once(relative: str, old: str, new: str) -> int:
    path = LAUNCHER / relative
    if not path.is_file():
        raise SystemExit(f"Fichier Launcher absent : {path}")
    source = path.read_text(encoding="utf-8")
    old_count = source.count(old)
    new_count = source.count(new)
    if old_count == 1:
        path.write_text(source.replace(old, new, 1), encoding="utf-8")
        return 1
    if old_count == 0 and new_count == 1:
        return 0
    raise SystemExit(
        f"Patch refuse pour {relative}: source={old_count}, deja-applique={new_count}; revue requise."
    )

def apply_wifi_picker() -> int:
    """Show the available Wi-Fi networks first, then connect only after selection."""
    path = LAUNCHER / "src/onlineLauncher.cpp"
    source = path.read_text(encoding="utf-8")
    start_marker = "bool connectWifi() {"
    end_marker = "\nbool ensureWifiConnected("
    if source.count(start_marker) != 1 or source.count(end_marker) != 1:
        raise SystemExit("Fonction connectWifi introuvable ou ambigue; revue requise.")
    start = source.index(start_marker)
    end = source.index(end_marker, start)
    current = source[start:end]
    if "ROZA-WIFI-PICKER-V3" in current:
        return 0
    if "launcherWifiScan(networks)" not in current or "if (!autoConnect) continue;" not in current:
        raise SystemExit("Source Wi-Fi Launcher 3.0.0 inattendue; patch refuse.")

    replacement = r"""
// ROZA-WIFI-PICKER-V3: list networks before attempting any connection.
bool connectWifi() {
    RAM_LOG("connectWifi-start");
    returnToMenu = false;
    displayRedStripe("Recherche des reseaux Wi-Fi...");
#if CONFIG_ESP_HOSTED_ENABLED
    launcherWifiStop();
#endif
    std::vector<LauncherWifiAp> networks;
    const int nets = launcherWifiScan(networks);
    if (nets < 0) {
        displayError("Scan Wi-Fi impossible");
        return false;
    }

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

    // Saved-network auto-connect is available only as a deliberate menu choice.
    if (autoConnect) {
        bool hasSavedNetwork = false;
        for (const LauncherWifiAp &ap : uniqueNetworks) {
            String candidate = ap.ssid.c_str();
            String savedPassword;
            if (getWifiCredential(candidate, savedPassword)) {
                hasSavedNetwork = true;
                break;
            }
        }
        if (hasSavedNetwork) {
            options.push_back({"Connexion auto (reseaux enregistres)", [&]() {
                for (const LauncherWifiAp &ap : uniqueNetworks) {
                    String candidate = ap.ssid.c_str();
                    String savedPassword;
                    if (!getWifiCredential(candidate, savedPassword)) continue;
                    launcherConsolePrintf("Wi-Fi: connexion sur %s (mot de passe masque)\n", candidate.c_str());
                    if (wifiConnect(candidate, static_cast<int>(ap.authmode))) break;
                    if (returnToMenu) break;
                }
            }});
        }
    }

    for (const LauncherWifiAp &ap : uniqueNetworks) {
        String networkSsid = ap.ssid.c_str();
        const int authMode = static_cast<int>(ap.authmode);
        String label = networkSsid;
        String savedPassword;
        if (getWifiCredential(networkSsid, savedPassword)) label += " [enregistre]";
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
    path.write_text(source[:start] + replacement.strip() + "\n" + source[end:], encoding="utf-8")
    return 1

def main() -> int:
    if not LAUNCHER.is_dir() or not (LAUNCHER / "platformio.ini").is_file():
        print("Sous-module launcher absent. Clone avec --recurse-submodules.", file=sys.stderr)
        return 2

    actual = subprocess.check_output(
        ["git", "-C", str(LAUNCHER), "rev-parse", "HEAD"], text=True
    ).strip()
    if actual != PINNED_LAUNCHER:
        raise SystemExit(
            f"Launcher non epingle sur la version attendue: {actual} (attendu {PINNED_LAUNCHER})"
        )

    total = 0
    for relative, replacements in PATCHES.items():
        changed = 0
        for old, new in replacements:
            changed += replace_once(relative, old, new)
        print(f"{relative}: {changed} personnalisation(s)")
        total += changed

    wifi_changes = apply_wifi_picker()
    total += wifi_changes
    print(f"Parcours Wi-Fi ROZA: {wifi_changes} personnalisation(s)")
    print("Identite ROZA CHANINA appliquee." if total else "Identite ROZA CHANINA deja appliquee.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
