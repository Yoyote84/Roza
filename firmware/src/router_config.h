#pragma once
#include <cstring>
#include <cctype>

enum RouterBaseConfigStatus {
  ROUTER_BASE_OK = 0,
  ROUTER_BASE_MISSING,
  ROUTER_BASE_PLACEHOLDER,
  ROUTER_BASE_HTTPS_UNSUPPORTED,
  ROUTER_BASE_INVALID_SCHEME,
  ROUTER_BASE_INVALID_HOST
};

static inline bool routerContainsIgnoreCase(const char* value, const char* token) {
  if (!value || !token || !token[0]) return false;
  for (const char* p = value; *p; ++p) {
    size_t i = 0;
    while (token[i] && p[i] &&
           tolower((unsigned char)p[i]) == tolower((unsigned char)token[i])) ++i;
    if (!token[i]) return true;
  }
  return false;
}

static inline bool routerValueIsPlaceholder(const char* value) {
  if (!value || !value[0]) return false;
  static const char* const markers[] = {
    "adresse_ip", "votre_", "your_", "placeholder", "change_me"
  };
  for (size_t i = 0; i < sizeof(markers) / sizeof(markers[0]); ++i)
    if (routerContainsIgnoreCase(value, markers[i])) return true;
  return false;
}

static inline RouterBaseConfigStatus routerBaseConfigStatus(const char* base, const char* secret) {
  if (!base || !base[0]) return ROUTER_BASE_MISSING;
  if (strncmp(base, "https://", 8) == 0) return ROUTER_BASE_HTTPS_UNSUPPORTED;
  if (strncmp(base, "http://", 7) != 0) return ROUTER_BASE_INVALID_SCHEME;
  if (routerValueIsPlaceholder(base) || (secret && secret[0] && routerValueIsPlaceholder(secret)))
    return ROUTER_BASE_PLACEHOLDER;
  const char* host = base + 7;
  if (!host[0] || host[0] == '/' || host[0] == ':' || strpbrk(host, " \t\r\n"))
    return ROUTER_BASE_INVALID_HOST;
  const char* end = strpbrk(host, ":/");
  if (end == host) return ROUTER_BASE_INVALID_HOST;
  return ROUTER_BASE_OK;
}

static inline const char* routerBaseConfigMessage(RouterBaseConfigStatus status) {
  switch (status) {
    case ROUTER_BASE_MISSING: return "configure CLASH_BASE";
    case ROUTER_BASE_PLACEHOLDER: return "replace CLASH_BASE / CLASH_SECRET example values";
    case ROUTER_BASE_HTTPS_UNSUPPORTED: return "HTTPS controller not supported (TLS is never bypassed)";
    case ROUTER_BASE_INVALID_SCHEME: return "CLASH_BASE must start with http://";
    case ROUTER_BASE_INVALID_HOST: return "invalid CLASH_BASE host or address";
    default: return "";
  }
}

static inline bool routerProxyGroupConfigured(const char* group) {
  return group && group[0] && !routerValueIsPlaceholder(group);
}
