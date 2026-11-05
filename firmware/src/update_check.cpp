#include "update_check.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <cstring>
#include "bg_fetch.h"
#include "net_job.h"
#include "tls_ca.h"
#include "wifi_net.h"
#include "ui_common.h"
#include "roza_theme.h"

#ifndef ROZA_BUILD_VERSION
#define ROZA_BUILD_VERSION "dev"
#endif

static const char* RELEASE_API = "https://api.github.com/repos/Yoyote84/Roza/releases/latest";
static DeferredFetch releaseJob;
static String installedVersion = ROZA_BUILD_VERSION;
static String latestTag, publishedDate, resultMessage;
static bool checked = false, releaseFound = false;
static bool updateAvailable = false, updateAssetFound = false;

static void fetchLatestRelease() {
  // Réserver avant CanvasLease : ces chaînes survivent au TLS.
  latestTag.reserve(64);
  publishedDate.reserve(24);
  resultMessage.reserve(96);
  latestTag = "";
  publishedDate = "";
  resultMessage = "";

  if (!wifiEnsureConnected()) {
    resultMessage = "Wi-Fi non connecte";
    checked = true;
    dirty = true;
    return;
  }
  if (!tlsClockReady()) {
    resultMessage = "Horloge non synchronisee";
    checked = true;
    dirty = true;
    return;
  }

  WiFiClientSecure client;
  tlsUseCaBundle(client);
  client.setHandshakeTimeout(10);

  bool parsed = false;
  bool assetFound = false;
  String tag; tag.reserve(64);
  String date; date.reserve(24);
  String error; error.reserve(96);
  {
    CanvasLease lease;
    HTTPClient http;
    http.setConnectTimeout(8000);
    http.setTimeout(12000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    if (!http.begin(client, RELEASE_API)) {
      error = "Connexion GitHub impossible";
    } else {
      http.addHeader("Accept", "application/vnd.github+json");
      http.addHeader("User-Agent", "ROZA-Cardputer-ADV");
      int code = http.GET();
      if (code == HTTP_CODE_OK) {
        JsonDocument doc;
        DeserializationError jsonError = deserializeJson(doc, http.getStream());
        if (!jsonError) {
          tag = doc["tag_name"] | "";
          date = doc["published_at"] | "";
          JsonArray assets = doc["assets"].as<JsonArray>();
          for (JsonObject asset : assets) {
            const char* assetName = asset["name"] | "";
            if (strcmp(assetName, "ROZA_UPDATE.bin") == 0) {
              assetFound = true;
              break;
            }
          }
          parsed = tag.length() > 5;
          if (!parsed) error = "Publication GitHub incomplete";
        } else {
          error = "Reponse GitHub illisible";
        }
      } else if (code == HTTP_CODE_NOT_FOUND) {
        error = "Aucune version publiee";
      } else {
        char msg[48];
        snprintf(msg, sizeof(msg), "GitHub HTTP %d", code);
        error = msg;
      }
      http.end();
    }
  } // HTTP/TLS et JSON sont libérés avant de rendre le canvas.

  if (parsed) {
    latestTag = tag;
    publishedDate = date;
    updateAssetFound = assetFound;
    releaseFound = true;
    String commit = latestTag.startsWith("roza-") ? latestTag.substring(5) : "";
    updateAvailable = !commit.startsWith(installedVersion) || installedVersion == "dev";
    resultMessage = !assetFound ? "Paquet USB absent" :
                    (updateAvailable ? "Nouvelle version disponible" : "ROZA est a jour");
  } else {
    releaseFound = false;
    updateAvailable = false;
    updateAssetFound = false;
    resultMessage = error.length() ? error : "Verification impossible";
  }
  checked = true;
  dirty = true;
}

void updateCheckEnter() {
  checked = false;
  releaseFound = false;
  updateAvailable = false;
  updateAssetFound = false;
  resultMessage = "";
  releaseJob.request();
  dirty = true;
}

void updateCheckRequest() {
  checked = false;
  releaseFound = false;
  updateAvailable = false;
  updateAssetFound = false;
  resultMessage = "";
  releaseJob.request();
  dirty = true;
}

void updateCheckUpdate() {
  if (releaseJob.due()) bgFetchRun(fetchLatestRelease);
}

void drawUpdateCheck() {
  releaseJob.markShown();
  cv.fillScreen(TFT_BLACK);
  drawPageHeader("Mise a jour ROZA", nullptr, ROZA_OLIVE_LIGHT);
  cv.setTextSize(1);
  cv.setTextDatum(top_left);
  cv.setTextColor(ROZA_PAPER_DIM, TFT_BLACK);
  cv.drawString("Installee:", 10, 24);
  cv.setTextColor(ROZA_PAPER, TFT_BLACK);
  cv.drawString(installedVersion, 76, 24);

  if (!checked) {
    cv.setTextDatum(middle_center);
    cv.setTextColor(ROZA_OLIVE_LIGHT, TFT_BLACK);
    cv.drawString("Verification GitHub...", SW / 2, 70);
  } else if (!releaseFound) {
    cv.setTextDatum(middle_center);
    cv.setTextColor(TFT_RED, TFT_BLACK);
    cv.drawString(trunc(resultMessage, 34), SW / 2, 68);
  } else {
    String shortTag = latestTag.startsWith("roza-") ? latestTag.substring(5, 17) : latestTag;
    cv.setTextColor(ROZA_PAPER_DIM, TFT_BLACK);
    cv.setTextDatum(top_left);
    cv.drawString("Derniere:", 10, 43);
    cv.setTextColor(ROZA_OLIVE_LIGHT, TFT_BLACK);
    cv.drawString(shortTag, 76, 43);
    cv.setTextColor(updateAvailable ? ROZA_OLIVE_LIGHT : ROZA_PAPER, TFT_BLACK);
    cv.setTextDatum(middle_center);
    cv.drawString(trunc(resultMessage, 36), SW / 2, 65);
    cv.setTextColor(ROZA_PAPER_DIM, TFT_BLACK);
    cv.drawString(updateAssetFound ? "ROZA_UPDATE.bin disponible" : "Paquet USB indisponible", SW / 2, 83);
    if (publishedDate.length() >= 10) cv.drawString(publishedDate.substring(0, 10), SW / 2, 99);
  }
  cv.setTextDatum(bottom_center);
  cv.setTextColor(ROZA_OLIVE, TFT_BLACK);
  cv.drawString("r: verifier   retour: ESC", SW / 2, SH - 2);
}
