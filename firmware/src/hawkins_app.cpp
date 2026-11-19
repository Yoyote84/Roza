#include "hawkins_app.h"
#include "hawkins_runtime.h"
#include "hawkins_nvs.h"
#include "hawkins_exploration.h"
#include "hawkins_character.h"
#include "gnss.h"
#include "ui_common.h"

#include <cstdio>
#include <cstring>

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
bool showCharacter = false;
const char* gpsStatusText = "GPS EN ATTENTE";

hawkins::CharacterState player{};
uint32_t playerSequence = 0U;
bool playerLoaded = false;
bool playerStorageError = false;
uint8_t characterCreationStep = 0U;  // 0=nom, 1=classe
uint8_t selectedClass = 0U;
char characterName[hawkins::kCharacterNameCapacity + 1U] = {};
uint8_t characterNameLength = 0U;

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
  cv.drawString("P PERSO   E MONDE   S SAUVER", 5, SH - 2);
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
    cv.drawString("P PERSO   E TERRAIN   S SAUVER", 5, SH - 2);
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
  cv.drawString("P PERSO   E TERRAIN   S SAUVER", 5, SH - 2);
}

void drawCharacterAttributes(const hawkins::CharacterState& character,
                             int firstY) {
  char line[44];
  const char* names[6] = {"FOR", "END", "AGI", "INT", "PER", "CHA"};
  for (int row = 0; row < 3; ++row) {
    snprintf(line, sizeof(line), "%s %u      %s %u",
             names[row * 2],
             unsigned(character.attributes[row * 2]),
             names[row * 2 + 1],
             unsigned(character.attributes[row * 2 + 1]));
    cv.setTextDatum(top_left);
    cv.setTextColor(row == 0 ? ACCENT : TFT_WHITE, TFT_BLACK);
    cv.drawString(line, 8, firstY + row * 12);
  }
}

