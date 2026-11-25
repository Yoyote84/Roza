#include "ota_update.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Update.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <mbedtls/base64.h>
#include <mbedtls/pk.h>
#include <mbedtls/sha256.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "bg_fetch.h"
#include "globals.h"
#include "ota_public_key.h"
#include "tls_ca.h"
#include "ui_common.h"
#include "wifi_net.h"

#ifndef ROZA_BUILD_NUMBER
#define ROZA_BUILD_NUMBER 0
#endif
static const char MANIFEST_URL[]="https://yoyote84.github.io/Roza/downloads/ota-manifest.json";
static const char FIRMWARE_URL[]="https://yoyote84.github.io/Roza/downloads/ROZA-OTA.bin";
static const size_t MANIFEST_MAX=2048;
static const size_t SIGNATURE_MAX=128;

enum OtaState:uint8_t{OTA_IDLE,OTA_AVAILABLE,OTA_CURRENT,OTA_ERROR,OTA_INSTALLED};
struct OtaManifest{uint32_t build,size;char commit[41],sha256[65],url[160],signature[180];};
static OtaManifest manifest{};
static OtaState state=OTA_IDLE;
static char message[88]="R : verifier une mise a jour";
static bool manifestReady=false,restartRequested=false;

static void status(OtaState s,const char* text){
 state=s;snprintf(message,sizeof(message),"%s",text?text:"");
}
static bool hexValue(const char* s,size_t n){
 if(!s||strlen(s)!=n)return false;
 for(size_t i=0;i<n;i++)if(!isxdigit((unsigned char)s[i]))return false;
 return true;
}
static bool verifySignature(const OtaManifest& m){
 char payload[512];
 const int n=snprintf(payload,sizeof(payload),
   "ROZA-OTA-MANIFEST-V1\ncardputer-adv\n%lu\n%s\n%lu\n%s\n%s\n",
   (unsigned long)m.build,m.commit,(unsigned long)m.size,m.sha256,m.url);
 if(n<=0||(size_t)n>=sizeof(payload))return false;
 uint8_t sig[SIGNATURE_MAX];size_t sigLen=0;
 if(mbedtls_base64_decode(sig,sizeof(sig),&sigLen,
    (const unsigned char*)m.signature,strlen(m.signature))!=0||!sigLen)return false;
 uint8_t digest[32];
 if(mbedtls_sha256_ret((const unsigned char*)payload,(size_t)n,digest,0)!=0)return false;
 mbedtls_pk_context key;mbedtls_pk_init(&key);
 const int parsed=mbedtls_pk_parse_public_key(&key,
   (const unsigned char*)ROZA_OTA_PUBLIC_KEY_PEM,sizeof(ROZA_OTA_PUBLIC_KEY_PEM));
 if(parsed!=0){mbedtls_pk_free(&key);return false;}
 const int verified=mbedtls_pk_verify(&key,MBEDTLS_MD_SHA256,digest,sizeof(digest),sig,sigLen);
 mbedtls_pk_free(&key);
 return verified==0;
}
static bool parseManifest(const String& body,OtaManifest& m,char* err,size_t errSize){
 if(!body.length()||body.length()>MANIFEST_MAX){snprintf(err,errSize,"Manifeste vide ou trop grand");return false;}
 JsonDocument doc;
 if(deserializeJson(doc,body)){snprintf(err,errSize,"JSON manifeste invalide");return false;}
 if((doc["schema"]|0)!=1||strcmp(doc["board"]|"","cardputer-adv")!=0){
  snprintf(err,errSize,"Cible ou schema incompatible");return false;
 }
 m.build=doc["build"]|0U;m.size=doc["size"]|0U;
 const char* commit=doc["commit"]|"";const char* digest=doc["sha256"]|"";
 const char* url=doc["url"]|"";const char* sig=doc["signature"]|"";
 if(!m.build||m.size<4096||m.size>0x3F0000UL||!hexValue(commit,40)||
    !hexValue(digest,64)||strcmp(url,FIRMWARE_URL)!=0||!sig[0]||
    strlen(sig)>=sizeof(m.signature)){
  snprintf(err,errSize,"Champs du manifeste invalides");return false;
 }
 snprintf(m.commit,sizeof(m.commit),"%s",commit);
 snprintf(m.sha256,sizeof(m.sha256),"%s",digest);
 snprintf(m.url,sizeof(m.url),"%s",url);
 snprintf(m.signature,sizeof(m.signature),"%s",sig);
 if(!verifySignature(m)){snprintf(err,errSize,"Signature ECDSA invalide");return false;}
 return true;
}
static bool fetchManifest(OtaManifest& m,char* err,size_t errSize){
 // OTA ne démarre jamais le Wi-Fi : l'utilisateur doit déjà être connecté.
 if(!wifiEnsureConnected()){snprintf(err,errSize,"Wi-Fi non connecte");return false;}
 if(!tlsClockReady()){snprintf(err,errSize,"Heure requise pour TLS");return false;}
 WiFiClientSecure client;tlsUseCaBundle(client);
 client.setHandshakeTimeout(10);client.setTimeout(12000);
 // Même ordre que les autres clients HTTPS ROZA : créer le bundle CA avant la lease,
 // puis libérer le grand canvas pendant le handshake TLS (Cardputer ADV sans PSRAM).
 CanvasLease lease;
 HTTPClient http;http.useHTTP10(true);http.setConnectTimeout(8000);http.setTimeout(12000);
 http.setUserAgent("ROZA-OTA/1.0");
 if(!http.begin(client,MANIFEST_URL)){snprintf(err,errSize,"Ouverture HTTPS impossible");return false;}
 const int code=http.GET();const int length=http.getSize();
 if(code!=HTTP_CODE_OK||(length>0&&(size_t)length>MANIFEST_MAX)){
  snprintf(err,errSize,"Manifeste HTTP %d",code);http.end();return false;
 }
 String body=http.getString();http.end();
 if(!body.length()||body.length()>MANIFEST_MAX||(length>0&&body.length()!=(size_t)length)){
  snprintf(err,errSize,"Manifeste tronque ou invalide");return false;
 }
 return parseManifest(body,m,err,errSize);
}
static bool isNewer(const OtaManifest& m){
 const uint32_t stored=loadUInt("ota","highest",0);
 const uint32_t current=(uint32_t)ROZA_BUILD_NUMBER;
 return m.build>(stored>current?stored:current);
}
static void checkWorker(){
 bgFetchStatus("Verification signature OTA");
 char err[88]={0};OtaManifest candidate{};
 if(bgFetchCancelled()){status(OTA_ERROR,"Verification annulee");return;}
 if(!fetchManifest(candidate,err,sizeof(err))){status(OTA_ERROR,err);return;}
 const esp_partition_t* target=esp_ota_get_next_update_partition(nullptr);
 if(!target||candidate.size>target->size){status(OTA_ERROR,"Slot OTA absent ou trop petit");return;}
 if(!isNewer(candidate)){manifestReady=false;status(OTA_CURRENT,"Firmware deja a jour ou plus recent");return;}
 manifest=candidate;manifestReady=true;
 char text[88];snprintf(text,sizeof(text),"Build %lu disponible - Y installer",(unsigned long)candidate.build);
 status(OTA_AVAILABLE,text);
}
static void installWorker(){
 bgFetchStatus("Recontrole manifeste signe");
 char err[88]={0};OtaManifest fresh{};
 if(!fetchManifest(fresh,err,sizeof(err))){status(OTA_ERROR,err);return;}
 if(!isNewer(fresh)||fresh.build!=manifest.build||fresh.size!=manifest.size||
    strcmp(fresh.sha256,manifest.sha256)!=0||strcmp(fresh.commit,manifest.commit)!=0){
  status(OTA_ERROR,"Manifeste modifie ou version obsolete");return;
 }
 const esp_partition_t* target=esp_ota_get_next_update_partition(nullptr);
 if(!target||fresh.size>target->size){status(OTA_ERROR,"Partition OTA indisponible");return;}
 if(!wifiEnsureConnected()||!tlsClockReady()){status(OTA_ERROR,"Wi-Fi ou horloge TLS indisponible");return;}
 WiFiClientSecure client;tlsUseCaBundle(client);
 client.setHandshakeTimeout(10);client.setTimeout(15000);
 // Le bundle CA reste alloué avant la lease ; le canvas est libéré pendant le téléchargement.
 CanvasLease lease;
 HTTPClient http;http.useHTTP10(true);http.setConnectTimeout(10000);http.setTimeout(15000);
 http.setUserAgent("ROZA-OTA/1.0");
 if(!http.begin(client,fresh.url)){status(OTA_ERROR,"Ouverture binaire impossible");return;}
 const int code=http.GET(),length=http.getSize();
 if(code!=HTTP_CODE_OK||length<0||(uint32_t)length!=fresh.size){
  snprintf(err,sizeof(err),"Binaire HTTP %d / taille invalide",code);http.end();status(OTA_ERROR,err);return;
 }
 if(!Update.begin(fresh.size,U_FLASH)){
  snprintf(err,sizeof(err),"OTA.begin: %s",Update.errorString());http.end();status(OTA_ERROR,err);return;
 }
 if(!Update.setSHA256(fresh.sha256)){
  Update.abort();http.end();status(OTA_ERROR,"SHA-256 invalide");return;
 }
 Stream& stream=http.getStream();uint8_t buffer[1024];uint32_t remaining=fresh.size;bool ok=true;
 while(remaining){
  if(bgFetchCancelled()){snprintf(err,sizeof(err),"Telechargement annule");ok=false;break;}
  const size_t wanted=remaining<sizeof(buffer)?remaining:sizeof(buffer);
  const size_t got=stream.readBytes(buffer,wanted);
  if(!got){snprintf(err,sizeof(err),"Flux HTTPS interrompu");ok=false;break;}
  const size_t written=Update.write(buffer,got);
  if(written!=got){snprintf(err,sizeof(err),"Ecriture flash: %s",Update.errorString());ok=false;break;}
  remaining-=(uint32_t)got;vTaskDelay(pdMS_TO_TICKS(1));
 }
 http.end();client.stop();
 if(!ok){Update.abort();status(OTA_ERROR,err);return;}
 if(!Update.end(false)){
  snprintf(err,sizeof(err),"Verification SHA-256: %s",Update.errorString());
  Update.abort();status(OTA_ERROR,err);return;
 }
 restartRequested=true;status(OTA_INSTALLED,"Image valide - redemarrage...");
}
void otaEnter(){manifestReady=false;restartRequested=false;status(OTA_IDLE,"R : verifier une mise a jour");dirty=true;}
void drawOta(){
 cv.fillScreen(TFT_BLACK);drawPageHeader("Mise a jour","OTA A/B",ACCENT);
 char line[48];snprintf(line,sizeof(line),"Build actuel : %lu",(unsigned long)ROZA_BUILD_NUMBER);
 cv.setTextDatum(top_left);cv.setTextColor(TFT_WHITE,TFT_BLACK);cv.drawString(line,7,30);
 if(state==OTA_AVAILABLE){
  snprintf(line,sizeof(line),"Nouveau build : %lu",(unsigned long)manifest.build);
  cv.setTextColor(ACCENT,TFT_BLACK);cv.drawString(line,7,48);
  snprintf(line,sizeof(line),"%lu Ko / SHA-256 OK",(unsigned long)(manifest.size/1024));
  cv.setTextColor(TFT_WHITE_DIM,TFT_BLACK);cv.drawString(line,7,64);
 }else{
  cv.setTextColor(state==OTA_ERROR?TFT_RED:TFT_WHITE_DIM,TFT_BLACK);cv.drawString(message,7,52);
 }
 cv.setTextColor(ACCENT,TFT_BLACK);cv.setTextDatum(bottom_left);
 cv.drawString("R/Entree verifier  Y installer",6,SH-2);
 cv.setTextDatum(bottom_right);cv.drawString("Retour",SW-6,SH-2);
}
void otaKey(char key){
 if(key=='\x60'){screen=SCREEN_SETTINGS;dirty=true;return;}
 if(key=='r'||key=='R'||key=='\n'){
  if(bgFetchBusy())return;
  if(!wifiEnsureConnected()){status(OTA_ERROR,"Active Wi-Fi dans Parametres");dirty=true;return;}
  manifestReady=false;bgFetchRun(checkWorker);dirty=true;return;
 }
 if(key=='y'||key=='Y'){
  if(bgFetchBusy())return;
  if(!manifestReady||state!=OTA_AVAILABLE){status(OTA_ERROR,"Verifier une mise a jour avant Y");dirty=true;return;}
  bgFetchRun(installWorker);dirty=true;
 }
}
void otaUpdate(){
 if(!restartRequested||bgFetchBusy())return;
 restartRequested=false;Serial.println("[ota] verified image; rebooting");
 Serial.flush();delay(700);esp_restart();
}
void otaConfirmBoot(){
 const esp_partition_t* running=esp_ota_get_running_partition();if(!running)return;
 esp_ota_img_states_t imageState;
 if(esp_ota_get_state_partition(running,&imageState)==ESP_OK&&imageState==ESP_OTA_IMG_PENDING_VERIFY){
  // L'écran est essentiel. SD, GNSS et SX1262 sont des modules optionnels.
  if(!canvasAvailable()||SW!=240||SH!=135){
   Serial.println("[ota] startup check failed; rolling back");Serial.flush();
   esp_ota_mark_app_invalid_rollback_and_reboot();return;
  }
  const esp_err_t result=esp_ota_mark_app_valid_cancel_rollback();
  if(result!=ESP_OK){
   Serial.printf("[ota] confirmation failed: %s\n",esp_err_to_name(result));Serial.flush();
   esp_ota_mark_app_invalid_rollback_and_reboot();return;
  }
 }
 const uint32_t current=(uint32_t)ROZA_BUILD_NUMBER;
 const uint32_t highest=loadUInt("ota","highest",0);
 if(current>highest)saveUInt("ota","highest",current);
}
