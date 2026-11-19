#include "hawkins_app.h"
#include "hawkins_runtime.h"
#include "hawkins_nvs.h"
#include "hawkins_exploration.h"
#include "gnss.h"
#include "ui_common.h"
#include <cstdio>

namespace {
hawkins::ExplorationState exploration{};
uint32_t explorationSequence = 0U;
uint32_t candidateCellKey = 0U;
uint32_t candidateSinceMs = 0U;
uint32_t lastSampleMs = 0U;
uint32_t gpsSats = 0U;
bool candidateActive = false;
bool explorerReady = false;
bool explorerStorageError = false;
bool showExploration = true;
const char* gpsStatusText = "GPS EN ATTENTE";

void setGpsStatus(const char* text) {
  if (gpsStatusText != text) {
    gpsStatusText = text;
    dirty = true;
  }
}

void drawMetric(int y, const char* label, uint8_t value) {
  char number[5];
  snprintf(number, sizeof(number), "%u", unsigned(value));
  cv.setFont(&fonts::Font0);
  cv.setTextSize(1);
  cv.setTextDatum(middle_left);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString(label, 5, y + 2);
  const int x = 82;
  const int width = 126;
  cv.fillRoundRect(x, y, width, 5, 2, DIM_BORDER);
  const int fill = int((uint16_t(value) * width) / 100U);
  if (fill > 0) cv.fillRoundRect(x, y, fill, 5, 2, ACCENT);
  cv.setTextDatum(middle_right);
  cv.setTextColor(TFT_WHITE, TFT_BLACK);
  cv.drawString(number, SW - 7, y + 2);
}

void drawExplorationPage(const hawkins::WorldState* world) {
  drawPageHeader("HAWKINS / TERRAIN", "HORS LIGNE", ACCENT);
  cv.setFont(&fonts::Font0);
  cv.setTextSize(1);
  cv.setTextDatum(top_left);
  cv.setTextColor(ACCENT, TFT_BLACK);
  cv.drawString("EXPLORATION PHYSIQUE", 5, 18);
  cv.setTextColor(TFT_WHITE, TFT_BLACK);
  cv.drawString(gpsStatusText, 5, 34);
  char line[44];
  snprintf(line, sizeof(line), "SATELLITES : %lu", (unsigned long)gpsSats);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString(line, 5, 48);
  const uint32_t stableSeconds =
      candidateActive ? (millis() - candidateSinceMs) / 1000U : 0U;
  snprintf(line, sizeof(line), "STABILITE : %lu / 10 s",
           (unsigned long)(stableSeconds > 10U ? 10U : stableSeconds));
  cv.drawString(line, 5, 62);
  cv.drawFastHLine(5, 77, SW - 10, DIM_BORDER);
  snprintf(line, sizeof(line), "SECTEURS : %lu",
           (unsigned long)exploration.sectorsVisited);
  cv.setTextColor(TFT_WHITE, TFT_BLACK);
  cv.drawString(line, 5, 84);
  snprintf(line, sizeof(line), "SIGNAUX DECOUVERTS : %lu",
           (unsigned long)exploration.discoveries);
  cv.setTextColor(ACCENT, TFT_BLACK);
  cv.drawString(line, 5, 98);
  const uint32_t day = world ? world->simulatedDays : 0U;
  snprintf(line, sizeof(line), "JOUR DU MONDE : %lu", (unsigned long)day);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString(line, 5, 112);
  cv.setTextDatum(bottom_left);
  cv.drawString("E MONDE   S SAUVER", 5, SH - 2);
}

void drawWorldPage(const hawkins::WorldState* world) {
  cv.fillScreen(TFT_BLACK);
  drawPageHeader("HAWKINS / MONDE", "HORS LIGNE", ACCENT);
  cv.setFont(&fonts::Font0);
  cv.setTextSize(1);
  if (!world) {
    cv.setTextDatum(middle_center);
    cv.setTextColor(TFT_ORANGE, TFT_BLACK);
    cv.drawString(hawkins::hawkinsRuntimeStatusText(), SW / 2, 52);
    cv.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    cv.drawString("Le monde fonctionne sans reseau.", SW / 2, 72);
    cv.drawString("Verifier la memoire interne NVS.", SW / 2, 88);
    cv.setTextDatum(bottom_left);
    cv.setTextColor(ICON_DIM, TFT_BLACK);
    cv.drawString("E TERRAIN   S SAUVER", 5, SH - 2);
    return;
  }
  char line[40];
  snprintf(line, sizeof(line), "JOUR %lu   EVENEMENTS %lu",
           (unsigned long)world->simulatedDays,
           (unsigned long)world->eventCount);
  cv.setTextDatum(top_left);
  cv.setTextColor(TFT_WHITE, TFT_BLACK);
  cv.drawString(line, 5, 18);
  drawMetric(38, "TENSION", world->tension);
  drawMetric(58, "ANOMALIE", world->anomalyLevel);
  drawMetric(78, "FACTIONS", world->factionActivity);
  cv.setTextDatum(top_left);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString("DERNIER EVENEMENT", 5, 98);
  cv.setTextColor(ACCENT, TFT_BLACK);
  cv.drawString(hawkins::eventTypeName(world->latestEvent.type), 5, 111);
  if (world->eventCount > 0U) {
    snprintf(line, sizeof(line), "SECT %02u INT %u",
             unsigned(world->latestEvent.sector),
             unsigned(world->latestEvent.intensity));
    cv.setTextDatum(top_right);
    cv.setTextColor(TFT_WHITE, TFT_BLACK);
    cv.drawString(line, SW - 5, 111);
  }
  if (hawkins::hawkinsRuntimeStatus() != hawkins::RuntimeStatus::Ready) {
    cv.setTextDatum(bottom_right);
    cv.setTextColor(TFT_ORANGE, TFT_BLACK);
    cv.drawString(hawkins::hawkinsRuntimeStatusText(), SW - 5, SH - 2);
  }
  cv.setTextDatum(bottom_left);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString("E TERRAIN   S SAUVER", 5, SH - 2);
}
}  // namespace