void drawCharacterPage() {
  cv.fillScreen(TFT_BLACK);
  cv.setFont(&fonts::Font0);
  cv.setTextSize(1);

  if (playerStorageError) {
    drawPageHeader("HAWKINS / PERSONNAGE", "ERREUR NVS", TFT_ORANGE);
    cv.setTextDatum(middle_center);
    cv.setTextColor(TFT_ORANGE, TFT_BLACK);
    cv.drawString("PROFIL INDISPONIBLE", SW / 2, 55);
    cv.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    cv.drawString("Sauvegarde presente mais invalide.", SW / 2, 73);
    cv.drawString("Aucun profil n'a ete efface.", SW / 2, 87);
    return;
  }

  if (!playerLoaded) {
    if (characterCreationStep == 0U) {
      drawPageHeader("HAWKINS / CREATION", "1/2 NOM", ACCENT);
      cv.setTextDatum(top_left);
      cv.setTextColor(TFT_WHITE, TFT_BLACK);
      cv.drawString("CHOISIS TON NOM (12 CAR.)", 8, 24);
      cv.fillRoundRect(7, 43, SW - 14, 23, 3, CARD_BG);
      cv.drawRoundRect(7, 43, SW - 14, 23, 3, ACCENT);
      cv.setTextColor(ACCENT, CARD_BG);
      cv.drawString(characterNameLength ? characterName : "_", 14, 51);
      cv.setTextColor(ICON_DIM, TFT_BLACK);
      cv.drawString("A-Z  0-9  ESPACE  -  _", 8, 80);
      cv.drawString("RETOUR ARRIERE : EFFACER", 8, 94);
      cv.drawString("ENTREE : CHOISIR UNE CLASSE", 8, 108);
    } else {
      const hawkins::CharacterClass& chosen =
          hawkins::characterClass(selectedClass);
      drawPageHeader("HAWKINS / CLASSE", "2/2", ACCENT);
      cv.setTextDatum(middle_center);
      cv.setTextColor(ACCENT, TFT_BLACK);
      cv.setTextSize(2);
      cv.drawString(chosen.name, SW / 2, 32);
      cv.setTextSize(1);
      cv.setTextColor(TFT_WHITE, TFT_BLACK);
      cv.drawString(chosen.role, SW / 2, 52);
      cv.setTextColor(ICON_DIM, TFT_BLACK);
      cv.drawString(chosen.specialty, SW / 2, 65);
      drawCharacterAttributes(
          hawkins::CharacterState{0, 0, selectedClass, 1, 0, 0, 0, 1,
                                  {chosen.attributes[0], chosen.attributes[1],
                                   chosen.attributes[2], chosen.attributes[3],
                                   chosen.attributes[4], chosen.attributes[5]},
                                  {'X', '\0'}}, 82);
      cv.setTextColor(ICON_DIM, TFT_BLACK);
      cv.setTextDatum(bottom_left);
      cv.drawString(", / CHANGER   ENTREE CREER", 5, SH - 2);
    }
    return;
  }

  char title[24];
  snprintf(title, sizeof(title), "NIVEAU %u", unsigned(player.level));
  drawPageHeader("HAWKINS / PERSONNAGE", title, ACCENT);
  cv.setTextDatum(top_left);
  cv.setTextColor(ACCENT, TFT_BLACK);
  cv.drawString(player.name, 8, 18);
  cv.setTextColor(TFT_WHITE, TFT_BLACK);
  cv.drawString(hawkins::characterClass(player.classId).name, 8, 31);

  char line[42];
  snprintf(line, sizeof(line), "XP %lu / %u",
           (unsigned long)player.xp,
           unsigned(hawkins::characterXpForNextLevel(player)));
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString(line, 8, 44);

  snprintf(line, sizeof(line), "PV %u/%u",
           unsigned(player.hitPoints),
           unsigned(hawkins::characterMaxHitPoints(player)));
  cv.setTextColor(TFT_WHITE, TFT_BLACK);
  cv.drawString(line, 8, 58);
  snprintf(line, sizeof(line), "STRESS %u/%u",
           unsigned(player.stress),
           unsigned(hawkins::characterMaxStress(player)));
  cv.setTextColor(TFT_WHITE, TFT_BLACK);
  cv.drawString(line, 126, 58);

  drawCharacterAttributes(player, 78);
  snprintf(line, sizeof(line), "POINTS A DEPENSER : %u",
           unsigned(player.skillPoints));
  cv.setTextDatum(top_left);
  cv.setTextColor(ACCENT, TFT_BLACK);
  cv.drawString(line, 8, 116);
  cv.setTextDatum(bottom_left);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString("P RETOUR   S SAUVER", 5, SH - 2);
}

bool savePlayerState(const hawkins::CharacterState& candidate) {
  uint32_t nextSequence = playerSequence;
  if (!hawkins::characterSaveCommit(candidate, &nextSequence)) return false;
  player = candidate;
  playerSequence = nextSequence;
  playerLoaded = true;
  playerStorageError = false;
  dirty = true;
  return true;
}

void handleCharacterCreationKey(char key) {
  if (characterCreationStep == 0U) {
    if (key == '\n') {
      if (characterNameLength == 0U) {
        memcpy(characterName, "JOUEUR", 6U);
        characterNameLength = 6U;
      }
      characterCreationStep = 1U;
      dirty = true;
      return;
    }
    if (key == '\b') {
      if (characterNameLength > 0U) {
        characterName[--characterNameLength] = '\0';
      }
      dirty = true;
      return;
    }
    char c = key;
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    const bool allowed = (c >= 'A' && c <= 'Z') ||
                         (c >= '0' && c <= '9') ||
                         c == ' ' || c == '-' || c == '_';
    if (allowed && characterNameLength < hawkins::kCharacterNameCapacity) {
      if (characterNameLength == 0U && c == ' ') return;
      characterName[characterNameLength++] = c;
      characterName[characterNameLength] = '\0';
      dirty = true;
    }
    return;
  }

  if (key == ',' || key == ';') {
    selectedClass = static_cast<uint8_t>(
        (selectedClass + hawkins::characterClassCount() - 1U) %
        hawkins::characterClassCount());
    dirty = true;
  } else if (key == '/' || key == '.') {
    selectedClass = static_cast<uint8_t>(
        (selectedClass + 1U) % hawkins::characterClassCount());
    dirty = true;
  } else if (key == '\b') {
    characterCreationStep = 0U;
    dirty = true;
  } else if (key == '\n') {
    hawkins::CharacterState created{};
    if (!hawkins::createCharacter(created, selectedClass, characterName)) {
      setGpsStatus("NOM DE PERSONNAGE INVALIDE");
      return;
    }
    if (savePlayerState(created)) {
      setGpsStatus("PERSONNAGE CREE ET SAUVEGARDE");
    } else {
      playerStorageError = true;
      setGpsStatus("ERREUR SAUVEGARDE PERSONNAGE");
    }
    dirty = true;
  }
}

