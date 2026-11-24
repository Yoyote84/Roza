#pragma once
#include "WiFiClientSecure.h"

enum followRedirects_t { HTTPC_DISABLE_FOLLOW_REDIRECTS, HTTPC_STRICT_FOLLOW_REDIRECTS, HTTPC_FORCE_FOLLOW_REDIRECTS };

class HTTPClient {
public:
  bool begin(WiFiClient&, const String&) { return false; }
  bool begin(const String&) { return false; }
  void useHTTP10(bool) {}
  void setFollowRedirects(followRedirects_t) {}
  void setReuse(bool) {}
  void setConnectTimeout(int) {}
  void setUserAgent(const char*) {}
  void setUserAgent(const String&) {}
  int GET() { return -1; }
  int POST(const uint8_t*, size_t) { return -1; }
  int POST(const String&) { return -1; }
  String getString() { return ""; }
  Stream& getStream() { static Stream s; return s; }
  Stream* getStreamPtr() { return nullptr; }
  int getSize() { return 0; }
  void end() {}
  void setTimeout(uint16_t) {}
  void addHeader(const String&, const String&) {}
  static String errorToString(int) { return "error"; }
};
