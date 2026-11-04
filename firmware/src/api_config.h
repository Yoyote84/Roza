// ROZA optional external-service configuration.
// Private values may be supplied by firmware/src/secrets.h (never commit that file).
// Empty defaults keep every application buildable without accounts, API keys or a server.
#pragma once

#if defined(__has_include)
#  if __has_include("secrets.h")
#    include "secrets.h"
#  endif
#endif

#ifndef CHAT_BASE_URL
#define CHAT_BASE_URL ""
#endif
#ifndef CHAT_API_KEY
#define CHAT_API_KEY ""
#endif
#ifndef CHAT_MODEL
#define CHAT_MODEL "gpt-4o-mini"
#endif
#ifndef CHAT_ROOT_CA
#define CHAT_ROOT_CA ""
#endif
#ifndef CHAT_ALLOW_INSECURE
#define CHAT_ALLOW_INSECURE false
#endif

#ifndef GITHUB_USER
#define GITHUB_USER ""
#endif

#ifndef N2YO_API_KEY
#define N2YO_API_KEY ""
#endif

#ifndef CLASH_BASE
#define CLASH_BASE ""
#endif
#ifndef CLASH_SECRET
#define CLASH_SECRET ""
#endif
#ifndef CLASH_PROXY_GROUP
#define CLASH_PROXY_GROUP ""
#endif
