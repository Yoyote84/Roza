#include "app_store.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <SD.h>
#include <mbedtls/sha256.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "sd_files.h"
#include "ui_common.h"
#include "wifi_net.h"
#include "tls_ca.h"
#include "bg_fetch.h"
namespace {
static const char* const APP_ROOT = "/ROZA/apps";
static const char* const CATALOG_URL = "https://raw.githubusercontent.com/Yoyote84/Roza/feature/roza-sd-app-manager/roza-app-catalog/catalog.json";
static const char* const ASSET_BASE = "https://raw.githubusercontent.com/Yoyote84/Roza/feature/roza-sd-app-manager/roza-app-catalog/apps/";
static const uint8_t MAX_APPS = 12;
static const int VISIBLE_APPS = 4;
static const size_t MAX_CATALOG_BYTES = 12 * 1024;
static const size_t MAX_MANIFEST_BYTES = 1024;
static const size_t MAX_ENTRY_BYTES = 96 * 1024;
struct InstalledApp { char id[25]; char name[25]; char version[16]; char runtime[16]; char entry[24]; char description[65]; };
struct CatalogApp { char id[25]; char name[25]; char version[16]; char runtime[16]; char entry[24]; char description[65]; char manifestSha[65]; char entrySha[65]; uint16_t manifestSize; uint32_t entrySize; };
static InstalledApp installed[MAX_APPS];
static CatalogApp catalog[MAX_APPS];
static uint8_t installedCount = 0, catalogCount = 0, invalidCount = 0, selected = 0, scrollTop = 0;
static bool detailMode = false, catalogMode = true, confirmRemove = false, scanOk = false, catalogReady = false;
static const char* scanMessage = "Non analyse";
static char catalogMessage[72] = "Catalogue non charge";
static char jobMessage[96] = "";
static CatalogApp jobApp{};
static char jobId[25] = "";
static bool validId(const char* value) {
  if (!value || !*value || strlen(value) > 24) return false;
  for (const char* p = value; *p; ++p) {
    const char c = *p;
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
  }
  return true;
}
static bool validEntry(const char* value) {
  if (!value || !*value || strlen(value) > 23 || value[0] == '.') return false;
  for (const char* p = value; *p; ++p) {
    const char c = *p;
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
  }
  return strcmp(value, "manifest.json") != 0 && strstr(value, "..") == nullptr;
}
static bool validSha256(const char* value) {
  if (!value || strlen(value) != 64) return false;
  for (const char* p = value; *p; ++p) {
    const char c = *p;
    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
  }
  return true;
}
static void copyField(char* dst, size_t size, const char* src) {
  if (dst && size) snprintf(dst, size, "%s", src ? src : "");
}
static void setJobMessage(const char* message) { copyField(jobMessage, sizeof(jobMessage), message); }
static bool ensureAppDirectory() {
  if (!SD.exists("/ROZA") && !SD.mkdir("/ROZA")) return false;
  if (!SD.exists(APP_ROOT) && !SD.mkdir(APP_ROOT)) return false;
  File root = SD.open(APP_ROOT);
  const bool ok = root && root.isDirectory();
  if (root) root.close();
  return ok;
}
static bool removeTree(const String& path) {
  File dir = SD.open(path.c_str());
  if (!dir) return !SD.exists(path.c_str());
  if (!dir.isDirectory()) { dir.close(); return SD.remove(path.c_str()); }
  for (File item = dir.openNextFile(); item; item = dir.openNextFile()) {
    const String base = sdBaseName(item.name());
    const bool isDir = item.isDirectory();
    item.close();
    if (base.length() == 0 || base == "." || base == "..") { dir.close(); return false; }
    const String child = path + "/" + base;
    const bool removed = isDir ? removeTree(child) : SD.remove(child.c_str());
    if (!removed) { dir.close(); return false; }
  }
  dir.close();
  return SD.rmdir(path.c_str());
}
static bool hashFileSha256(const String& path, char output[65]) {
  File file = SD.open(path.c_str(), FILE_READ);
  if (!file || file.isDirectory()) { if (file) file.close(); return false; }
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  int rc = mbedtls_sha256_starts_ret(&ctx, 0);
  uint8_t buffer[1024];
  while (rc == 0 && file.available()) {
    const int count = file.read(buffer, sizeof(buffer));
    if (count < 0) { rc = -1; break; }
    if (count == 0) break;
    rc = mbedtls_sha256_update_ret(&ctx, buffer, (size_t)count);
  }
  uint8_t digest[32];
  if (rc == 0) rc = mbedtls_sha256_finish_ret(&ctx, digest);
  mbedtls_sha256_free(&ctx);
  file.close();
  if (rc != 0) return false;
  static const char ROZA_HEX_CHARS[] = "0123456789abcdef";
  for (int i = 0; i < 32; ++i) {
    output[i * 2] = ROZA_HEX_CHARS[digest[i] >> 4];
    output[i * 2 + 1] = ROZA_HEX_CHARS[digest[i] & 0x0f];
  }
  output[64] = 0;
  return true;
}
static bool hashMatches(const String& path, const char* expected) {
  char actual[65];
  return validSha256(expected) && hashFileSha256(path, actual) && strcasecmp(actual, expected) == 0;
}
static bool readSmallFile(const String& path, String& contents, size_t maxBytes) {
  File file = SD.open(path.c_str(), FILE_READ);
  if (!file || file.isDirectory() || file.size() == 0 || file.size() > maxBytes) { if (file) file.close(); return false; }
  contents = "";
  contents.reserve(file.size());
  while (file.available()) {
    const int c = file.read();
    if (c < 0) break;
    contents += (char)c;
    if (contents.length() > maxBytes) { file.close(); return false; }
  }
  file.close();
  return contents.length() > 0;
}
static bool tlsReady() {
  if (WiFi.status() != WL_CONNECTED) { setJobMessage("Connectez le Wi-Fi dans Settings"); return false; }
  if (!tlsClockReady()) { setJobMessage("Heure requise : synchronisez NTP"); return false; }
  return true;
}
static String assetUrl(const char* id, const char* filename) {
  return String(ASSET_BASE) + id + "/" + filename;
}
static bool downloadVerifiedFile(const String& url, const String& path, size_t expectedSize,
                                 size_t maxSize, const char* expectedSha) {
  if (!expectedSize || expectedSize > maxSize || !validSha256(expectedSha)) return false;
  if (SD.exists(path.c_str())) SD.remove(path.c_str());
  WiFiClientSecure client;
  tlsUseCaBundle(client);
  client.setTimeout(15000);
  HTTPClient http;
  http.setTimeout(15000);
  http.setReuse(false);
  http.useHTTP10(true);
  if (!http.begin(client, url)) return false;
  const int code = http.GET();
  const int responseSize = http.getSize();
  if (code != HTTP_CODE_OK || responseSize < 0 || (size_t)responseSize != expectedSize) {
    http.end();
    return false;
  }
  WiFiClient* stream = http.getStreamPtr();
  File output = SD.open(path.c_str(), FILE_WRITE);
  if (!stream || !output) { if (output) output.close(); http.end(); return false; }
  uint8_t buffer[1024];
  size_t received = 0;
  uint32_t lastProgress = millis();
  bool writeOk = true;
  while (received < expectedSize && millis() - lastProgress < 45000UL) {
    const int available = stream->available();
    if (available > 0) {
      size_t wanted = (size_t)available;
      if (wanted > sizeof(buffer)) wanted = sizeof(buffer);
      if (wanted > expectedSize - received) wanted = expectedSize - received;
      const int count = stream->readBytes(buffer, wanted);
      if (count > 0) {
        if (output.write(buffer, (size_t)count) != (size_t)count) { writeOk = false; break; }
        received += (size_t)count;
        lastProgress = millis();
      }
    } else {
      if (!http.connected()) break;
      delay(2);
    }
  }
  output.flush();
  output.close();
  http.end();
  if (!writeOk || received != expectedSize) { SD.remove(path.c_str()); return false; }
  if (!hashMatches(path, expectedSha)) { SD.remove(path.c_str()); return false; }
  return true;
}
static bool fetchCatalogBody(String& body) {
  if (!tlsReady()) return false;
  WiFiClientSecure client;
  tlsUseCaBundle(client);
  client.setTimeout(15000);
  HTTPClient http;
  http.setTimeout(15000);
  http.setReuse(false);
  http.useHTTP10(true);
  if (!http.begin(client, CATALOG_URL)) { setJobMessage("Impossible d'ouvrir le catalogue HTTPS"); return false; }
  const int code = http.GET();
  const int length = http.getSize();
  if (code != HTTP_CODE_OK || length <= 0 || (size_t)length > MAX_CATALOG_BYTES) {
    http.end();
    setJobMessage("Catalogue inaccessible ou trop volumineux");
    return false;
  }
  body = http.getString();
  http.end();
  if (body.length() != (size_t)length) { setJobMessage("Catalogue incomplet"); return false; }
  return true;
}
static void scanInstalledApps() {
  installedCount = 0;
  invalidCount = 0;
  selected = 0;
  scrollTop = 0;
  detailMode = false;
  confirmRemove = false;
  scanOk = false;
  if (!sdReady()) { scanMessage = "Carte microSD indisponible"; return; }
  if (!ensureAppDirectory()) { scanMessage = "Dossier /ROZA/apps inaccessible"; return; }
  File root = SD.open(APP_ROOT);
  if (!root || !root.isDirectory()) {
    if (root) root.close();
    scanMessage = "Impossible de lire /ROZA/apps";
    return;
  }
  for (File item = root.openNextFile(); item; item = root.openNextFile()) {
    if (!item.isDirectory()) { item.close(); continue; }
    const String folder = sdBaseName(item.name());
    item.close();
    if (!validId(folder.c_str())) continue;
    const String folderPath = String(APP_ROOT) + "/" + folder;
    String manifestText;
    if (!readSmallFile(folderPath + "/manifest.json", manifestText, MAX_MANIFEST_BYTES)) {
      ++invalidCount;
      continue;
    }
    JsonDocument doc;
    if (deserializeJson(doc, manifestText)) { ++invalidCount; continue; }
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
        !name[0] || strlen(name) > 24 || !version[0] || strlen(version) > 15 ||
        !validEntry(entry) || !runtime[0] || strlen(runtime) > 15) {
      ++invalidCount;
      continue;
    }
    File entryFile = SD.open((folderPath + "/" + entry).c_str(), FILE_READ);
    const bool entryOk = entryFile && !entryFile.isDirectory();
    if (entryFile) entryFile.close();
    if (!entryOk) { ++invalidCount; continue; }
    if (installedCount < MAX_APPS) {
      InstalledApp& out = installed[installedCount++];
      copyField(out.id, sizeof(out.id), id);
      copyField(out.name, sizeof(out.name), name);
      copyField(out.version, sizeof(out.version), version);
      copyField(out.runtime, sizeof(out.runtime), runtime);
      copyField(out.entry, sizeof(out.entry), entry);
      copyField(out.description, sizeof(out.description), description);
    } else ++invalidCount;
  }
  root.close();
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
static bool parseCatalog(const String& body) {
  JsonDocument doc;
  if (deserializeJson(doc, body)) { setJobMessage("JSON du catalogue invalide"); return false; }
  const char* format = doc["format"] | "";
  const int version = doc["format_version"] | 0;
  JsonArray apps = doc["apps"].as<JsonArray>();
  if (strcmp(format, "roza-app-catalog") != 0 || version != 1 ||
      apps.isNull() || apps.size() == 0 || apps.size() > MAX_APPS) {
    setJobMessage("Format ou nombre d'apps invalide");
    return false;
  }
  CatalogApp next[MAX_APPS]{};
  uint8_t count = 0;
  for (JsonObject item : apps) {
    CatalogApp candidate{};
    const char* id = item["id"] | "";
    const char* name = item["name"] | "";
    const char* appVersion = item["version"] | "";
    const char* description = item["description"] | "";
    const char* runtime = item["runtime"] | "";
    const char* entry = item["entry"] | "";
    const char* manifestSha = item["manifest_sha256"] | "";
    const char* entrySha = item["entry_sha256"] | "";
    const uint32_t manifestSize = item["manifest_size"] | 0;
    const uint32_t entrySize = item["entry_size"] | 0;
    if (!validId(id) || !name[0] || strlen(name) > 24 ||
        !appVersion[0] || strlen(appVersion) > 15 ||
        !validEntry(entry) || strcmp(runtime, "roza-lua") != 0 ||
        !validSha256(manifestSha) || !validSha256(entrySha) ||
        manifestSize == 0 || manifestSize > MAX_MANIFEST_BYTES ||
        entrySize == 0 || entrySize > MAX_ENTRY_BYTES) {
      setJobMessage("Entree de catalogue invalide");
      return false;
    }
    for (uint8_t i = 0; i < count; ++i) {
      if (strcmp(next[i].id, id) == 0) {
        setJobMessage("Identifiant duplique dans le catalogue");
        return false;
      }
    }
    copyField(candidate.id, sizeof(candidate.id), id);
    copyField(candidate.name, sizeof(candidate.name), name);
    copyField(candidate.version, sizeof(candidate.version), appVersion);
    copyField(candidate.description, sizeof(candidate.description), description);
    copyField(candidate.runtime, sizeof(candidate.runtime), runtime);
    copyField(candidate.entry, sizeof(candidate.entry), entry);
    copyField(candidate.manifestSha, sizeof(candidate.manifestSha), manifestSha);
    copyField(candidate.entrySha, sizeof(candidate.entrySha), entrySha);
    candidate.manifestSize = (uint16_t)manifestSize;
    candidate.entrySize = entrySize;
    next[count++] = candidate;
  }
  memcpy(catalog, next, sizeof(catalog));
  catalogCount = count;
  catalogReady = true;
  return true;
}
static void fetchCatalogWorker() {
  bgFetchStatus("Catalogue HTTPS");
  String body;
  if (!fetchCatalogBody(body)) {
    copyField(catalogMessage, sizeof(catalogMessage), jobMessage);
    return;
  }
  bgFetchStatus("Verification catalogue");
  if (!parseCatalog(body)) {
    copyField(catalogMessage, sizeof(catalogMessage), jobMessage);
    return;
  }
  snprintf(catalogMessage, sizeof(catalogMessage), "%u application(s) disponibles", catalogCount);
  setJobMessage("Catalogue mis a jour");
}
static bool manifestMatches(const String& manifestPath, const CatalogApp& app) {
  String body;
  if (!readSmallFile(manifestPath, body, MAX_MANIFEST_BYTES)) return false;
  JsonDocument doc;
  if (deserializeJson(doc, body)) return false;
  const char* format = doc["format"] | "";
  const int formatVersion = doc["format_version"] | 0;
  const char* id = doc["id"] | "";
  const char* name = doc["name"] | "";
  const char* version = doc["version"] | "";
  const char* runtime = doc["runtime"] | "";
  const char* entry = doc["entry"] | "";
  return strcmp(format, "roza-app") == 0 && formatVersion == 1 &&
         strcmp(id, app.id) == 0 && strcmp(name, app.name) == 0 &&
         strcmp(version, app.version) == 0 && strcmp(runtime, app.runtime) == 0 &&
         strcmp(entry, app.entry) == 0;
}
static void installWorker() {
  if (!sdReady() || !ensureAppDirectory()) { setJobMessage("Carte microSD indisponible"); return; }
  if (!tlsReady()) return;
  const String finalPath = String(APP_ROOT) + "/" + jobApp.id;
  const String stagePath = String(APP_ROOT) + "/.stage-" + jobApp.id;
  const String backupPath = String(APP_ROOT) + "/.backup-" + jobApp.id;
  const String manifestPart = stagePath + "/manifest.part";
  const String entryPart = stagePath + "/" + jobApp.entry + ".part";
  const String manifestFinal = stagePath + "/manifest.json";
  const String entryFinal = stagePath + "/" + jobApp.entry;
  if (SD.exists(backupPath.c_str())) {
    if (!SD.exists(finalPath.c_str())) {
      if (!SD.rename(backupPath.c_str(), finalPath.c_str())) {
        setJobMessage("Sauvegarde precedente a restaurer");
        return;
      }
    } else if (!removeTree(backupPath)) {
      setJobMessage("Ancienne sauvegarde impossible a nettoyer");
      return;
    }
  }
  if (SD.exists(stagePath.c_str()) && !removeTree(stagePath)) {
    setJobMessage("Dossier temporaire impossible a nettoyer");
    return;
  }
  if (!SD.mkdir(stagePath.c_str())) {
    setJobMessage("Creation du dossier temporaire impossible");
    return;
  }
  bgFetchStatus("Telechargement manifeste");
  if (!downloadVerifiedFile(assetUrl(jobApp.id, "manifest.json"), manifestPart,
                            jobApp.manifestSize, MAX_MANIFEST_BYTES, jobApp.manifestSha)) {
    removeTree(stagePath);
    setJobMessage("Manifeste invalide ou SHA-256 incorrect");
    return;
  }
  if (!manifestMatches(manifestPart, jobApp)) {
    removeTree(stagePath);
    setJobMessage("Manifeste different du catalogue");
    return;
  }
  bgFetchStatus("Telechargement application");
  if (!downloadVerifiedFile(assetUrl(jobApp.id, jobApp.entry), entryPart,
                            jobApp.entrySize, MAX_ENTRY_BYTES, jobApp.entrySha)) {
    removeTree(stagePath);
    setJobMessage("Fichier incomplet ou SHA-256 incorrect");
    return;
  }
  if (!SD.rename(manifestPart.c_str(), manifestFinal.c_str()) ||
      !SD.rename(entryPart.c_str(), entryFinal.c_str())) {
    removeTree(stagePath);
    setJobMessage("Preparation du paquet impossible");
    return;
  }
  const bool hadPrevious = SD.exists(finalPath.c_str());
  if (hadPrevious && !SD.rename(finalPath.c_str(), backupPath.c_str())) {
    removeTree(stagePath);
    setJobMessage("Impossible de sauvegarder l'ancienne version");
    return;
  }
  if (!SD.rename(stagePath.c_str(), finalPath.c_str())) {
    if (hadPrevious) SD.rename(backupPath.c_str(), finalPath.c_str());
    removeTree(stagePath);
    setJobMessage("Installation interrompue, retour arriere tente");
    return;
  }
  if (hadPrevious) removeTree(backupPath);
  scanInstalledApps();
  setJobMessage("Application installee et verifiee");
}
static void removeWorker() {
  if (!sdReady() || !validId(jobId)) {
    setJobMessage("Identifiant ou carte microSD invalide");
    return;
  }
  const String path = String(APP_ROOT) + "/" + jobId;
  if (!SD.exists(path.c_str())) { setJobMessage("Application deja absente"); return; }
  if (!removeTree(path)) { setJobMessage("Suppression incomplete"); return; }
  scanInstalledApps();
  setJobMessage("Application supprimee de la microSD");
}
static void startCatalogRefresh() {
  if (bgFetchBusy()) return;
  setJobMessage("");
  catalogReady = false;
  catalogCount = 0;
  if (WiFi.status() != WL_CONNECTED) {
    setJobMessage("Connectez le Wi-Fi dans Settings");
    copyField(catalogMessage, sizeof(catalogMessage), jobMessage);
    return;
  }
  copyField(catalogMessage, sizeof(catalogMessage), "Connexion au catalogue...");
  bgFetchRun(fetchCatalogWorker);
}
static void startInstall() {
  if (!catalogReady || catalogCount == 0 || selected >= catalogCount || bgFetchBusy()) return;
  if (!sdReady()) { setJobMessage("Inserez une carte microSD"); return; }
  jobApp = catalog[selected];
  setJobMessage("");
  bgFetchRun(installWorker);
}
static void startRemove() {
  if (!scanOk || installedCount == 0 || selected >= installedCount || bgFetchBusy()) return;
  copyField(jobId, sizeof(jobId), installed[selected].id);
  confirmRemove = false;
  detailMode = false;
  setJobMessage("");
  bgFetchRun(removeWorker);
}
static bool isInstalled(const char* id) {
  for (uint8_t i = 0; i < installedCount; ++i)
    if (strcmp(installed[i].id, id) == 0) return true;
  return false;
}
static void keepSelectionVisible(uint8_t count) {
  if (count == 0) { selected = 0; scrollTop = 0; return; }
  if (selected >= count) selected = count - 1;
  if (selected < scrollTop) scrollTop = selected;
  if (selected >= scrollTop + VISIBLE_APPS) selected = scrollTop + VISIBLE_APPS - 1;
  if (selected >= count) selected = count - 1;
  if (selected >= scrollTop + VISIBLE_APPS) scrollTop = selected - VISIBLE_APPS + 1;
}
static void drawFooter(const char* text) {
  cv.setTextDatum(bottom_center);
  cv.setTextSize(1);
  cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
  const char* shown = jobMessage[0] ? jobMessage : text;
  cv.drawString(shown, SW / 2, SH - 2);
}
} // namespace
void appStoreEnter() {
  setJobMessage("");
  scanInstalledApps();
  catalogMode = true;
  catalogReady = false;
  catalogCount = 0;
  selected = 0;
  scrollTop = 0;
  detailMode = false;
  confirmRemove = false;
  if (WiFi.status() == WL_CONNECTED) startCatalogRefresh();
  else copyField(catalogMessage, sizeof(catalogMessage), "Wi-Fi requis : Settings");
  dirty = true;
}
void appStoreExit() {
  installedCount = 0;
  catalogCount = 0;
  invalidCount = 0;
  selected = 0;
  scrollTop = 0;
  detailMode = false;
  confirmRemove = false;
}
void appStoreKey(char key) {
  if (bgFetchBusy()) return;
  if (key == 0x60) {
    if (confirmRemove) confirmRemove = false;
    else if (detailMode) detailMode = false;
    else { appStoreExit(); screen = SCREEN_MENU; }
    dirty = true;
    return;
  }
  if (confirmRemove) {
    if (key == 10) startRemove();
    else if (key == 'u' || key == 'U') confirmRemove = false;
    dirty = true;
    return;
  }
  if (key == 't' || key == 'T') {
    catalogMode = !catalogMode;
    detailMode = false;
    selected = 0;
    scrollTop = 0;
    dirty = true;
    return;
  }
  if (key == 'r' || key == 'R') {
    startCatalogRefresh();
    dirty = true;
    return;
  }
  const uint8_t count = catalogMode ? catalogCount : installedCount;
  if (key == ';' || key == ',') {
    if (count > 0 && selected > 0) --selected;
    keepSelectionVisible(count);
    dirty = true;
    return;
  }
  if (key == '.' || key == '/') {
    if (count > 0 && selected + 1 < count) ++selected;
    keepSelectionVisible(count);
    dirty = true;
    return;
  }
  if (detailMode) {
    if (key == 10 || key == ' ') detailMode = false;
    else if ((key == 'i' || key == 'I') && catalogMode) startInstall();
    else if ((key == 'u' || key == 'U') && !catalogMode) confirmRemove = true;
    dirty = true;
    return;
  }
  if (key == 10 && count > 0) {
    detailMode = true;
    dirty = true;
  } else if ((key == 'i' || key == 'I') && catalogMode) {
    startInstall();
    dirty = true;
  } else if ((key == 'u' || key == 'U') && !catalogMode && installedCount > 0) {
    confirmRemove = true;
    dirty = true;
  }
}
void drawAppStore() {
  cv.fillScreen(TFT_BLACK);
  drawPageHeader("Apps", catalogMode ? "Catalogue" : "microSD");
  if (confirmRemove && installedCount > 0) {
    cv.setTextDatum(middle_center);
    cv.setTextColor(TFT_ORANGE, TFT_BLACK);
    cv.drawString("Supprimer cette application ?", SW / 2, 48);
    cv.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    cv.drawString(installed[selected].name, SW / 2, 67);
    cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
    cv.drawString("Entree : confirmer", SW / 2, 88);
    drawFooter("retour / u : annuler");
    return;
  }
  const uint8_t count = catalogMode ? catalogCount : installedCount;
  if (detailMode && count > 0) {
    cv.setTextDatum(top_left);
    cv.setTextSize(1);
    if (catalogMode) {
      const CatalogApp& app = catalog[selected];
      cv.setTextColor(ACCENT, TFT_BLACK);
      cv.drawString(app.name, 8, 24);
      cv.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
      cv.drawString((String("Version: ") + app.version).c_str(), 8, 39);
      cv.drawString((String("Moteur: ") + app.runtime).c_str(), 8, 51);
      cv.drawString((String("Taille: ") + String(app.entrySize) + " o").c_str(), 8, 63);
      String desc = app.description;
      if (desc.length() > 36) desc = desc.substring(0, 36);
      cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
      cv.drawString(desc, 8, 79);
      cv.setTextColor(isInstalled(app.id) ? TFT_GREEN : TFT_ORANGE, TFT_BLACK);
      cv.drawString(isInstalled(app.id) ? "Deja installee" : "Disponible au telechargement", 8, 96);
      cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
      cv.drawString("Moteur Lua non actif dans cette version", 8, 110);
      drawFooter("i : installer   Entree : retour");
    } else {
      const InstalledApp& app = installed[selected];
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
      cv.drawString("Paquet present sur microSD", 8, 108);
      drawFooter("u : desinstaller   Entree : retour");
    }
    return;
  }
  cv.setTextDatum(top_left);
  cv.setTextSize(1);
  cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
  if (catalogMode) cv.drawString(catalogReady ? catalogMessage : "Catalogue : chargement requis", 7, 20);
  else if (!scanOk) cv.drawString(scanMessage, 7, 20);
  else {
    String summary = String(installedCount) + " installee(s)";
    if (invalidCount) summary += "  " + String(invalidCount) + " ignoree(s)";
    cv.drawString(summary, 7, 20);
  }
  if (count == 0) {
    cv.setTextDatum(middle_center);
    cv.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    if (catalogMode) {
      cv.drawString(catalogReady ? "Catalogue vide" : "Catalogue indisponible", SW / 2, 55);
      cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
      cv.drawString(catalogMessage, SW / 2, 74);
      cv.drawString("r : reessayer", SW / 2, 91);
    } else {
      cv.drawString(scanOk ? "Aucune application installee" : scanMessage, SW / 2, 58);
      cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
      cv.drawString("/ROZA/apps", SW / 2, 77);
    }
  } else {
    for (int row = 0; row < VISIBLE_APPS; ++row) {
      const int i = (int)scrollTop + row;
      if (i >= count) break;
      const int y = 31 + row * 23;
      const bool sel = i == selected;
      if (sel) cv.fillRoundRect(4, y, SW - 8, 21, 3, CARD_BG);
      cv.setTextDatum(middle_left);
      cv.setTextColor(sel ? TFT_WHITE : TFT_LIGHTGREY, sel ? CARD_BG : TFT_BLACK);
      cv.drawString(catalogMode ? catalog[i].name : installed[i].name, 10, y + 7);
      cv.setTextDatum(middle_right);
      cv.setTextColor(sel ? TFT_CYAN : TFT_DARKGREY, sel ? CARD_BG : TFT_BLACK);
      cv.drawString(catalogMode ? catalog[i].version : installed[i].version, SW - 10, y + 7);
    }
  }
  if (catalogMode) drawFooter("t : microSD   ; . : choix   i : installer");
  else drawFooter("t : catalogue   ; . : choix   u : supprimer");
}
