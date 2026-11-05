#include "app_store.h"
#include "bg_fetch.h"
#include "wifi_net.h"
#include "tls_ca.h"
#include "sd_files.h"
#include "ui_common.h"
#include "weather.h"
#include "stopwatch.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cstring>

static const char* APP_CATALOG_URL =
  "https://raw.githubusercontent.com/Yoyote84/Roza/main/firmware/data/apps/catalog.json";
static const size_t MAX_CATALOG_BYTES = 8192;
static const size_t MAX_PACKAGE_BYTES = 8192;
static const int MAX_STORE_APPS = 8;
static const int MAX_APP_PAGES = 6;

struct StoreEntry {
  char id[25];
  char name[33];
  char version[13];
  char description[97];
  char url[193];
  bool installed;
};
struct RuntimePage {
  char title[33];
  char text[181];
  char action[13];
};

static StoreEntry storeApps[MAX_STORE_APPS];
static RuntimePage runtimePages[MAX_APP_PAGES];
static int storeCount = 0;
static int storeSelected = 0;
static int runtimePage = 0;
static int runtimePageCount = 0;
static int downloadIndex = -1;
static bool storeRunning = false;
static bool removeConfirm = false;
static char runtimeName[33] = "ROZA App";
static char storeError[72] = "";

static void setStoreError(const char* message) {
  snprintf(storeError, sizeof(storeError), "%s", message ? message : "");
}
static bool copyField(char* dst, size_t cap, const char* src, bool allowEmpty) {
  if (!dst || cap == 0 || !src) return false;
  size_t n = strlen(src);
  if ((!allowEmpty && n == 0) || n >= cap) return false;
  memcpy(dst, src, n);
  dst[n] = '\0';
  return true;
}
static bool validAppId(const char* id) {
  if (!id || !id[0] || strlen(id) > 24) return false;
  for (const char* p = id; *p; ++p)
    if (!((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9') || *p == '-')) return false;
  return true;
}
static bool validAction(const char* action) {
  return action &&
    (!strcmp(action, "none") || !strcmp(action, "time") ||
     !strcmp(action, "files") || !strcmp(action, "weather") ||
     !strcmp(action, "gnss") || !strcmp(action, "calc") ||
     !strcmp(action, "settings"));
}
static void makeAppPath(const char* id, char* out, size_t cap, const char* suffix) {
  snprintf(out, cap, "/ROZA/apps/%s%s", id, suffix);
}
static bool prepareNetwork() {
  if (!wifiEnsureConnected()) {
    setStoreError("Wi-Fi non connecte. Active-le dans Parametres.");
    return false;
  }
  if (!tlsClockReady()) {
    setStoreError("Heure non reglee : ouvre Date/heure.");
    return false;
  }
  return true;
}
static bool readHttpBody(WiFiClientSecure& client, const char* url, size_t maxBytes, String& body) {
  if (!url || strncmp(url, "https://raw.githubusercontent.com/", 33) != 0) {
    setStoreError("URL refusee : source GitHub Raw requise.");
    return false;
  }
  HTTPClient http;
  http.setConnectTimeout(8000);
  http.setTimeout(12000);
  http.setReuse(false);
  if (!http.begin(client, url)) {
    setStoreError("Impossible d'ouvrir la connexion HTTPS.");
    return false;
  }

  bool ok = false;
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    char msg[72];
    snprintf(msg, sizeof(msg), "HTTP %d pendant le telechargement.", code);
    setStoreError(msg);
  } else {
    int expected = http.getSize();
    if (expected > (int)maxBytes) {
      setStoreError("Paquet trop volumineux (limite 8 Ko).");
    } else {
      body = http.getString();
      if (body.length() == 0 || body.length() > maxBytes) {
        setStoreError("Reponse vide ou trop volumineuse.");
      } else ok = true;
    }
  }
  http.end();
  return ok;
}
static void refreshInstalledFlags() {
  for (int i = 0; i < storeCount; ++i) {
    char path[64];
    makeAppPath(storeApps[i].id, path, sizeof(path), ".json");
    storeApps[i].installed = sdMounted && SD.exists(path);
  }
}
static void fetchCatalog() {
  setStoreError("");
  storeCount = 0;
  if (!prepareNetwork()) return;

  WiFiClientSecure client;
  tlsUseCaBundle(client);
  client.setHandshakeTimeout(10);
  bool parsed = false;
  {
    CanvasLease lease;
    String body;
    body.reserve(MAX_CATALOG_BYTES + 1);
    JsonDocument doc;
    if (!readHttpBody(client, APP_CATALOG_URL, MAX_CATALOG_BYTES, body)) return;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
      setStoreError("JSON invalide dans le catalogue.");
      return;
    }
    JsonArray apps = doc["apps"].as<JsonArray>();
    if (doc["format"].as<int>() != 1 || apps.isNull()) {
      setStoreError("Format de catalogue non reconnu.");
      return;
    }
    for (JsonVariant item : apps) {
      if (storeCount >= MAX_STORE_APPS) break;
      StoreEntry candidate{};
      const char* id = item["id"] | "";
      const char* name = item["name"] | "";
      const char* version = item["version"] | "";
      const char* description = item["description"] | "";
      const char* url = item["url"] | "";
      if (!validAppId(id) ||
          !copyField(candidate.id, sizeof(candidate.id), id, false) ||
          !copyField(candidate.name, sizeof(candidate.name), name, false) ||
          !copyField(candidate.version, sizeof(candidate.version), version, false) ||
          !copyField(candidate.description, sizeof(candidate.description), description, true) ||
          !copyField(candidate.url, sizeof(candidate.url), url, false) ||
          strncmp(candidate.url, "https://raw.githubusercontent.com/", 33) != 0) continue;
      storeApps[storeCount++] = candidate;
    }
    parsed = storeCount > 0;
    if (!parsed) setStoreError("Aucune application ROZA compatible.");
  }
  if (parsed) {
    if (storeSelected >= storeCount) storeSelected = storeCount - 1;
    refreshInstalledFlags();
  }
  dirty = true;
}

static bool parsePackage(JsonDocument& doc, const char* expectedId) {
  const char* id = doc["id"] | "";
  const char* name = doc["name"] | "";
  const char* version = doc["version"] | "";
  JsonArray pages = doc["pages"].as<JsonArray>();
  if (doc["format"].as<int>() != 1 || strcmp(id, expectedId) != 0 ||
      !validAppId(id) || pages.isNull() || pages.size() == 0 ||
      pages.size() > MAX_APP_PAGES ||
      !copyField(runtimeName, sizeof(runtimeName), name, false)) {
    setStoreError("Paquet ROZA invalide ou incompatible.");
    return false;
  }
  if (!version || strlen(version) == 0 || strlen(version) >= 13) {
    setStoreError("Version d'application invalide.");
    return false;
  }
  int count = 0;
  for (JsonObject page : pages) {
    RuntimePage candidate{};
    const char* title = page["title"] | "";
    const char* text = page["text"] | "";
    const char* action = page["action"] | "none";
    if (!copyField(candidate.title, sizeof(candidate.title), title, false) ||
        !copyField(candidate.text, sizeof(candidate.text), text, true) ||
        !copyField(candidate.action, sizeof(candidate.action), action, false) ||
        !validAction(candidate.action)) {
      setStoreError("Page d'application invalide.");
      return false;
    }
    runtimePages[count++] = candidate;
  }
  runtimePageCount = count;
  runtimePage = 0;
  return true;
}
static void downloadSelected() {
  if (downloadIndex < 0 || downloadIndex >= storeCount) {
    setStoreError("Aucune application selectionnee.");
    return;
  }
  if (!sdMounted) {
    setStoreError("Carte microSD requise pour installer.");
    return;
  }
  if (!prepareNetwork()) return;

  StoreEntry entry = storeApps[downloadIndex];
  WiFiClientSecure client;
  tlsUseCaBundle(client);
  client.setHandshakeTimeout(10);

  bool installed = false;
  {
    CanvasLease lease;
    String body;
    body.reserve(MAX_PACKAGE_BYTES + 1);
    JsonDocument doc;
    if (!readHttpBody(client, entry.url, MAX_PACKAGE_BYTES, body)) return;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
      setStoreError("JSON invalide dans le paquet.");
      return;
    }
    if (!parsePackage(doc, entry.id)) return;

    if (!SD.exists("/ROZA") && !SD.mkdir("/ROZA")) {
      setStoreError("Creation du dossier /ROZA impossible.");
      return;
    }
    if (!SD.exists("/ROZA/apps") && !SD.mkdir("/ROZA/apps")) {
      setStoreError("Creation de /ROZA/apps impossible.");
      return;
    }

    char finalPath[64], tempPath[64];
    makeAppPath(entry.id, finalPath, sizeof(finalPath), ".json");
    makeAppPath(entry.id, tempPath, sizeof(tempPath), ".tmp");
    if (SD.exists(tempPath)) SD.remove(tempPath);
    File temp = SD.open(tempPath, FILE_WRITE);
    if (!temp) {
      setStoreError("Impossible d'ecrire sur la microSD.");
      return;
    }
    size_t written = temp.write((const uint8_t*)body.c_str(), body.length());
    temp.flush();
    temp.close();
    if (written != body.length()) {
      SD.remove(tempPath);
      setStoreError("Ecriture incomplete : installation annulee.");
      return;
    }
    // Le fichier precedent reste intact si l'ecriture temporaire echoue.
    if (SD.exists(finalPath) && !SD.remove(finalPath)) {
      SD.remove(tempPath);
      setStoreError("Impossible de remplacer l'ancienne version.");
      return;
    }
    if (!SD.rename(tempPath, finalPath)) {
      SD.remove(tempPath);
      setStoreError("Echec de finalisation de l'installation.");
      return;
    }
    installed = true;
  }

  if (installed) {
    storeApps[downloadIndex].installed = true;
    storeRunning = true;
    removeConfirm = false;
    setStoreError("");
  }
  dirty = true;
}
static bool loadInstalled(int index) {
  if (index < 0 || index >= storeCount || !sdMounted) return false;
  char path[64];
  makeAppPath(storeApps[index].id, path, sizeof(path), ".json");
  bool ok = false;
  {
    CanvasLease lease;
    File file = SD.open(path, FILE_READ);
    if (!file) setStoreError("Fichier installe introuvable.");
    else {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, file);
      file.close();
      if (err) setStoreError("JSON local invalide.");
      else ok = parsePackage(doc, storeApps[index].id);
    }
  }
  if (ok) {
    storeRunning = true;
    removeConfirm = false;
  }
  return ok;
}
static void removeSelected() {
  if (storeSelected < 0 || storeSelected >= storeCount || !sdMounted) return;
  char path[64];
  makeAppPath(storeApps[storeSelected].id, path, sizeof(path), ".json");
  if (SD.exists(path) && SD.remove(path)) {
    storeApps[storeSelected].installed = false;
    setStoreError("Application desinstallee.");
  } else setStoreError("Impossible de supprimer le paquet.");
  removeConfirm = false;
}
static void launchBuiltin(const char* action) {
  storeRunning = false;
  if (!strcmp(action, "time")) {
    screen = SCREEN_CLOCK;
    stopwatchEnter();
  } else if (!strcmp(action, "files")) {
    screen = SCREEN_FILES;
    curPath = "/";
    loadDir(curPath);
  } else if (!strcmp(action, "weather")) {
    screen = SCREEN_WEATHER;
    weatherEnter();
  } else if (!strcmp(action, "gnss")) {
    screen = SCREEN_GNSS;
  } else if (!strcmp(action, "calc")) {
    screen = SCREEN_CALC;
  } else if (!strcmp(action, "settings")) {
    screen = SCREEN_SETTINGS;
    settingsIndex = 0;
  }
  dirty = true;
}
void appStoreEnter() {
  storeRunning = false;
  removeConfirm = false;
  storeSelected = 0;
  if (!sdMounted) {
    setStoreError("Carte microSD absente : installation impossible.");
    dirty = true;
    return;
  }
  setStoreError("Chargement du catalogue...");
  bgFetchRun(fetchCatalog);
  dirty = true;
}
void appStoreExit() {
  storeRunning = false;
  removeConfirm = false;
}
void appStoreDraw() {
  cv.fillScreen(TFT_BLACK);
  if (storeRunning && runtimePageCount > 0) {
    char pageInfo[16];
    snprintf(pageInfo, sizeof(pageInfo), "%d/%d", runtimePage + 1, runtimePageCount);
    drawPageHeader(runtimeName, pageInfo, ACCENT);
    cv.fillRoundRect(8, 18, SW - 16, 96, 4, ROZA_INK);
    cv.drawRoundRect(8, 18, SW - 16, 96, 4, ROZA_OLIVE_DARK);
    cv.setTextDatum(top_left);
    cv.setTextColor(ACCENT, ROZA_INK);
    cv.setTextSize(1);
    cv.drawString(runtimePages[runtimePage].title, 14, 25);
    const char* p = runtimePages[runtimePage].text;
    int y = 43, lines = 0;
    while (*p && lines < 5) {
      char line[38];
      int n = 0, lastSpace = -1;
      while (p[n] && p[n] != '
' && n < 35) {
        if (p[n] == ' ') lastSpace = n;
        ++n;
      }
      if (p[n] && p[n] != '
' && lastSpace > 0) n = lastSpace;
      while (n > 0 && (((uint8_t)p[n] & 0xC0) == 0x80)) --n;
      if (n <= 0) n = (int)strlen(p) < 35 ? (int)strlen(p) : 35;
      memcpy(line, p, n);
      line[n] = '\0';
      cv.setTextColor(ROZA_PAPER, ROZA_INK);
      cv.drawString(line, 14, y);
      p += n;
      while (*p == ' ') ++p;
      if (*p == '
') ++p;
      y += 12;
      ++lines;
    }
    if (strcmp(runtimePages[runtimePage].action, "none") != 0) {
      cv.setTextColor(ROZA_OLIVE_LIGHT, ROZA_INK);
      cv.drawString("Entree : ouvrir la fonction", 14, 104);
    }
    cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
    cv.setTextDatum(bottom_center);
    cv.drawString("; . pages   retour : catalogue", SW / 2, SH - 2);
    return;
  }

  char position[16];
  if (storeCount > 0) snprintf(position, sizeof(position), "%d/%d", storeSelected + 1, storeCount);
  else snprintf(position, sizeof(position), "CATALOGUE");
  drawPageHeader("ROZA Apps", position, ACCENT);
  if (!sdMounted) {
    cv.setTextColor(TFT_RED, TFT_BLACK);
    cv.setTextDatum(middle_center);
    cv.setTextSize(1);
    cv.drawString("Carte microSD absente", SW / 2, 56);
  } else if (storeCount == 0) {
    cv.setTextDatum(middle_center);
    cv.setTextSize(1);
    cv.setTextColor(storeError[0] ? TFT_YELLOW : ROZA_PAPER_DIM, TFT_BLACK);
    cv.drawString(storeError[0] ? storeError : "Catalogue vide", SW / 2, 52);
    cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
    cv.drawString("r : recharger le catalogue", SW / 2, 73);
  } else {
    StoreEntry& entry = storeApps[storeSelected];
    cv.fillRoundRect(8, 18, SW - 16, 94, 4, ROZA_INK);
    cv.drawRoundRect(8, 18, SW - 16, 94, 4, ROZA_OLIVE_DARK);
    cv.setTextDatum(top_left);
    cv.setTextColor(ROZA_PAPER, ROZA_INK);
    cv.setTextSize(2);
    cv.drawString(entry.name, 14, 25);
    cv.setTextSize(1);
    cv.setTextColor(ROZA_OLIVE_LIGHT, ROZA_INK);
    char version[24];
    snprintf(version, sizeof(version), "v%s  %s", entry.version, entry.installed ? "INSTALLEE" : "A INSTALLER");
    cv.drawString(version, 14, 47);
    const char* p = entry.description;
    int y = 62, lines = 0;
    while (*p && lines < 3) {
      char line[38];
      int n = 0, lastSpace = -1;
      while (p[n] && p[n] != '
' && n < 35) {
        if (p[n] == ' ') lastSpace = n;
        ++n;
      }
      if (p[n] && p[n] != '
' && lastSpace > 0) n = lastSpace;
      while (n > 0 && (((uint8_t)p[n] & 0xC0) == 0x80)) --n;
      if (n <= 0) n = (int)strlen(p) < 35 ? (int)strlen(p) : 35;
      memcpy(line, p, n);
      line[n] = '\0';
      cv.setTextColor(ROZA_PAPER_DIM, ROZA_INK);
      cv.drawString(line, 14, y);
      p += n;
      while (*p == ' ') ++p;
      if (*p == '
') ++p;
      y += 11;
      ++lines;
    }
    if (storeError[0]) {
      cv.setTextColor(TFT_YELLOW, ROZA_INK);
      cv.drawString(storeError, 14, 99);
    }
    if (removeConfirm) {
      cv.setTextColor(TFT_RED, ROZA_INK);
      cv.drawString("Enter confirmer retrait / retour annuler", 14, 99);
    }
  }
  cv.setTextDatum(bottom_center);
  cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
  if (removeConfirm) cv.drawString("Enter confirmer   retour annuler", SW / 2, SH - 2);
  else cv.drawString("Enter installer/ouvrir  d maj  x retirer", SW / 2, SH - 2);
  cv.setTextDatum(bottom_left);
  cv.drawString("; . choisir  r actualiser", 6, SH - 2);
}
void appStoreKey(char k) {
  if (storeRunning && runtimePageCount > 0) {
    if ((uint8_t)k == 96) storeRunning = false;
    else if (k == ';' || k == ',') { if (runtimePage > 0) --runtimePage; }
    else if (k == '.' || k == '/') { if (runtimePage + 1 < runtimePageCount) ++runtimePage; }
    else if ((uint8_t)k == 10) launchBuiltin(runtimePages[runtimePage].action);
    dirty = true;
    return;
  }
  if (removeConfirm) {
    if ((uint8_t)k == 96) removeConfirm = false;
    else if ((uint8_t)k == 10) removeSelected();
    dirty = true;
    return;
  }
  if ((uint8_t)k == 96) {
    appStoreExit();
    screen = SCREEN_MENU;
  } else if (k == ';' || k == ',') {
    if (storeCount > 0) storeSelected = (storeSelected + storeCount - 1) % storeCount;
  } else if (k == '.' || k == '/') {
    if (storeCount > 0) storeSelected = (storeSelected + 1) % storeCount;
  } else if (k == 'r' || k == 'R') {
    setStoreError("Actualisation du catalogue...");
    bgFetchRun(fetchCatalog);
  } else if ((uint8_t)k == 10) {
    if (storeCount <= 0) setStoreError("Catalogue indisponible : appuie sur r.");
    else if (storeApps[storeSelected].installed) loadInstalled(storeSelected);
    else {
      downloadIndex = storeSelected;
      setStoreError("Telechargement et installation...");
      bgFetchRun(downloadSelected);
    }
  } else if (k == 'd' || k == 'D') {
    if (storeCount > 0) {
      downloadIndex = storeSelected;
      setStoreError("Mise a jour...");
      bgFetchRun(downloadSelected);
    }
  } else if (k == 'i' || k == 'I') {
    if (storeCount > 0 && storeApps[storeSelected].installed) loadInstalled(storeSelected);
    else setStoreError("Application non installee : Enter pour installer.");
  } else if (k == 'x' || k == 'X') {
    if (storeCount > 0 && storeApps[storeSelected].installed) removeConfirm = true;
  }
  dirty = true;
}
