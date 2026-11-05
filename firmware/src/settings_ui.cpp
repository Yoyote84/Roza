#include "settings_ui.h"
#include <WiFi.h>
#include <ctime>
#include <sys/time.h>
#include "icons.h"
#include "sd_files.h"
#include "led.h"
#include "power_util.h"
#include "ui_common.h"
#include "roza_theme.h"
#include "weather.h"
#include "ram_profile.h"
#include "list_sel.h"
#include <esp_partition.h>
#include <esp_ota_ops.h>

// 设置项右侧的数值/状态
String setValue(SetId id) {
  char b[24];
  switch (id) {
    case SET_WIFI:    return String(WiFi.status() == WL_CONNECTED ? "ALLUMÉ" : "ÉTEINT");
    case SET_GNSS:    return String();
    case SET_PCMODE:  return String("USB link for PC");
    case SET_BRIGHT:  snprintf(b, sizeof(b), "%d%%", brightPct); return String(b);
    case SET_VOL:
      if (debugOn) return String("MUTE (DBG)");
      snprintf(b, sizeof(b), volPct == 0 ? "MUET" : "%d%%", volPct); return String(b);
    case SET_BOOT_SOUND: return String(bootSoundOn ? "OUI" : "NON");
    case SET_LED:     return String(ledModeName());
    case SET_THEME:   return String(themeName(themeMode));
    case SET_SLEEP: {
      int s = SLEEP_OPTS[sleepOptIdx];
      if (s == 0) return String("Jamais");
      snprintf(b, sizeof(b), "%ds", s); return String(b);
    }
    case SET_TZ:      return String(timeSynced ? "OK" : "A regler");
    case SET_WX_UNIT: return String(weatherImperial ? "Imperial" : "Metric");
    case SET_BATTERY: {
      int lvl = powerBatteryLevel();
      if (lvl < 0) return String("--");
      snprintf(b, sizeof(b), "%d%%", lvl); return String(b);
    }
    case SET_DEBUG:   return String(debugOn ? "OUI" : "NON");
    case SET_FORMAT:  return String(sdMounted ? "Effacer >" : "Pas de SD");
    case SET_ABOUT:   return String("Infos >");
  }
  return String();
}

static const int SET_PER_PAGE = 4;   // 每页 4 项，15 项共 4 页

