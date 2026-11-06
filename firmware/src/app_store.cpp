#include "app_store.h"

#include <ArduinoJson.h>
#include <SD.h>
#include <stdio.h>
#include <string.h>

#include "sd_files.h"
#include "ui_common.h"

namespace {
static const char* const APP_ROOT = "/ROZA/apps";
static const uint8_t MAX_APPS = 12;
static const int VISIBLE_APPS = 4;

struct InstalledApp {
  char id[25];
  char name[25];
  char version[16];
  char runtime[16];
  char entry[24];
  char description[65];
};

static InstalledApp installed[MAX_APPS];
static uint8_t installedCount = 0;
static uint8_t invalidCount = 0;
static uint8_t selected = 0;
static uint8_t scrollTop = 0;
static bool detailMode = false;
static bool scanOk = false;
static const char* scanMessage = "Non analyse";

static bool validId(const char* value) {
  if (!value || !*value || strlen(value) > 24) return false;
  for (const char* p = value; *p; ++p) {
    const char c = *p;
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
          c == '_' || c == '-')) return false;
  }
  return true;
}

// L'entrée reste un simple nom de fichier : aucun sous-dossier, ".." ou chemin absolu.
static bool validEntry(const char* value) {
  if (!value || !*value || strlen(value) > 23 || value[0] == '.') return false;
  for (const char* p = value; *p; ++p) {
    const char c = *p;
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
  }
  return strstr(value, "..") == nullptr;
}

static void copyField(char* dst, size_t dstSize, const char* src) {
  if (!dst || dstSize == 0) return;
  snprintf(dst, dstSize, "%s", src ? src : "");
}

static bool ensureAppDirectory() {
  // Création limitée à /ROZA et /ROZA/apps. Aucun formatage ni suppression.
  if (!SD.exists("/ROZA") && !SD.mkdir("/ROZA")) return false;
  if (!SD.exists(APP_ROOT) && !SD.mkdir(APP_ROOT)) return false;
  File root = SD.open(APP_ROOT);
  const bool ok = root && root.isDirectory();
  if (root) root.close();
  return ok;
}

static bool readManifest(const String& folder, const String& folderPath, InstalledApp& out) {
  const String manifestPath = folderPath + "/manifest.json";
  File manifest = SD.open(manifestPath.c_str(), FILE_READ);
  if (!manifest || manifest.isDirectory() || manifest.size() == 0 || manifest.size() > 1024) {
    if (manifest) manifest.close();
    return false;
  }

  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, manifest);
  manifest.close();
  if (err) return false;

  const char* format = doc["format"] | "";
  const int formatVersion = doc["format_version"] | 0;
  const char* id = doc["id"] | "";
  const char* name = doc["name"] | "";
  const char* version = doc["version"] | "";
  const char* runtime = doc["runtime"] | "";
  const char* entry = doc["entry"] | "";
  const char* description = doc["description"] | "";

  if (strcmp(format, "roza-app") != 0 || formatVersion != 1 ||
      !validId(id) || strcmp(id, folder.c_str()) != 0 ||
      !name[0] || strlen(name) > 24 ||
      !version[0] || strlen(version) > 15 ||
      !validEntry(entry) || !runtime[0] || strlen(runtime) > 15) {
    return false;
  }

  const String entryPath = folderPath + "/" + entry;
  File entryFile = SD.open(entryPath.c_str(), FILE_READ);
  const bool entryOk = entryFile && !entryFile.isDirectory();
  if (entryFile) entryFile.close();
  if (!entryOk) return false;

  copyField(out.id, sizeof(out.id), id);
  copyField(out.name, sizeof(out.name), name);
  copyField(out.version, sizeof(out.version), version);
  copyField(out.runtime, sizeof(out.runtime), runtime);
  copyField(out.entry, sizeof(out.entry), entry);
  copyField(out.description, sizeof(out.description), description);
  return true;
}

static void scanInstalledApps() {
  installedCount = 0;
  invalidCount = 0;
  selected = 0;
  scrollTop = 0;
  detailMode = false;
  scanOk = false;

  if (!sdReady()) {
    scanMessage = "Carte microSD indisponible";
    return;
  }
  if (!ensureAppDirectory()) {
    scanMessage = "Dossier /ROZA/apps inaccessible";
    return;
  }

  File root = SD.open(APP_ROOT);
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    scanMessage = "Impossible de lire /ROZA/apps";
    return;
  }

  for (File item = root.openNextFile(); item; item = root.openNextFile()) {
    if (item.isDirectory()) {
      const String folder = sdBaseName(item.name());
      item.close();
      if (!validId(folder.c_str())) {
        ++invalidCount;
        continue;
      }
      const String folderPath = String(APP_ROOT) + "/" + folder;
      InstalledApp candidate{};
      if (readManifest(folder, folderPath, candidate)) {
        if (installedCount < MAX_APPS) installed[installedCount++] = candidate;
        else ++invalidCount;
      } else {
        ++invalidCount;
      }
    } else {
      item.close();
    }
  }
  root.close();

  // Tri alphabétique avec un tableau de taille fixe.
  for (uint8_t i = 1; i < installedCount; ++i) {
    InstalledApp value = installed[i];
    int j = (int)i - 1;
    while (j >= 0 && strcmp(installed[j].name, value.name) > 0) {
      installed[j + 1] = installed[j];
      --j;
    }
    installed[j + 1] = value;
  }

  scanOk = true;
  scanMessage = "Analyse terminee";
}