void awardExplorationExperience(uint16_t amount) {
  if (!playerLoaded || amount == 0U) return;
  hawkins::CharacterState updated = player;
  hawkins::awardCharacterXp(updated, amount);
  if (!savePlayerState(updated)) {
    playerStorageError = true;
    setGpsStatus("ERREUR SAUVEGARDE XP");
  }
}

}  // namespace

void hawkinsAppEnter() {
  candidateActive = false;
  lastSampleMs = 0U;
  explorerReady = false;
  explorerStorageError = false;
  showExploration = true;
  showCharacter = false;
  gpsStatusText = "GPS EN ATTENTE";

  const hawkins::ExplorationSaveLoadStatus exploreLoad =
      hawkins::explorationSaveLoad(exploration, &explorationSequence);
  if (exploreLoad == hawkins::ExplorationSaveLoadStatus::Loaded) {
    explorerReady = true;
  } else if (exploreLoad == hawkins::ExplorationSaveLoadStatus::Empty) {
    hawkins::initializeExploration(exploration);
    explorerReady = hawkins::explorationSaveCommit(
        exploration, &explorationSequence);
    explorerStorageError = !explorerReady;
  } else {
    explorerStorageError = true;
  }

  const hawkins::CharacterSaveLoadStatus characterLoad =
      hawkins::characterSaveLoad(player, &playerSequence);
  playerLoaded = characterLoad == hawkins::CharacterSaveLoadStatus::Loaded;
  playerStorageError = characterLoad == hawkins::CharacterSaveLoadStatus::Corrupt ||
                       characterLoad == hawkins::CharacterSaveLoadStatus::StorageError;
  characterCreationStep = 0U;
  selectedClass = 0U;
  memset(characterName, 0, sizeof(characterName));
  characterNameLength = 0U;
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
    dirty = true;
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
    const uint16_t reward =
        result == hawkins::ExplorationResult::NewDiscovery ? 50U : 25U;
    awardExplorationExperience(reward);
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
  if (showCharacter && !playerLoaded && !playerStorageError) {
    handleCharacterCreationKey(key);
    return;
  }

  if (showCharacter) {
    if (key == 'p' || key == 'P') {
      showCharacter = false;
      dirty = true;
      return;
    }
    if (key == 's' || key == 'S') {
      if (playerLoaded && savePlayerState(player)) {
        setGpsStatus("PERSONNAGE SAUVEGARDE");
      } else {
        setGpsStatus("ERREUR SAUVEGARDE PERSONNAGE");
      }
      dirty = true;
      return;
    }
  }

  if (key == 'p' || key == 'P') {
    showCharacter = true;
    dirty = true;
  } else if (key == 'e' || key == 'E') {
    showExploration = !showExploration;
    dirty = true;
  } else if (key == 's' || key == 'S') {
    const bool worldSaved = hawkins::hawkinsRuntimeSaveNow();
    const bool exploreSaved = explorerReady &&
        hawkins::explorationSaveCommit(exploration, &explorationSequence);
    const bool characterSaved = !playerLoaded ||
        savePlayerState(player);
    setGpsStatus(worldSaved && exploreSaved && characterSaved
                     ? "SAUVEGARDE LOCALE OK"
                     : "ERREUR DE SAUVEGARDE");
    dirty = true;
  }
}

void drawHawkinsApp() {
  if (showCharacter) {
    drawCharacterPage();
    return;
  }
  const hawkins::WorldState* world = hawkins::hawkinsRuntimeWorld();
  if (showExploration) {
    cv.fillScreen(TFT_BLACK);
    drawExplorationPage(world);
  } else {
    drawWorldPage(world);
  }
}