void drawSettings() {
  cv.fillScreen(TFT_BLACK);

  int page = settingsIndex / SET_PER_PAGE;
  int pageCount = (SET_COUNT + SET_PER_PAGE - 1) / SET_PER_PAGE;
  int startIdx = page * SET_PER_PAGE;

  char pageStr[32];
  snprintf(pageStr, sizeof(pageStr), "%u/%u", (unsigned)(page + 1), (unsigned)pageCount);
  drawPageHeader("Parametres", pageStr, ROZA_OLIVE_LIGHT);

  const int startY = 16, cardH = 24, gap = 2;
  const int cardX = 4, cardW = SW - 8;

  for (int p = 0; p < SET_PER_PAGE; p++) {
    int i = startIdx + p;
    if (i >= SET_COUNT) break;
    int y = startY + p * (cardH + gap);
    bool sel = (i == settingsIndex);

    // 1. 卡片背景与边框
    if (sel) {
      cv.fillRoundRect(cardX, y, cardW, cardH, 3, ROZA_CARD);
      cv.drawRoundRect(cardX, y, cardW, cardH, 3, ACCENT);
      cv.drawRoundRect(cardX + 1, y + 1, cardW - 2, cardH - 2, 2, ROZA_OLIVE_LIGHT);
    } else {
      cv.fillRoundRect(cardX, y, cardW, cardH, 3, ROZA_INK);
      cv.drawRoundRect(cardX, y, cardW, cardH, 3, ROZA_OLIVE_DARK);
    }

    // 2. 左侧图标与设置项名称
    int midY = y + cardH / 2;
    drawSetIcon(cardX + 14, midY, 7, sel ? ACCENT : ROZA_OLIVE, SETTINGS[i].id);

    cv.setTextColor(sel ? ROZA_PAPER : ROZA_PAPER_DIM, sel ? ROZA_CARD : ROZA_INK);
    cv.setTextDatum(middle_left); cv.setTextSize(1);
    cv.drawString(SETTINGS[i].name, cardX + 28, midY);

    // 3. 右侧状态微胶囊 (Pill Badge)
    int rx = cardX + cardW - 6;
    SetId sid = SETTINGS[i].id;

    if (sid == SET_WIFI || sid == SET_DEBUG || sid == SET_BOOT_SOUND) {
      // 开关型项
      bool on = false;
      if (sid == SET_WIFI) on = (WiFi.status() == WL_CONNECTED);
      else if (sid == SET_DEBUG) on = debugOn;
      else if (sid == SET_BOOT_SOUND) on = bootSoundOn;

      const int bw = 32, bh = 14, bx = rx - bw, by = y + 5;
      if (on) {
        uint16_t onCol = ROZA_OLIVE_LIGHT;
        cv.fillRoundRect(bx, by, bw, bh, 2, ROZA_INK);
        cv.drawRoundRect(bx, by, bw, bh, 2, onCol);
        cv.setTextColor(onCol, ROZA_INK);
        cv.setTextDatum(middle_center);
        cv.drawString("OUI", bx + bw / 2, by + 7);
      } else {
        cv.fillRoundRect(bx, by, bw, bh, 2, ROZA_INK);
        cv.drawRoundRect(bx, by, bw, bh, 2, ROZA_OLIVE_DARK);
        cv.setTextColor(ROZA_PAPER_DIM, ROZA_INK);
        cv.setTextDatum(middle_center);
        cv.drawString("NON", bx + bw / 2, by + 7);
      }
    } else if (sid == SET_LED) {
      // LED 灯效模式微胶囊
      const char* mnm = ledModeName();
      bool on = ledIsOn();
      int bw = strlen(mnm) * 6 + 10;
      if (bw < 32) bw = 32;
      int bx = rx - bw, by = y + 5;
      uint16_t col = on ? ROZA_OLIVE_LIGHT : ROZA_PAPER_DIM;
      uint16_t borderCol = on ? ROZA_OLIVE_LIGHT : ROZA_OLIVE_DARK;
      cv.fillRoundRect(bx, by, bw, 14, 2, ROZA_INK);
      cv.drawRoundRect(bx, by, bw, 14, 2, borderCol);
      cv.setTextColor(col, ROZA_INK);
      cv.setTextDatum(middle_center);
      cv.drawString(mnm, bx + bw / 2, by + 7);
    } else if (sid == SET_THEME) {
      // 主题项：胶囊使用对应主题颜色
      const char* tnm = themeName(themeMode);
      uint16_t tcol = (themeMode == 0) ? ROZA_OLIVE_LIGHT : getGroupTheme(themeMode - 1).primary;
      int bw = strlen(tnm) * 6 + 10;
      if (bw < 36) bw = 36;
      int bx = rx - bw, by = y + 5;
      cv.fillRoundRect(bx, by, bw, 14, 2, ROZA_INK);
      cv.drawRoundRect(bx, by, bw, 14, 2, tcol);
      cv.setTextColor(tcol, ROZA_INK);
      cv.setTextDatum(middle_center);
      cv.drawString(tnm, bx + bw / 2, by + 7);
    } else if (sid == SET_BATTERY) {
      int lvl = powerBatteryLevel();
      uint16_t bcol = (lvl > 50) ? ROZA_OLIVE_LIGHT : (lvl > 20) ? TFT_YELLOW : TFT_RED;
      char bb[16]; snprintf(bb, sizeof(bb), "%d%%%s", lvl, powerCharging() ? "+" : "");
      int bw = strlen(bb) * 6 + 10;
      if (bw < 34) bw = 34;
      int bx = rx - bw, by = y + 5;
      cv.fillRoundRect(bx, by, bw, 14, 2, ROZA_INK);
      cv.drawRoundRect(bx, by, bw, 14, 2, bcol);
      cv.setTextColor(bcol, ROZA_INK);
      cv.setTextDatum(middle_center);
      cv.drawString(bb, bx + bw / 2, by + 7);
    } else if (sid == SET_FORMAT || sid == SET_ABOUT || sid == SET_GNSS || sid == SET_PCMODE) {
      String val = setValue(sid);
    } else {
      // 其它数值/档位项 (Bright, Vol, Sleep, Tz, Units)
      String val = setValue(sid);
      int bw = val.length() * 6 + 10;
      if (bw < 34) bw = 34;
      int bx = rx - bw, by = y + 5;
      uint16_t vcol = sel ? ROZA_OLIVE_LIGHT : ROZA_PAPER_DIM;
      cv.fillRoundRect(bx, by, bw, 14, 2, ROZA_INK);
      cv.drawRoundRect(bx, by, bw, 14, 2, sel ? vcol : ROZA_OLIVE_DARK);
      cv.setTextColor(vcol, ROZA_INK);
      cv.setTextDatum(middle_center);
      cv.drawString(val, bx + bw / 2, by + 7);
    }
  }

  // 底部辅助导航与页码点 (y = 125..134)
  drawPageDots(page, pageCount);
  cv.setTextDatum(bottom_left); cv.setTextSize(1);
  cv.setTextColor(ROZA_OLIVE, TFT_BLACK);
  cv.drawString("; . naviguer", 6, SH - 2);

  cv.setTextDatum(bottom_right);
  cv.drawString("Entree : OK", SW - 6, SH - 2);
}