static void keepSelectionVisible() {
  if (selected < scrollTop) scrollTop = selected;
  if (selected >= scrollTop + VISIBLE_APPS) scrollTop = selected - VISIBLE_APPS + 1;
}

static void drawFooter(const char* text) {
  cv.setTextDatum(bottom_center);
  cv.setTextSize(1);
  cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
  cv.drawString(text, SW / 2, SH - 2);
}
} // namespace

void appStoreEnter() {
  scanInstalledApps();
  dirty = true;
}

void appStoreExit() {
  installedCount = 0;
  invalidCount = 0;
  selected = 0;
  scrollTop = 0;
  detailMode = false;
}

void appStoreKey(char key) {
  if (key == '`') {
    if (detailMode) detailMode = false;
    else {
      appStoreExit();
      screen = SCREEN_MENU;
    }
    dirty = true;
    return;
  }

  if (key == 'r' || key == 'R') {
    scanInstalledApps();
    dirty = true;
    return;
  }

  if (detailMode) {
    if (key == '\n' || key == ' ') detailMode = false;
    dirty = true;
    return;
  }

  if (key == ';' || key == ',') {
    if (installedCount > 0 && selected > 0) --selected;
    keepSelectionVisible();
    dirty = true;
  } else if (key == '.' || key == '/') {
    if (installedCount > 0 && selected + 1 < installedCount) ++selected;
    keepSelectionVisible();
    dirty = true;
  } else if (key == '\n' && installedCount > 0) {
    detailMode = true;
    dirty = true;
  }
}

void drawAppStore() {
  cv.fillScreen(TFT_BLACK);
  drawPageHeader("Apps", "microSD");

  if (!sdReady()) {
    cv.setTextDatum(middle_center);
    cv.setTextColor(TFT_ORANGE, TFT_BLACK);
    cv.drawString("Carte microSD indisponible", SW / 2, 56);
    cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
    cv.drawString("Verifiez son insertion", SW / 2, 74);
    drawFooter("` : retour");
    return;
  }

  if (!scanOk) {
    cv.setTextDatum(middle_center);
    cv.setTextColor(TFT_ORANGE, TFT_BLACK);
    cv.drawString(scanMessage, SW / 2, 58);
    cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
    cv.drawString("Aucun fichier supprime", SW / 2, 77);
    drawFooter("r : reessayer   ` : retour");
    return;
  }

  if (detailMode && installedCount > 0) {
    const InstalledApp& app = installed[selected];
    cv.setTextDatum(top_left);
    cv.setTextSize(1);
    cv.setTextColor(ACCENT, TFT_BLACK);
    cv.drawString(app.name, 8, 24);
    cv.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    cv.drawString((String("ID: ") + app.id).c_str(), 8, 39);
    cv.drawString((String("Version: ") + app.version).c_str(), 8, 51);
    cv.drawString((String("Moteur: ") + app.runtime).c_str(), 8, 63);
    cv.drawString((String("Entree: ") + app.entry).c_str(), 8, 75);
    String desc = app.description;
    if (desc.length() > 36) desc = desc.substring(0, 36);
    cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
    cv.drawString(desc, 8, 91);
    cv.setTextColor(TFT_ORANGE, TFT_BLACK);
    cv.drawString("Execution non active dans cette version", 8, 108);
    drawFooter("Entree / ` : liste");
    return;
  }

  cv.setTextDatum(top_left);
  cv.setTextSize(1);
  cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
  String summary = String(installedCount) + " paquet(s) reconnu(s)";
  if (invalidCount) summary += "  " + String(invalidCount) + " ignore(s)";
  cv.drawString(summary, 7, 20);

  if (installedCount == 0) {
    cv.setTextDatum(middle_center);
    cv.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    cv.drawString("Aucune application installee", SW / 2, 55);
    cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
    cv.drawString("/ROZA/apps", SW / 2, 72);
    cv.drawString("Catalogue GitHub : prochaine etape", SW / 2, 91);
  } else {
    for (int row = 0; row < VISIBLE_APPS; ++row) {
      const int i = (int)scrollTop + row;
      if (i >= installedCount) break;
      const int y = 31 + row * 23;
      const bool sel = i == selected;
      if (sel) cv.fillRoundRect(4, y, SW - 8, 21, 3, CARD_BG);
      cv.setTextDatum(middle_left);
      cv.setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, sel ? CARD_BG : TFT_BLACK);
      cv.drawString(installed[i].name, 10, y + 7);
      cv.setTextDatum(middle_right);
      cv.setTextColor(sel ? TFT_CYAN : TFT_DARKGREY, sel ? CARD_BG : TFT_BLACK);
      cv.drawString(installed[i].version, SW - 10, y + 7);
    }
    cv.setTextDatum(top_left);
    cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
    cv.drawString("Paquet reconnu - lancement a venir", 7, 126);
  }

  if (installedCount == 0) drawFooter("r : actualiser   ` : retour");
  else drawFooter("; . naviguer   Entree : details");
}
