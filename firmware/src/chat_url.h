#pragma once
#include <cstdio>
#include <cstring>

// Accept a base URL, a /v1 base, or a complete OpenAI-compatible endpoint.
// HTTPS is mandatory because the request may carry an API key.
static inline bool chatBuildCompletionsUrl(const char* base, char* out, size_t capacity) {
  if (!out || capacity == 0) return false;
  out[0] = 0;
  if (!base || strncmp(base, "https://", 8) != 0) return false;
  if (strstr(base, "COLLER_") || strstr(base, "VOTRE_") || strstr(base, "PLACEHOLDER") ||
      strchr(base, '?') || strchr(base, '#')) return false;
  size_t len = strlen(base);
  while (len > 8 && base[len - 1] == '/') --len;
  if (len <= 8) return false;
  const char* suffix = "/v1/chat/completions";
  const char* v1 = "/v1";
  const char* endpoint = "/chat/completions";
  if (len >= strlen(endpoint) && strcmp(base + len - strlen(endpoint), endpoint) == 0) suffix = "";
  else if (len >= strlen(v1) && strcmp(base + len - strlen(v1), v1) == 0) suffix = "/chat/completions";
  if (len + strlen(suffix) + 1 > capacity) return false;
  memcpy(out, base, len);
  out[len] = 0;
  if (suffix[0]) strcat(out, suffix);
  return true;
}