void hawkinsAppEnter() {
  candidateActive = false;
  lastSampleMs = 0U;
  explorerReady = false;
  explorerStorageError = false;
  showExploration = true;
  gpsStatusText = "GPS EN ATTENTE";
  const hawkins::ExplorationSaveLoadStatus load =
      hawkins::explorationSaveLoad(exploration, &explorationSequence);
  if (load == hawkins::ExplorationSaveLoadStatus::Loaded) {
    explorerReady = true;
  } else if (load == hawkins::ExplorationSaveLoadStatus::Empty) {
    hawkins::initializeExploration(exploration);
    explorerReady = hawkins::explorationSaveCommit(
        exploration, &explorationSequence);
    explorerStorageError = !explorerReady;
  } else {
    explorerStorageError = true;
  }
  dirty = true;
}

void hawkinsAppUpdate() {
  const uint32_t now = millis();
  if (lastSampleMs != 0U && now - lastSampleMs < 1000U) return;
  lastSampleMs = now;
  if (!explorerReady) {
    setGpsStatus(explorerStorageError ? "ERREUR MEMOIRE NVS"
                                      : "EXPLORATION INDISPONIBLE");
    candidateActive = false;
    return;
  }
  if (!gnssCapConnected()) {
    gpsSats = 0U;
    candidateActive = false;
    setGpsStatus("CAP LORA-1262 ABSENT");
    return;
  }
  gpsSats = static_cast<uint32_t>(gnssSats());
  if (!gnssHasFix(3000U)) {
    candidateActive = false;
    setGpsStatus("RECHERCHE GPS - RESTER DEHORS");
    return;
  }
  if (gpsSats < 5U) {
    candidateActive = false;
    setGpsStatus("SIGNAL GPS TROP FAIBLE");
    return;
  }

  const hawkins::WorldState* world = hawkins::hawkinsRuntimeWorld();
  if (!world) {
    candidateActive = false;
    setGpsStatus("MONDE LOCAL INDISPONIBLE");
    return;
  }
  uint32_t cellKey = 0U;
  if (!hawkins::makeExplorationCellKey(gnssLat(), gnssLng(),
                                       world->seed, cellKey)) {
    candidateActive = false;
    setGpsStatus("POSITION GPS INVALIDE");
    return;
  }
  if (exploration.hasOrigin != 0U &&
      cellKey == exploration.lastCellKey) {
    candidateActive = false;
    setGpsStatus("POSITION STABLE - SECTEUR CONNU");
    return;
  }
  if (!candidateActive || cellKey != candidateCellKey) {
    candidateCellKey = cellKey;
    candidateSinceMs = now;
    candidateActive = true;
    setGpsStatus("POSITION EN VERIFICATION");
    return;
  }
  if (now - candidateSinceMs < hawkins::kExplorationConfirmMs) {
    dirty = true;  // Actualise le compteur de stabilité visible sur l'écran.
    return;
  }

  const hawkins::ExplorationResult result =
      hawkins::observeExplorationCell(exploration, candidateCellKey);
  candidateActive = false;
  if (result == hawkins::ExplorationResult::Invalid) {
    setGpsStatus("POSITION NON VALIDEE");
    return;
  }
  if (result == hawkins::ExplorationResult::SameCell ||
      result == hawkins::ExplorationResult::Revisited) {
    setGpsStatus("SECTEUR DEJA CONNU");
    dirty = true;
    return;
  }
  if (!hawkins::explorationSaveCommit(exploration, &explorationSequence)) {
    explorerStorageError = true;
    explorerReady = false;
    setGpsStatus("ERREUR SAUVEGARDE GPS");
    return;
  }
  if (result == hawkins::ExplorationResult::OriginSet) {
    setGpsStatus("POINT DE DEPART ENREGISTRE");
  } else {
    const bool daySaved = hawkins::hawkinsRuntimeAdvanceDays(1U);
    if (result == hawkins::ExplorationResult::NewDiscovery) {
      setGpsStatus(daySaved ? "SIGNAL ANORMAL DECOUVERT"
                            : "SIGNAL TROUVE - SAUVEGARDE MONDE");
    } else {
      setGpsStatus(daySaved ? "NOUVEAU SECTEUR EXPLORE"
                            : "SECTEUR TROUVE - SAUVEGARDE MONDE");
    }
  }
  dirty = true;
}

void hawkinsAppKey(char key) {
  if (key == 's' || key == 'S') {
    const bool worldSaved = hawkins::hawkinsRuntimeSaveNow();
    const bool exploreSaved = explorerReady &&
        hawkins::explorationSaveCommit(exploration, &explorationSequence);
    setGpsStatus(worldSaved && exploreSaved ? "SAUVEGARDE LOCALE OK"
                                           : "ERREUR DE SAUVEGARDE");
    dirty = true;
  } else if (key == 'e' || key == 'E') {
    showExploration = !showExploration;
    dirty = true;
  }
}

void drawHawkinsApp() {
  const hawkins::WorldState* world = hawkins::hawkinsRuntimeWorld();
  if (showExploration) {
    cv.fillScreen(TFT_BLACK);
    drawExplorationPage(world);
  } else {
    drawWorldPage(world);
  }
}