// 通用：段式霓虹发光刻度计（亮度/音量共用）
void drawBar(const char* title, int pct, const char* hint) {
  cv.fillScreen(TFT_BLACK);

  char pctStr[16]; snprintf(pctStr, sizeof(pctStr), "%d%%", pct);
  drawPageHeader(title, pctStr, ACCENT);

  // 居中大卡片
  const int cx = 10, cy = 18, cw = SW - 20, ch = 96;
  cv.fillRoundRect(cx, cy, cw, ch, 4, ROZA_INK);
  cv.drawRoundRect(cx, cy, cw, ch, 4, ROZA_OLIVE_DARK);

  // 图标 + 百分比读数
  if (strstr(title, "Bright") != nullptr) {
    icoSun(32, 40, 10, ACCENT);
  } else {
    icoSpeaker(32, 40, 10, ACCENT);
  }

  cv.setTextDatum(middle_left); cv.setTextSize(2);
  cv.setTextColor(ROZA_PAPER, ROZA_INK);
  cv.drawString(pctStr, 52, 40);

  // 12 段式霓虹水平刻度计 (y = 62)
  const int nSeg = 12;
  const int segW = 12, segH = 18, segGap = 4;
  int startX = cx + (cw - (nSeg * (segW + segGap) - segGap)) / 2;
  int filled = (pct * nSeg + 50) / 100;

  for (int s = 0; s < nSeg; s++) {
    int sx = startX + s * (segW + segGap);
    if (s < filled) {
      cv.fillRoundRect(sx, 62, segW, segH, 2, ACCENT);
      cv.drawFastVLine(sx + 1, 64, segH - 4, ROZA_PAPER);
    } else {
      cv.fillRoundRect(sx, 62, segW, segH, 2, ROZA_CARD);
      cv.drawRoundRect(sx, 62, segW, segH, 2, ROZA_OLIVE_DARK);
    }
  }

  // 刻度标注
  cv.setTextColor(ROZA_OLIVE, ROZA_INK); cv.setTextDatum(top_left); cv.setTextSize(1);
  cv.drawString("MIN", startX, 83);
  cv.setTextDatum(top_right);
  cv.drawString("MAX", startX + nSeg * (segW + segGap) - segGap, 83);

  // 底部操作提示
  cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
  cv.setTextDatum(bottom_center); cv.setTextSize(1);
  cv.drawString(hint, SW / 2, SH - 2);
}

// 熄屏等待时长：标准 HUD 外框与时间档位胶囊
void drawSleep() {
  cv.fillScreen(TFT_BLACK);

  int s = SLEEP_OPTS[sleepOptIdx];
  char b[16];
  if (s == 0) snprintf(b, sizeof(b), "Jamais"); else snprintf(b, sizeof(b), "%ds", s);
  drawPageHeader("Veille ecran", b, ACCENT);

  const int cx = 10, cy = 18, cw = SW - 20, ch = 96;
  cv.fillRoundRect(cx, cy, cw, ch, 4, ROZA_INK);
  cv.drawRoundRect(cx, cy, cw, ch, 4, ROZA_OLIVE_DARK);

  cv.setTextColor(ACCENT, ROZA_INK);
  cv.setTextDatum(middle_center); cv.setTextSize(3);
  cv.drawString(b, SW / 2, 46);

  // 6 个档位胶囊排开 (y = 74)
  const int pCount = SLEEP_OPT_COUNT;
  const int pw = 30, ph = 14, pgap = 4;
  int startX = cx + (cw - (pCount * (pw + pgap) - pgap)) / 2;

  for (int i = 0; i < pCount; i++) {
    int px = startX + i * (pw + pgap);
    bool on = (i == sleepOptIdx);
    char tb[8];
    if (SLEEP_OPTS[i] == 0) snprintf(tb, sizeof(tb), "Arret");
    else                    snprintf(tb, sizeof(tb), "%ds", SLEEP_OPTS[i]);

    if (on) {
      cv.fillRoundRect(px, 74, pw, ph, 2, ACCENT);
      cv.setTextColor(TFT_BLACK, ACCENT);
    } else {
      cv.fillRoundRect(px, 74, pw, ph, 2, ROZA_CARD);
      cv.drawRoundRect(px, 74, pw, ph, 2, ROZA_OLIVE_DARK);
      cv.setTextColor(ROZA_PAPER_DIM, ROZA_CARD);
    }
    cv.setTextDatum(middle_center); cv.setTextSize(1);
    cv.drawString(tb, px + pw / 2, 74 + ph / 2);
  }

  cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
  cv.setTextDatum(bottom_center); cv.setTextSize(1);
  cv.drawString("; . prec./suiv.    ` retour", SW / 2, SH - 2);
}

