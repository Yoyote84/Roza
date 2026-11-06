#include "roza_runtime.h"
#include <SD.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include "globals.h"

namespace {
static const size_t MAX_LUA_HEAP = 64 * 1024;
static const size_t MAX_LUA_SOURCE = 96 * 1024;
static const uint32_t CALLBACK_BUDGET_MS = 100;
static const uint32_t LOOP_INTERVAL_MS = 50;
static lua_State* state = nullptr;
static size_t luaHeapUsed = 0;
static bool running = false;
static bool errorVisible = false;
static bool exitRequested = false;
static uint32_t loopAt = 0;
static uint32_t callbackStartedAt = 0;
static char lastKey[12] = "";
static char runtimeError[96] = "";

struct SdReader {
  File* file;
  uint8_t buffer[512];
  size_t total;
  bool tooLarge;
};

static void copyText(char* out, size_t capacity, const char* value) {
  if (capacity) snprintf(out, capacity, "%s", value ? value : "");
}

static void closeState() {
  if (state) {
    lua_close(state);
    state = nullptr;
  }
  luaHeapUsed = 0;
  running = false;
}

static void setError(const char* message) {
  char saved[96];
  copyText(saved, sizeof(saved), message && message[0] ? message : "Erreur runtime Lua");
  closeState();
  copyText(runtimeError, sizeof(runtimeError), saved);
  errorVisible = true;
  exitRequested = false;
}

static void* limitedAllocator(void*, void* pointer, size_t oldSize, size_t newSize) {
  if (newSize == 0) {
    if (pointer) {
      free(pointer);
      if (oldSize <= luaHeapUsed) luaHeapUsed -= oldSize;
      else luaHeapUsed = 0;
    }
    return nullptr;
  }
  size_t retained = luaHeapUsed;
  if (pointer) {
    if (oldSize <= retained) retained -= oldSize;
    else retained = 0;
  }
  if (newSize > MAX_LUA_HEAP || retained > MAX_LUA_HEAP - newSize) return nullptr;
  void* replacement = realloc(pointer, newSize);
  if (!replacement) return nullptr;
  luaHeapUsed = retained + newSize;
  return replacement;
}

static void instructionBudget(lua_State* L, lua_Debug*) {
  if ((uint32_t)(millis() - callbackStartedAt) > CALLBACK_BUDGET_MS) {
    luaL_error(L, "limite de temps depassee (100 ms)");
  }
}

static const char* readFromSd(lua_State*, void* userData, size_t* size) {
  SdReader* reader = static_cast<SdReader*>(userData);
  if (!reader || !reader->file || !size) return nullptr;
  const int count = reader->file->read(reader->buffer, sizeof(reader->buffer));
  if (count <= 0) {
    *size = 0;
    return nullptr;
  }
  reader->total += (size_t)count;
  if (reader->total > MAX_LUA_SOURCE) {
    reader->tooLarge = true;
    *size = 0;
    return nullptr;
  }
  *size = (size_t)count;
  return reinterpret_cast<const char*>(reader->buffer);
}

static bool validId(const char* value) {
  if (!value || !value[0] || strlen(value) > 24) return false;
  for (const char* p = value; *p; ++p) {
    const char c = *p;
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
  }
  return true;
}

static bool validEntry(const char* value) {
  if (!value || !value[0] || strlen(value) > 23 || value[0] == '.') return false;
  for (const char* p = value; *p; ++p) {
    const char c = *p;
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return false;
  }
  return strcmp(value, "manifest.json") != 0 && strstr(value, "..") == nullptr;
}

static void openSafeLibrary(const char* name, lua_CFunction openFunction) {
  luaL_requiref(state, name, openFunction, 1);
  lua_pop(state, 1);
}

static void removeGlobal(const char* name) {
  lua_pushnil(state);
  lua_setglobal(state, name);
}

static void removeStringFunction(const char* name) {
  lua_getglobal(state, LUA_STRLIBNAME);
  if (lua_istable(state, -1)) {
    lua_pushnil(state);
    lua_setfield(state, -2, name);
  }
  lua_pop(state, 1);
}

static int apiClear(lua_State* L) {
  const uint16_t color = (uint16_t)luaL_optinteger(L, 1, TFT_BLACK);
  cv.fillScreen(color);
  return 0;
}

static int apiText(lua_State* L) {
  const char* value = luaL_checkstring(L, 1);
  const int x = (int)luaL_checkinteger(L, 2);
  const int y = (int)luaL_checkinteger(L, 3);
  const uint16_t color = (uint16_t)luaL_optinteger(L, 4, TFT_WHITE);
  cv.setTextDatum(top_left);
  cv.setTextColor(color, TFT_BLACK);
  cv.drawString(value, x, y);
  return 0;
}

static int apiTextSize(lua_State* L) {
  int size = (int)luaL_checkinteger(L, 1);
  if (size < 1) size = 1;
  if (size > 2) size = 2;
  cv.setTextSize((uint8_t)size);
  return 0;
}

static int apiRect(lua_State* L) {
  const int x = (int)luaL_checkinteger(L, 1);
  const int y = (int)luaL_checkinteger(L, 2);
  const int w = (int)luaL_checkinteger(L, 3);
  const int h = (int)luaL_checkinteger(L, 4);
  const uint16_t color = (uint16_t)luaL_optinteger(L, 5, TFT_WHITE);
  if (w > 0 && h > 0) cv.drawRect(x, y, w, h, color);
  return 0;
}

static int apiFillRect(lua_State* L) {
  const int x = (int)luaL_checkinteger(L, 1);
  const int y = (int)luaL_checkinteger(L, 2);
  const int w = (int)luaL_checkinteger(L, 3);
  const int h = (int)luaL_checkinteger(L, 4);
  const uint16_t color = (uint16_t)luaL_optinteger(L, 5, TFT_WHITE);
  if (w > 0 && h > 0) cv.fillRect(x, y, w, h, color);
  return 0;
}

static int apiLine(lua_State* L) {
  const int x1 = (int)luaL_checkinteger(L, 1);
  const int y1 = (int)luaL_checkinteger(L, 2);
  const int x2 = (int)luaL_checkinteger(L, 3);
  const int y2 = (int)luaL_checkinteger(L, 4);
  const uint16_t color = (uint16_t)luaL_optinteger(L, 5, TFT_WHITE);
  cv.drawLine(x1, y1, x2, y2, color);
  return 0;
}

static int apiWidth(lua_State* L) { lua_pushinteger(L, SW); return 1; }
static int apiHeight(lua_State* L) { lua_pushinteger(L, SH); return 1; }
static int apiMillis(lua_State* L) { lua_pushinteger(L, millis()); return 1; }
static int apiKey(lua_State* L) { lua_pushstring(L, lastKey); return 1; }
static int apiExit(lua_State*) { exitRequested = true; return 0; }

static void registerApi() {
  lua_newtable(state);
  lua_pushcfunction(state, apiClear); lua_setfield(state, -2, "clear");
  lua_pushcfunction(state, apiText); lua_setfield(state, -2, "text");
  lua_pushcfunction(state, apiTextSize); lua_setfield(state, -2, "text_size");
  lua_pushcfunction(state, apiRect); lua_setfield(state, -2, "rect");
  lua_pushcfunction(state, apiFillRect); lua_setfield(state, -2, "fill_rect");
  lua_pushcfunction(state, apiLine); lua_setfield(state, -2, "line");
  lua_pushcfunction(state, apiWidth); lua_setfield(state, -2, "width");
  lua_pushcfunction(state, apiHeight); lua_setfield(state, -2, "height");
  lua_pushcfunction(state, apiMillis); lua_setfield(state, -2, "millis");
  lua_pushcfunction(state, apiKey); lua_setfield(state, -2, "key");
  lua_pushcfunction(state, apiExit); lua_setfield(state, -2, "exit");
  lua_setglobal(state, "roza");
}

static void prepareEnvironment() {
  openSafeLibrary(LUA_GNAME, luaopen_base);
  openSafeLibrary(LUA_MATHLIBNAME, luaopen_math);
  openSafeLibrary(LUA_STRLIBNAME, luaopen_string);
  openSafeLibrary(LUA_TABLIBNAME, luaopen_table);
  openSafeLibrary(LUA_UTF8LIBNAME, luaopen_utf8);
  removeGlobal("dofile");
  removeGlobal("loadfile");
  removeGlobal("load");
  removeGlobal("collectgarbage");
  removeGlobal("pcall");
  removeGlobal("xpcall");
  removeGlobal("print");
  removeGlobal("warn");
  removeStringFunction("find");
  removeStringFunction("match");
  removeStringFunction("gmatch");
  removeStringFunction("gsub");
  registerApi();
}

static bool callGlobal(const char* name, const char* argument = nullptr) {
  if (!state || !running) return false;
  lua_getglobal(state, name);
  if (!lua_isfunction(state, -1)) {
    lua_pop(state, 1);
    return true;
  }
  int argumentCount = 0;
  if (argument) {
    lua_pushstring(state, argument);
    argumentCount = 1;
  }
  callbackStartedAt = millis();
  lua_sethook(state, instructionBudget, LUA_MASKCOUNT, 1000);
  const int status = lua_pcall(state, argumentCount, 0, 0);
  lua_sethook(state, nullptr, 0, 0);
  if (status != LUA_OK) {
    const char* message = lua_tostring(state, -1);
    char saved[96];
    copyText(saved, sizeof(saved), message ? message : "Erreur Lua");
    setError(saved);
    return false;
  }
  if (exitRequested) {
    closeState();
    errorVisible = false;
    exitRequested = false;
    runtimeError[0] = 0;
    return false;
  }
  return true;
}

static void drawError() {
  cv.fillScreen(TFT_BLACK);
  cv.setTextDatum(top_left);
  cv.setTextSize(1);
  cv.setTextColor(TFT_RED, TFT_BLACK);
  cv.drawString("Erreur application Lua", 8, 18);
  cv.setTextColor(TFT_WHITE, TFT_BLACK);
  cv.drawString(runtimeError, 8, 40);
  cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
  cv.drawString("Retour : touche retour", 8, SH - 13);
}
} // namespace

