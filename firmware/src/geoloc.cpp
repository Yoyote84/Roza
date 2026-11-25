#include "geoloc.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "gnss.h"
#include <ctime>
#include "wifi_net.h"
#include "net_resolve.h"
#include "ui_common.h"
#include "tls_ca.h"

static GeoFix cached;   // 只缓存 IP 定位的结果；GPS 每次现读
static int32_t cachedDay = -1;

// IP 定位结果落 NVS。理由：一次 ip-api 请求（DNS+TCP+HTTP）实测要一秒上下，
// 而它是同步做的——开机后第一次进天气/飞机页就得白等这一秒（实测整个进入过程 3 秒，
// 其中大头就是它）。位置这东西一天内基本不会变，存下来下次开机直接用。
// 用"儒略日"当版本号：跨天了才重新查一次；没对上时间就无条件复用（总比卡一秒强）。
static const char* GEO_NS = "geoloc";

static int32_t todayIndex() {
  if (!timeSynced) return -1;
  return (int32_t)(time(nullptr) / 86400);
}

static bool geoLoadCache() {
  PrefsLock lock;   // 四个键当一个整体读，别跟另一个线程的 end() 交错（见 globals.h）
  double la = loadDouble(GEO_NS, "lat", 1e9);
  double lo = loadDouble(GEO_NS, "lon", 1e9);
  String nm = loadString(GEO_NS, "name", "");
  int32_t day = (int32_t)loadInt(GEO_NS, "day", -1);
  if (la > 1e8 || lo > 1e8) return false;
  if (day < 0) return false;                                  // 无有效日期记录视为过期
  int32_t today = todayIndex();
  if (today >= 0 && day != today) return false;               // 跨天了，重新查
  cached.lat = la; cached.lon = lo; cached.name = nm;
  cached.fromGps = false; cached.valid = true;
  cachedDay = day;
  return true;
}

static void geoSaveCache() {
  PrefsLock lock;   // 同上，四个键当一个整体写
  int32_t today = todayIndex();
  if (today < 0) return;                                      // NTP 未同步时不写 NVS 缓存
  saveDouble(GEO_NS, "lat", cached.lat);
  saveDouble(GEO_NS, "lon", cached.lon);
  saveString(GEO_NS, "name", cached.name);
  saveInt(GEO_NS, "day", today);
}

void geoInvalidate() {
  cached.valid = false;
  cachedDay = -1;
  PrefsLock lock;   // 可能在后台拉取的工人上跑（见 globals.h）
  prefs.begin(GEO_NS, false);
  prefs.clear();
  prefs.end();
}

bool geoGetOffline(GeoFix& out) {
  // 天气之类只要大致位置：10 分钟内的 GPS 定位都比 IP 定位准
  if (gnssHasFix(10UL * 60 * 1000)) {
    out.lat = gnssLat(); out.lon = gnssLng();
    out.name = "GPS"; out.fromGps = true; out.valid = true;
    return true;
  }

  // 跨天后使内存缓存失效
  int32_t today = todayIndex();
  if (cached.valid && today >= 0 && cachedDay >= 0 && today != cachedDay) {
    geoInvalidate();
  }

  if (cached.valid) { out = cached; return true; }
  if (geoLoadCache()) { out = cached; return true; }   // 上次开机查过的，直接用，省掉那一秒
  return false;
}

bool geoGet(GeoFix& out, String* err) {
  if (geoGetOffline(out)) return true;

  if (!wifiEnsureConnected()) { if (err) *err = "wifi not connected"; return false; }

  // ipwho.is propose une API gratuite sans clé et compatible HTTPS. L'ancienne
  // API ip-api.com gratuite est limitée à HTTP : on évite désormais de transmettre
  // des coordonnées de localisation en clair.
  if (!tlsClockReady()) { if (err) *err = "waiting for clock (NTP)"; return false; }
  NetResolveResult nr = netResolve("ipwho.is");
  if (nr != NR_OK) { if (err) *err = String("geo ") + netResolveErrText(nr); return false; }

  if (err) err->reserve(64);
  cached.name.reserve(48);
  double latitude = 0.0, longitude = 0.0;
  String locationName;
  locationName.reserve(48);

  WiFiClientSecure client;
  tlsUseCaBundle(client);
  client.setHandshakeTimeout(10);
  String body;
  body.reserve(512);
  {
    CanvasLease lease;
    HTTPClient http;
    http.setConnectTimeout(8000);
    http.setTimeout(8000);
    http.useHTTP10(true);
    if (!http.begin(client, "https://ipwho.is/?fields=success,message,city,region,latitude,longitude")) {
      if (err) *err = "geo begin failed";
      return false;
    }
    int code = http.GET();
    if (code != 200) {
      if (err) *err = String("geo https ") + code;
      http.end();
      return false;
    }
    body = http.getString();
    http.end();

    JsonDocument doc;
    if (deserializeJson(doc, body)) { if (err) *err = "geo bad json"; return false; }
    if (!(doc["success"] | false)) {
      const char* message = doc["message"] | "lookup failed";
      if (err) *err = String("geo ") + message;
      return false;
    }
    latitude = doc["latitude"] | 0.0;
    longitude = doc["longitude"] | 0.0;
    const char* city = doc["city"] | "";
    const char* region = doc["region"] | "unknown";
    locationName = (city && city[0]) ? city : region;
  }

  cached.lat = latitude;
  cached.lon = longitude;
  cached.name = locationName;
  cached.fromGps = false;
  cached.valid = true;
  cachedDay = todayIndex();
  geoSaveCache();
  out = cached;
  return true;
}