static bool manualTimeEditing = false;
static struct tm manualTimeValue{};
static int manualTimeField = 0;
static int manualDaysInMonth(int y, int m) {
  static const int d[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (m == 2 && ((y % 4 == 0 && y % 100 != 0) || y % 400 == 0)) return 29;
  return d[m - 1];
}
void manualTimeBegin() {
  time_t now = time(nullptr);
  if (now >= 1735689600) localtime_r(&now, &manualTimeValue);
  else { memset(&manualTimeValue, 0, sizeof(manualTimeValue)); manualTimeValue.tm_year=126; manualTimeValue.tm_mday=1; manualTimeValue.tm_hour=12; }
  if (manualTimeValue.tm_year < 125) manualTimeValue.tm_year = 125;  // 2025 minimum
  if (manualTimeValue.tm_year > 199) manualTimeValue.tm_year = 199;  // 2099 maximum
  manualTimeField=0; manualTimeEditing=true; dirty=true;
}
bool manualTimeEditingNow() { return manualTimeEditing; }
void manualTimeKey(char k) {
  if (!manualTimeEditing) return;
  if ((uint8_t)k == 96) { manualTimeEditing=false; dirty=true; return; }
  if (k==',') manualTimeField=(manualTimeField+4)%5;
  else if (k=='/') manualTimeField=(manualTimeField+1)%5;
  else if (k=='-' || k=='_') {
    if(manualTimeField==0) manualTimeValue.tm_year=max(125,manualTimeValue.tm_year-1);
    else if(manualTimeField==1) manualTimeValue.tm_mon=(manualTimeValue.tm_mon+11)%12;
    else if(manualTimeField==2) manualTimeValue.tm_mday=manualTimeValue.tm_mday>1?manualTimeValue.tm_mday-1:manualDaysInMonth(manualTimeValue.tm_year+1900,manualTimeValue.tm_mon+1);
    else if(manualTimeField==3) manualTimeValue.tm_hour=(manualTimeValue.tm_hour+23)%24;
    else manualTimeValue.tm_min=(manualTimeValue.tm_min+59)%60;
  } else if(k=='+' || k=='=') {
    if(manualTimeField==0) manualTimeValue.tm_year=min(199,manualTimeValue.tm_year+1);
    else if(manualTimeField==1) manualTimeValue.tm_mon=(manualTimeValue.tm_mon+1)%12;
    else if(manualTimeField==2) manualTimeValue.tm_mday=manualTimeValue.tm_mday<manualDaysInMonth(manualTimeValue.tm_year+1900,manualTimeValue.tm_mon+1)?manualTimeValue.tm_mday+1:1;
    else if(manualTimeField==3) manualTimeValue.tm_hour=(manualTimeValue.tm_hour+1)%24;
    else manualTimeValue.tm_min=(manualTimeValue.tm_min+1)%60;
  } else if(k==10) {
    manualTimeValue.tm_sec=0; manualTimeValue.tm_isdst=-1;
    time_t epoch=mktime(&manualTimeValue);
    if(epoch>=1735689600) { struct timeval tv={epoch,0}; if(settimeofday(&tv,nullptr)==0) { saveUInt("clock","manual_epoch",(uint32_t)epoch); timeSynced=true; timeFromGps=false; } }
    manualTimeEditing=false;
  }
  int md=manualDaysInMonth(manualTimeValue.tm_year+1900,manualTimeValue.tm_mon+1);
  if(manualTimeValue.tm_mday>md) manualTimeValue.tm_mday=md;
  dirty=true;
}
void drawTz() {
  cv.fillScreen(TFT_BLACK);
  if(manualTimeEditing) {
    drawPageHeader("Reglage manuel","DATE / HEURE",ACCENT);
    const char* names[]={"ANNEE","MOIS","JOUR","HEURE","MIN"};
    char dateBuf[24],timeBuf[16],dayBuf[24];
    snprintf(dateBuf,sizeof(dateBuf),"%04d-%02d-%02d",manualTimeValue.tm_year+1900,manualTimeValue.tm_mon+1,manualTimeValue.tm_mday);
    snprintf(timeBuf,sizeof(timeBuf),"%02d:%02d",manualTimeValue.tm_hour,manualTimeValue.tm_min);
    struct tm p=manualTimeValue; p.tm_isdst=-1; time_t ep=mktime(&p); struct tm wd{}; localtime_r(&ep,&wd);
    static const char* days[]={"Dimanche","Lundi","Mardi","Mercredi","Jeudi","Vendredi","Samedi"};
    snprintf(dayBuf,sizeof(dayBuf),"%s",days[wd.tm_wday]);
    cv.fillRoundRect(8,20,SW-16,82,4,ROZA_INK); cv.drawRoundRect(8,20,SW-16,82,4,ROZA_OLIVE_DARK);
    cv.setTextDatum(middle_center); cv.setTextSize(2); cv.setTextColor(ROZA_PAPER,ROZA_INK); cv.drawString(dateBuf,SW/2,43);
    cv.setTextSize(3); cv.setTextColor(ACCENT,ROZA_INK); cv.drawString(timeBuf,SW/2,72);
    cv.setTextSize(1); cv.setTextColor(ROZA_OLIVE_LIGHT,ROZA_INK); cv.drawString(dayBuf,SW/2,91);
    cv.setTextColor(TFT_YELLOW,TFT_BLACK); cv.setTextDatum(bottom_center); cv.drawString(names[manualTimeField],SW/2,115);
    cv.setTextColor(TFT_DARKGREY,TFT_BLACK); cv.setTextDatum(bottom_center);
    cv.drawString(", / champ  - / + valeur",SW/2,SH-10);
    cv.drawString("Entree sauver  retour annuler",SW/2,SH-2);
    return;
  }

  char pageBuf[16];
  snprintf(pageBuf, sizeof(pageBuf), "%d/%d", tzOptIdx + 1, TZ_OPT_COUNT);
  drawPageHeader("Date/heure", pageBuf, ACCENT);

  const int cx = 10, cy = 18, cw = SW - 20, ch = 96;
  cv.fillRoundRect(cx, cy, cw, ch, 4, ROZA_INK);
  cv.drawRoundRect(cx, cy, cw, ch, 4, ROZA_OLIVE_DARK);

  cv.setTextColor(ROZA_PAPER, ROZA_INK);
  cv.setTextDatum(middle_center); cv.setTextSize(2);
  cv.drawString(TZ_OPTS[tzOptIdx].label, SW / 2, 46);

  cv.setTextColor(ROZA_OLIVE_LIGHT, ROZA_INK);
  cv.setTextDatum(middle_center); cv.setTextSize(1);
  char posixBuf[40];
  snprintf(posixBuf, sizeof(posixBuf), "Regle: %s", TZ_OPTS[tzOptIdx].posix);
  cv.drawString(posixBuf, SW / 2, 74);

  cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
  cv.setTextDatum(bottom_center); cv.setTextSize(1);
  cv.drawString("; . : fuseau horaire", SW / 2, SH - 10);
  cv.drawString("m : regler date/heure   retour", SW / 2, SH - 2);
}

// 电池与电源仪表盘：双联硬件卡片
void drawBatteryDetail() {
  cv.fillScreen(TFT_BLACK);

  int lvl = powerBatteryLevel();
  int16_t mv = M5.Power.getBatteryVoltage();
  bool charging = powerCharging();

  drawPageHeader("Batterie & alim.", charging ? "EN CHARGE" : "DECHARGE", charging ? ROZA_OLIVE_LIGHT : TFT_YELLOW);

  // 卡片 1 (左)：实时电量与状态
  const int c1w = 113, c2w = 114;
  const int bpy = 16, bph = 102;
  cv.fillRoundRect(4, bpy, c1w, bph, 4, ROZA_INK);
  cv.drawRoundRect(4, bpy, c1w, bph, 4, ROZA_OLIVE_DARK);

  cv.setTextDatum(top_left); cv.setTextSize(1);
  cv.setTextColor(ACCENT, ROZA_INK);
  cv.drawString("ETAT EN DIRECT", 8, bpy + 4);
  cv.drawFastHLine(6, bpy + 14, c1w - 4, ROZA_CARD);

  // 电池电芯图标 (x = 10, y = 36)
  int bx = 10, by = bpy + 22, bw = 38, bh = 18;
  cv.drawRoundRect(bx, by, bw, bh, 3, ROZA_PAPER_DIM);
  cv.fillRect(bx + bw, by + 5, 3, bh - 10, ROZA_PAPER_DIM);
  if (lvl >= 0) {
    int fillW = (bw - 4) * constrain(lvl, 0, 100) / 100;
    uint16_t bcol = lvl <= 20 ? TFT_RED : (lvl <= 50 ? TFT_ORANGE : ACCENT);
    cv.fillRoundRect(bx + 2, by + 2, fillW, bh - 4, 1, bcol);
  }

  // 大字电量
  char b[32];
  if (lvl >= 0) snprintf(b, sizeof(b), "%d%%", lvl);
  else          snprintf(b, sizeof(b), "--%%");
  cv.setTextColor(ROZA_PAPER, ROZA_INK); cv.setTextSize(2);
  cv.drawString(b, 54, bpy + 23);

  // Autonomie indicative : afficher aussi les minutes pour éviter un faux "0h".
  char t[16];
  int rem = powerRemainingTime();
  if (rem < 0) {
    snprintf(t, sizeof(t), "--");
  } else if (rem < 60) {
    snprintf(t, sizeof(t), "%dm", rem);
  } else {
    snprintf(t, sizeof(t), "%dh%02dm", rem / 60, rem % 60);
  }
  cv.setTextColor(0xFFAA00, ROZA_INK); cv.setTextSize(1);
  cv.drawString(t, 54, bpy + 44);

  // 端电压
  cv.setTextSize(1);
  if (mv >= 0) snprintf(b, sizeof(b), "Tension: %.2f V", mv / 1000.0f);
  else         snprintf(b, sizeof(b), "Tension: n/d");
  cv.setTextColor(ROZA_OLIVE_LIGHT, ROZA_INK);
  cv.drawString(b, 10, bpy + 48);

  // 充放电状态胶囊
  const int sbw = c1w - 18, sbh = 16, sbx = 9, sby = bpy + 68;
  if (charging) {
    cv.fillRoundRect(sbx, sby, sbw, sbh, 2, ROZA_INK);
    cv.drawRoundRect(sbx, sby, sbw, sbh, 2, ROZA_OLIVE_LIGHT);
    cv.setTextColor(ROZA_OLIVE_LIGHT, ROZA_INK);
    cv.setTextDatum(middle_center);
    cv.drawString("EN CHARGE +", sbx + sbw / 2, sby + sbh / 2);
  } else {
    cv.fillRoundRect(sbx, sby, sbw, sbh, 2, ROZA_INK);
    cv.drawRoundRect(sbx, sby, sbw, sbh, 2, ROZA_OLIVE_DARK);
    cv.setTextColor(ROZA_PAPER_DIM, ROZA_INK);
    cv.setTextDatum(middle_center);
    cv.drawString("DECHARGE", sbx + sbw / 2, sby + sbh / 2);
  }

  // 卡片 2 (右)：硬件规格与设计参数
  const int c2x = 121;
  cv.fillRoundRect(c2x, bpy, c2w, bph, 4, ROZA_INK);
  cv.drawRoundRect(c2x, bpy, c2w, bph, 4, ROZA_OLIVE_DARK);

  cv.setTextDatum(top_left); cv.setTextSize(1);
  cv.setTextColor(ROZA_OLIVE_LIGHT, ROZA_INK);
  cv.drawString("SPECS ALIM.", c2x + 6, bpy + 4);
  cv.drawFastHLine(c2x + 2, bpy + 14, c2w - 4, ROZA_CARD);

  int y2 = bpy + 18; const int lh = 13;
  cv.setTextColor(ROZA_PAPER_DIM, ROZA_INK);
  cv.drawString("PMIC : TP4057", c2x + 6, y2); y2 += lh;
  cv.drawString("Cap. : ~1400 mAh", c2x + 6, y2); y2 += lh;
  cv.drawString("Pile : 1S Li-Po", c2x + 6, y2); y2 += lh;
  cv.drawString("V_min: 3.30 V", c2x + 6, y2); y2 += lh;
  cv.drawString("V_max: 4.20 V", c2x + 6, y2); y2 += lh;
  cv.drawString("Est. courbe V", c2x + 6, y2);

  // 底部提示
  cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
  cv.setTextDatum(bottom_center); cv.setTextSize(1);
  cv.drawString("`  retour reglages", SW / 2, SH - 2);
}

void drawFormat() {
  cv.fillScreen(TFT_BLACK);
  if (!sdMounted) {
    cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
    cv.setTextDatum(middle_center); cv.setTextSize(2);
    cv.drawString("Carte SD absente", SW / 2, SH / 2 - 6);
    return;
  }
  // 顶部醒目警告条
  cv.fillRect(0, 0, SW, 20, TFT_RED);
  cv.setTextColor(ROZA_PAPER, TFT_RED);
  cv.setTextDatum(middle_center); cv.setTextSize(2);
  cv.drawString(formatStep == 0 ? "Formater SD ?" : "CONFIRMER ?", SW / 2, 10);

  cv.setTextColor(ROZA_PAPER, TFT_BLACK);
  cv.setTextDatum(middle_center); cv.setTextSize(1);
  if (formatStep == 0) {
    cv.drawString("Efface toutes donnees", SW / 2, 48);
    char c[32]; snprintf(c, sizeof(c), "sur carte %.1f Go.", sdSizeMB / 1024.0);
    cv.drawString(c, SW / 2, 64);
    cv.setTextColor(TFT_YELLOW, TFT_BLACK);
    cv.drawString("Touche Y : suite", SW / 2, 92);
  } else {
    cv.setTextColor(TFT_RED, TFT_BLACK);
    cv.drawString("TOUT SERA EFFACE !", SW / 2, 52);
    cv.setTextColor(TFT_YELLOW, TFT_BLACK);
    cv.drawString("Touche Y : FORMATER", SW / 2, 84);
  }
  cv.setTextColor(TFT_DARKGREY, TFT_BLACK);
  cv.setTextDatum(bottom_center);
  cv.drawString("`  annuler", SW / 2, SH - 2);
}

// 一行：标签 + 占比进度条 + 百分比，下面再跟一行"已用/总量"细节
static void drawUsageBar(int y, const char* label, int pct, const String& detail) {
  cv.setTextColor(ROZA_PAPER, TFT_BLACK);
  cv.setTextDatum(top_left); cv.setTextSize(1);
  cv.drawString(label, 8, y);
  const int barX = 44, barW = SW - 44 - 34, barH = 10;
  cv.drawRect(barX, y, barW, barH, ROZA_OLIVE_DARK);
  uint16_t col = pct >= 85 ? TFT_RED : (pct >= 65 ? TFT_ORANGE : ACCENT);
  cv.fillRect(barX + 1, y + 1, (barW - 2) * constrain(pct, 0, 100) / 100, barH - 2, col);
  char pb[8]; snprintf(pb, sizeof(pb), "%d%%", pct);
  cv.setTextColor(col, TFT_BLACK);
  cv.setTextDatum(top_right);
  cv.drawString(pb, SW - 6, y);
  cv.setTextColor(ROZA_OLIVE, TFT_BLACK);
  cv.setTextDatum(top_left);
  cv.drawString(detail, barX, y + 12);
}

int aboutPage = 0;   // 0=硬件规格  1=RAM/ROM/SD 用量  2=开机内存轨迹
const int ABOUT_PAGE_COUNT = 3;
int aboutRamScroll = 0;

void aboutRamScrollMove(int dir) {
  const int VISIBLE_ROWS = 6;
  int count = ramMarkCount();
  int maxScroll = count > VISIBLE_ROWS ? count - VISIBLE_ROWS : 0;
  aboutRamScroll += dir;
  if (aboutRamScroll > maxScroll) aboutRamScroll = maxScroll;
  if (aboutRamScroll < 0) aboutRamScroll = 0;
}

void aboutRamReset() {
  aboutRamScroll = 0;
}

static void drawAboutInfo() {
  // 卡片 1 (左)：计算核心
  const int c1w = 113, c2w = 114;
  const int bpy = 16, bph = 102;
  cv.fillRoundRect(4, bpy, c1w, bph, 4, ROZA_INK);
  cv.drawRoundRect(4, bpy, c1w, bph, 4, ROZA_OLIVE_DARK);

  cv.setTextDatum(top_left); cv.setTextSize(1);
  cv.setTextColor(ACCENT, ROZA_INK);
  cv.drawString("CALCUL", 8, bpy + 4);
  cv.drawFastHLine(6, bpy + 14, c1w - 4, ROZA_CARD);

  int y1 = bpy + 18; const int lh = 13;
  cv.setTextColor(ROZA_PAPER_DIM, ROZA_INK);
  cv.drawString("MCU : ESP32-S3", 8, y1); y1 += lh;
  cv.drawString("Freq: 240 MHz", 8, y1); y1 += lh;
  cv.drawString("Arch: Xtensa LX7", 8, y1); y1 += lh;
  cv.drawString("Coeur: double", 8, y1); y1 += lh;
  cv.drawString("SRAM: 320 KB", 8, y1); y1 += lh;
  cv.drawString("PSRAM: aucune", 8, y1);

  // 卡片 2 (右)：外设与系统
  const int c2x = 121;
  cv.fillRoundRect(c2x, bpy, c2w, bph, 4, ROZA_INK);
  cv.drawRoundRect(c2x, bpy, c2w, bph, 4, ROZA_OLIVE_DARK);

  cv.setTextDatum(top_left); cv.setTextSize(1);
  cv.setTextColor(ROZA_OLIVE_LIGHT, ROZA_INK);
  cv.drawString("SYSTEME & E/S", c2x + 6, bpy + 4);
  cv.drawFastHLine(c2x + 2, bpy + 14, c2w - 4, ROZA_CARD);

  int y2 = bpy + 18;
  cv.setTextColor(ROZA_PAPER_DIM, ROZA_INK);
  cv.drawString("Flash : 8 MB QSPI", c2x + 6, y2); y2 += lh;
  cv.drawString("LCD   : 1.14\" IPS", c2x + 6, y2); y2 += lh;
  cv.drawString("Keys  : TCA8418", c2x + 6, y2); y2 += lh;
  cv.drawString("Audio : NS4168", c2x + 6, y2); y2 += lh;
  cv.drawString("Sync  : NTP & GPS", c2x + 6, y2); y2 += lh;
  cv.drawString("Base : Slacker v4", c2x + 6, y2);
}

static void drawAboutUsage() {
  int y = 16;

  uint32_t ramTotal = ESP.getHeapSize(), ramFree = ESP.getFreeHeap();
  uint32_t ramUsed = ramTotal - ramFree;
  int ramPct = ramTotal ? (int)(((uint64_t)ramUsed * 100) / ramTotal) : 0;
  drawUsageBar(y, "RAM", ramPct, fmtBytes(ramUsed) + " / " + fmtBytes(ramTotal)); y += 26;

  uint32_t romUsed = ESP.getSketchSize();
  const esp_partition_t *running = esp_ota_get_running_partition();
  if (!running) running = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, nullptr);
  uint32_t romTotal = running ? running->size : (romUsed + ESP.getFreeSketchSpace());
  int romPct = romTotal ? (int)(((uint64_t)romUsed * 100) / romTotal) : 0;
  drawUsageBar(y, "ROM", romPct, fmtBytes(romUsed) + " / " + fmtBytes(romTotal)); y += 26;

  if (sdMounted) {
    int sdPct = sdSizeMB ? (int)(sdUsedMB * 100 / sdSizeMB) : 0;
    char det[24]; snprintf(det, sizeof(det), "%.1fG / %.1fG", sdUsedMB / 1024.0, sdSizeMB / 1024.0);
    drawUsageBar(y, "SD", sdPct, det);
  } else {
    drawUsageBar(y, "SD", 0, "No card");
  }
  y += 26;

  // 底部芯片指标卡片
  cv.fillRoundRect(4, y, SW - 8, 20, 3, ROZA_INK);
  cv.drawRoundRect(4, y, SW - 8, 20, 3, ROZA_OLIVE_DARK);

  cv.setTextDatum(middle_left); cv.setTextSize(1);
  cv.setTextColor(ROZA_OLIVE_LIGHT, ROZA_INK);
  cv.drawString("RAM mini: " + fmtBytes(ESP.getMinFreeHeap()), 10, y + 10);

  char temp[24]; snprintf(temp, sizeof(temp), "Temp: %.0f C", temperatureRead());
  cv.setTextDatum(middle_right);
  cv.setTextColor(TFT_YELLOW, ROZA_INK);
  cv.drawString(temp, SW - 10, y + 10);
}

