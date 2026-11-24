#include "../../src/router_config.h"
#include <cassert>
#include <cstdio>

int main() {
  assert(routerBaseConfigStatus("", "") == ROUTER_BASE_MISSING);
  assert(routerBaseConfigStatus("http://ADRESSE_IP_DU_SERVEUR:9090", "secret") == ROUTER_BASE_PLACEHOLDER);
  assert(routerBaseConfigStatus("http://adresse_ip_du_serveur:9090", "secret") == ROUTER_BASE_PLACEHOLDER);
  assert(routerBaseConfigStatus("https://192.168.1.4:9443", "") == ROUTER_BASE_HTTPS_UNSUPPORTED);
  assert(routerBaseConfigStatus("ftp://192.168.1.4:9090", "") == ROUTER_BASE_INVALID_SCHEME);
  assert(routerBaseConfigStatus("http://192.168.1.4:9090", "VOTRE_SECRET_CLASH") == ROUTER_BASE_PLACEHOLDER);
  assert(routerBaseConfigStatus("http://192.168.1.4:9090/", "") == ROUTER_BASE_OK);
  assert(routerProxyGroupConfigured("Proxy"));
  assert(!routerProxyGroupConfigured("VOTRE_GROUPE_DE_PROXY"));
  assert(!routerProxyGroupConfigured(""));
  puts("router controller configuration: all checks passed");
  return 0;
}