bool rozaRuntimeStart(const char* appId, const char* entryName) {
  rozaRuntimeStop();
  if (!validId(appId) || !validEntry(entryName)) {
    setError("Identifiant ou fichier d'entree invalide");
    return false;
  }
  const String path = String("/ROZA/apps/") + appId + "/" + entryName;
  File file = SD.open(path.c_str(), FILE_READ);
  if (!file || file.isDirectory()) {
    if (file) file.close();
    setError("Script Lua introuvable sur microSD");
    return false;
  }
  if (file.size() == 0 || file.size() > MAX_LUA_SOURCE) {
    file.close();
    setError("Taille du script invalide (max. 96 Kio)");
    return false;
  }
  luaHeapUsed = 0;
  state = lua_newstate(limitedAllocator, nullptr);
  if (!state) {
    file.close();
    setError("Memoire insuffisante pour Lua (64 Kio max.)");
    return false;
  }
  errorVisible = false;
  runtimeError[0] = 0;
  exitRequested = false;
  lastKey[0] = 0;
  prepareEnvironment();

  SdReader reader;
  reader.file = &file;
  reader.total = 0;
  reader.tooLarge = false;
  const int loadStatus = lua_load(state, readFromSd, &reader, entryName, "t");
  file.close();
  if (reader.tooLarge) {
    setError("Script trop volumineux (max. 96 Kio)");
    return false;
  }
  if (loadStatus != LUA_OK) {
    const char* message = lua_tostring(state, -1);
    setError(message ? message : "Impossible de compiler le script Lua");
    return false;
  }
  running = true;
  callbackStartedAt = millis();
  lua_sethook(state, instructionBudget, LUA_MASKCOUNT, 1000);
  const int runStatus = lua_pcall(state, 0, 0, 0);
  lua_sethook(state, nullptr, 0, 0);
  if (runStatus != LUA_OK) {
    const char* message = lua_tostring(state, -1);
    setError(message ? message : "Erreur au chargement du script");
    return false;
  }
  if (exitRequested) {
    closeState();
    return false;
  }
  lua_getglobal(state, "draw");
  const bool hasDraw = lua_isfunction(state, -1);
  lua_pop(state, 1);
  if (!hasDraw) {
    setError("Le script doit definir draw()");
    return false;
  }
  running = true;
  loopAt = millis();
  if (!callGlobal("setup")) return false;
  return running;
}

