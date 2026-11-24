#pragma once
#include <ArduinoJson.h>
#include <cstring>

struct FxRatePoint { char date[11]; float rate; };

static inline bool fxValidIsoDate(const char* date) {
  if (!date || strlen(date) != 10) return false;
  for (int i = 0; i < 10; ++i) {
    if (i == 4 || i == 7) { if (date[i] != '-') return false; }
    else if (date[i] < '0' || date[i] > '9') return false;
  }
  return true;
}

// Frankfurter v2 /rates returns an array of {date, base, quote, rate} rows.
static inline bool fxParseFrankfurterV2(JsonDocument& doc, FxRatePoint* out, int capacity, int& count) {
  count = 0;
  if (!out || capacity <= 0) return false;
  JsonArrayConst rows = doc.as<JsonArrayConst>();
  if (rows.isNull()) return false;
  for (JsonObjectConst row : rows) {
    const char* base = row["base"] | "";
    const char* quote = row["quote"] | "";
    const char* date = row["date"] | "";
    const float rate = row["rate"] | 0.0f;
    if (strcmp(base, "USD") != 0 || strcmp(quote, "CNY") != 0) continue;
    if (!fxValidIsoDate(date) || rate <= 0.0f) continue;
    if (count >= capacity) break;
    strncpy(out[count].date, date, sizeof(out[count].date) - 1);
    out[count].date[sizeof(out[count].date) - 1] = 0;
    out[count].rate = rate;
    ++count;
  }
  // Sort chronologically; the API's array order is not an application invariant.
  for (int i = 1; i < count; ++i) {
    FxRatePoint key = out[i];
    int j = i - 1;
    while (j >= 0 && strcmp(out[j].date, key.date) > 0) { out[j + 1] = out[j]; --j; }
    out[j + 1] = key;
  }
  return count > 0;
}