void drawAboutRam() {
  const int cardX = 4, cardY = 16, cardW = SW - 8, cardH = 102;
  cv.fillRoundRect(cardX, cardY, cardW, cardH, 4, ROZA_INK);
  cv.drawRoundRect(cardX, cardY, cardW, cardH, 4, ROZA_OLIVE_DARK);

  int count = ramMarkCount();
  if (count == 0) {
    cv.setTextColor(ROZA_OLIVE, ROZA_INK);
    cv.setTextDatum(middle_center); cv.setTextSize(1);
    cv.drawString("Aucun releve RAM", SW / 2, cardY + cardH / 2);

    cv.setTextDatum(bottom_left);
    cv.drawString("; . page", 6, SH - 2);
    return;
  }

  // 表头
  const int hdrY = cardY + 7;
  cv.setTextSize(1);
  cv.setTextColor(ACCENT, ROZA_INK);

  cv.setTextDatum(middle_left);
  cv.drawString("Etape", cardX + 4, hdrY);

  cv.setTextDatum(middle_right);
  cv.drawString("free8", 107, hdrY);
  cv.drawString("dFree", 149, hdrY);
  cv.drawString("larg8", 188, hdrY);
  cv.drawString("dLarg", 228, hdrY);

  cv.drawFastHLine(cardX + 2, cardY + 15, cardW - 4, ROZA_CARD);

  // 列表范围与滚动
  const int VISIBLE_ROWS = 6;
  listClampScroll(aboutRamScroll, aboutRamScroll, count, VISIBLE_ROWS);

  const int rowH = 14;
  const int startY = cardY + 16;

  for (int r = 0; r < VISIBLE_ROWS; r++) {
    int idx = aboutRamScroll + r;
    if (idx >= count) break;
    const RamMark& m = ramMarkAt(idx);

    int rowY = startY + r * rowH;
    int midY = rowY + 7;

    // 阶段名（最长 10 字符，例如 "wifi start"）
    char sBuf[12];
    snprintf(sBuf, sizeof(sBuf), "%.10s", m.stage);
    cv.setTextDatum(middle_left);
    cv.setTextColor(ROZA_PAPER_DIM, ROZA_INK);
    cv.drawString(sBuf, cardX + 4, midY);

    // free8
    char fb[12];
    snprintf(fb, sizeof(fb), "%lu", (unsigned long)m.free8);
    cv.setTextDatum(middle_right);
    cv.setTextColor(ROZA_OLIVE_LIGHT, ROZA_INK);
    cv.drawString(fb, 107, midY);

    // dFree
    if (idx == 0) {
      cv.setTextColor(ROZA_OLIVE, ROZA_INK);
      cv.drawString("-", 149, midY);
    } else {
      long dFree = (long)m.free8 - (long)ramMarkAt(idx - 1).free8;
      char dfb[14];
      snprintf(dfb, sizeof(dfb), "%+ld", dFree);
      uint16_t dfCol = (dFree > 0) ? ROZA_OLIVE_LIGHT : (dFree < 0 ? 0xFDA0 : ROZA_OLIVE);
      cv.setTextColor(dfCol, ROZA_INK);
      cv.drawString(dfb, 149, midY);
    }

    // largest8
    char lb[12];
    snprintf(lb, sizeof(lb), "%lu", (unsigned long)m.largest8);
    cv.setTextColor(ROZA_OLIVE_LIGHT, ROZA_INK);
    cv.drawString(lb, 188, midY);

    // dLarg
    if (idx == 0) {
      cv.setTextColor(ROZA_OLIVE, ROZA_INK);
      cv.drawString("-", 228, midY);
    } else {
      long dLarg = (long)m.largest8 - (long)ramMarkAt(idx - 1).largest8;
      char dlb[14];
      snprintf(dlb, sizeof(dlb), "%+ld", dLarg);
      uint16_t dlCol = (dLarg > 0) ? ROZA_OLIVE_LIGHT : (dLarg < 0 ? 0xFDA0 : ROZA_OLIVE);
      cv.setTextColor(dlCol, ROZA_INK);
      cv.drawString(dlb, 228, midY);
    }
  }

  // 滚动条（超过 VISIBLE_ROWS 时绘制）
  if (count > VISIBLE_ROWS) {
    drawScrollBar(SW - 7, startY, VISIBLE_ROWS * rowH, aboutRamScroll, count, VISIBLE_ROWS, ACCENT, ROZA_OLIVE_DARK);
  }

  // 底部提示
  cv.setTextDatum(bottom_left); cv.setTextSize(1);
  cv.setTextColor(ROZA_OLIVE, TFT_BLACK);
  cv.drawString("; . page", 6, SH - 2);

  cv.setTextDatum(bottom_right);
  if (count > VISIBLE_ROWS) {
    cv.drawString("[ ] defil.", SW - 6, SH - 2);
  } else if (count >= 2) {
    char totBuf[16];
    long total = (long)ramMarkAt(count - 1).free8 - (long)ramMarkAt(0).free8;
    snprintf(totBuf, sizeof(totBuf), "%+ldB", total);
    cv.drawString(totBuf, SW - 6, SH - 2);
  }
}

void drawAbout() {
  cv.fillScreen(TFT_BLACK);
  char pageStr[16];
  snprintf(pageStr, sizeof(pageStr), "%d/%d", aboutPage + 1, ABOUT_PAGE_COUNT);
  drawPageHeader("A propos systeme", pageStr, ACCENT);

  if (aboutPage == 0)      drawAboutInfo();
  else if (aboutPage == 1) drawAboutUsage();
  else                     drawAboutRam();

  drawPageDots(aboutPage, ABOUT_PAGE_COUNT);
}
