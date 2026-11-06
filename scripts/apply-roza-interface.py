#!/usr/bin/env python3
"""Apply the ROZA paper-and-burgundy menu layout to the pinned Launcher source."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
DISPLAY = ROOT / "launcher" / "src" / "display.cpp"

START = "void drawMainMenu(std::vector<MenuOptions> &opt, int index, bool forceFullRedraw) {"
END = "\n/***************************************************************************************\n** Function name: drawStatusIcons"

REPLACEMENT = r'''// ROZA-INTERFACE-V1: compact 240x135 paper-and-burgundy list layout.
void drawMainMenu(std::vector<MenuOptions> &opt, int index, bool forceFullRedraw) {
    static int lastIndex = -1;
    static int lastFirst = -1;
    const int count = static_cast<int>(opt.size());
    if (count < 1) {
        displayRedStripe("Aucune option disponible");
        return;
    }
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;

    const int headerHeight = 18;
    const int firstRowY = 43;
    const int rowHeight = 13;
    const int footerHeight = 13;
    int visibleRows = (tftHeight - firstRowY - footerHeight) / rowHeight;
    if (visibleRows < 1) visibleRows = 1;
    if (visibleRows > count) visibleRows = count;

    int first = 0;
    if (index >= visibleRows) first = index - visibleRows + 1;

    const bool fullRedraw = forceFullRedraw || lastIndex < 0 || first != lastFirst;
    const int previous = lastIndex;
    lastIndex = index;
    lastFirst = first;

    if (fullRedraw) {
        tft->fillScreen(BGCOLOR);
        tft->fillRect(0, 0, tftWidth, headerHeight, ALCOLOR);
        drawRozaWordmark(6, 5, BGCOLOR, 1);
        tft->setTextSize(_fp);
        tft->setTextColor(BGCOLOR, ALCOLOR);
        tft->drawString("CARNET", 34, 5, 1);
        tft->setTextColor(FGCOLOR, BGCOLOR);
        tft->drawString("Accueil", 7, 27, 1);
        tft->drawLine(6, 37, tftWidth - 6, 37, 0xC6B5);
    }

    for (int i = first; i < first + visibleRows; ++i) {
        const int row = i - first;
        const int y = firstRowY + row * rowHeight;
        opt[i].x = 5;
        opt[i].y = y - 3;
        opt[i].w = tftWidth - 10;
        opt[i].h = 12;

        if (!fullRedraw && i != index && i != previous) continue;

        if (i == index) {
            tft->fillRect(5, y - 3, tftWidth - 10, 12, 0xE6D8);
            tft->fillRect(5, y - 3, 3, 12, ALCOLOR);
            tft->setTextColor(ALCOLOR, 0xE6D8);
        } else {
            tft->fillRect(5, y - 3, tftWidth - 10, 12, BGCOLOR);
            tft->setTextColor(FGCOLOR, BGCOLOR);
        }

        tft->setTextSize(_fp);
        String label = opt[i].text;
        if (label.isEmpty()) label = opt[i].name;
        const int maxChars = (tftWidth - 22) / (LW * _fp);
        if (maxChars > 0 && static_cast<int>(label.length()) > maxChars)
            label = label.substring(0, maxChars);
        tft->drawString(label, 11, y, 1);
    }

    // Clear rows left behind when scrolling or returning from a longer menu.
    if (!fullRedraw && previous >= first && previous < first + visibleRows &&
        previous != index) {
        const int oldY = firstRowY + (previous - first) * rowHeight;
        tft->fillRect(5, oldY - 3, tftWidth - 10, 12, BGCOLOR);
        tft->setTextColor(FGCOLOR, BGCOLOR);
        String oldLabel = opt[previous].text;
        if (oldLabel.isEmpty()) oldLabel = opt[previous].name;
        const int maxChars = (tftWidth - 22) / (LW * _fp);
        if (maxChars > 0 && static_cast<int>(oldLabel.length()) > maxChars)
            oldLabel = oldLabel.substring(0, maxChars);
        tft->drawString(oldLabel, 11, oldY, 1);
    }

    // Footer reports live device state; unknown battery readings are not fabricated.
    tft->fillRect(0, tftHeight - footerHeight, tftWidth, footerHeight, 0xE6D8);
    tft->setTextSize(_fp);
    tft->setTextColor(FGCOLOR, 0xE6D8);
    tft->drawString(launcherWifiIsConnected() ? "Wi-Fi" : "Hors ligne", 7, tftHeight - 10, 1);
    const int battery = getBattery();
    String batteryLabel = battery > 0 ? "BAT " + String(battery) + "%" : "BAT --";
    tft->drawRightString(batteryLabel, tftWidth - 7, tftHeight - 10, 1);

    tft->display(false);
}'''

def main() -> int:
    if not DISPLAY.is_file():
        print(f"Source Launcher absent : {DISPLAY}", file=sys.stderr)
        return 2

    source = DISPLAY.read_text(encoding="utf-8")
    if "ROZA-INTERFACE-V1" in source:
        print("Interface ROZA déjà appliquée.")
        return 0
    if source.count(START) != 1 or source.count(END) != 1:
        raise SystemExit("drawMainMenu introuvable ou ambigu : aucune modification effectuée.")
    if "drawRozaWordmark(" not in source:
        raise SystemExit("Le branding ROZA doit être appliqué avant l'interface.")

    start = source.index(START)
    end = source.index(END, start)
    source = source[:start] + REPLACEMENT + source[end:]
    DISPLAY.write_text(source, encoding="utf-8")
    print("Menu principal ROZA : liste compacte, palette papier/bordeaux, état Wi-Fi et batterie.")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
