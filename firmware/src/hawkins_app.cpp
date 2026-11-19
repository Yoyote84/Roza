#include "hawkins_app.h"
#include "hawkins_runtime.h"
#include "ui_common.h"
#include <cstdio>
#include <ctime>
void hawkinsAppEnter(){}
void hawkinsAppKey(char key){if(key=='s'||key=='S'){hawkins::hawkinsRuntimeSaveNow();dirty=true;}}
static void drawMetric(int y,const char* label,uint8_t value){
 char n[5];snprintf(n,sizeof(n),"%u",unsigned(value));cv.setFont(&fonts::Font0);cv.setTextSize(1);
 cv.setTextDatum(middle_left);cv.setTextColor(ICON_DIM,TFT_BLACK);cv.drawString(label,5,y+2);
 const int x=82,w=126;cv.fillRoundRect(x,y,w,5,2,DIM_BORDER);
 int fill=int((uint16_t(value)*w)/100U);if(fill>0)cv.fillRoundRect(x,y,fill,5,2,ACCENT);
 cv.setTextDatum(middle_right);cv.setTextColor(TFT_WHITE,TFT_BLACK);cv.drawString(n,SW-7,y+2);
}
void drawHawkinsApp(){
 cv.fillScreen(TFT_BLACK);char clockText[12]="--:--";
 if(timeSynced){struct tm t{};if(getLocalTime(&t,0))snprintf(clockText,sizeof(clockText),"%02d:%02d",t.tm_hour,t.tm_min);}
 drawPageHeader("HAWKINS / MONDE",clockText,ICON_DIM);
 const hawkins::WorldState* world=hawkins::hawkinsRuntimeWorld();
 if(!world){
  cv.setFont(&fonts::Font0);cv.setTextSize(1);cv.setTextDatum(middle_center);
  cv.setTextColor(ACCENT,TFT_BLACK);cv.drawString(hawkins::hawkinsRuntimeStatusText(),SW/2,48);
  cv.setTextColor(TFT_LIGHTGREY,TFT_BLACK);
  hawkins::RuntimeStatus s=hawkins::hawkinsRuntimeStatus();
  if(s==hawkins::RuntimeStatus::Empty||s==hawkins::RuntimeStatus::WaitingForClock){
   cv.drawString("GPS, NTP ou date valide requis",SW/2,68);
   cv.drawString("Le monde sera cree automatiquement",SW/2,84);
   cv.drawString("quand l'horloge sera disponible.",SW/2,96);
  }else{
   cv.drawString("Aucune sauvegarde n'a ete effacee.",SW/2,76);
   cv.drawString("Verifier la memoire NVS avant reprise.",SW/2,92);
  }
 }else{
  char line[40];snprintf(line,sizeof(line),"JOUR %lu   EVENEMENTS %lu",
   (unsigned long)world->simulatedDays,(unsigned long)world->eventCount);
  cv.setFont(&fonts::Font0);cv.setTextSize(1);cv.setTextDatum(top_left);
  cv.setTextColor(TFT_WHITE,TFT_BLACK);cv.drawString(line,5,18);
  drawMetric(38,"TENSION",world->tension);drawMetric(58,"ANOMALIE",world->anomalyLevel);
  drawMetric(78,"FACTIONS",world->factionActivity);
  cv.setTextDatum(top_left);cv.setTextColor(ICON_DIM,TFT_BLACK);cv.drawString("DERNIER EVENEMENT",5,98);
  cv.setTextColor(ACCENT,TFT_BLACK);cv.drawString(hawkins::eventTypeName(world->latestEvent.type),5,111);
  if(world->eventCount>0){
   snprintf(line,sizeof(line),"SECT %02u INT %u",unsigned(world->latestEvent.sector),unsigned(world->latestEvent.intensity));
   cv.setTextDatum(top_right);cv.setTextColor(TFT_WHITE,TFT_BLACK);cv.drawString(line,SW-5,111);
  }
  if(hawkins::hawkinsRuntimeStatus()!=hawkins::RuntimeStatus::Ready){
   cv.setTextDatum(bottom_right);cv.setTextColor(TFT_ORANGE,TFT_BLACK);
   cv.drawString(hawkins::hawkinsRuntimeStatusText(),SW-5,SH-2);
  }
 }
 cv.setTextDatum(bottom_left);cv.setTextColor(ICON_DIM,TFT_BLACK);cv.drawString("BACK     S SAVE",5,SH-2);
}