void rozaRuntimeStop() {
  closeState();
  errorVisible = false;
  exitRequested = false;
  runtimeError[0] = 0;
  lastKey[0] = 0;
}

bool rozaRuntimeActive() { return running && state != nullptr; }
bool rozaRuntimeVisible() { return rozaRuntimeActive() || errorVisible; }

void rozaRuntimeKey(char key) {
  if (!rozaRuntimeActive()) return;
  const char* name = nullptr;
  if (key == 10 || key == '\n' || key == '\r') name = "ENTER";
  else if (key == 8 || key == 127) name = "BACKSPACE";
  else {
    lastKey[0] = key;
    lastKey[1] = 0;
  }
  if (name) copyText(lastKey, sizeof(lastKey), name);
  callGlobal("on_key", lastKey);
}

void rozaRuntimeUpdate() {
  if (!rozaRuntimeActive()) return;
  if ((uint32_t)(millis() - loopAt) < LOOP_INTERVAL_MS) return;
  loopAt = millis();
  callGlobal("loop");
}

void rozaRuntimeDraw() {
  if (errorVisible) { drawError(); return; }
  if (!rozaRuntimeActive()) return;
  cv.fillScreen(TFT_BLACK);
  cv.setTextDatum(top_left);
  cv.setTextSize(1);
  callGlobal("draw");
  if (errorVisible) { drawError(); return; }
  if (rozaRuntimeActive()) {
    cv.setTextDatum(bottom_right);
    cv.setTextSize(1);
    cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
    cv.drawString("Retour : touche retour", SW - 5, SH - 2);
  }
}

const char* rozaRuntimeLastError() { return runtimeError; }
