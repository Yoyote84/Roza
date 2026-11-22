#include "hawkins_app.h"
#include "hawkins_runtime.h"
#include "hawkins_ai_protocol.h"  // tailles bornées des buffers de narration/choix
#include "hawkins_nvs.h"
#include "hawkins_exploration.h"
#include "hawkins_character.h"
#include "hawkins_missions.h"
#include "hawkins_adventure.h"
#include "hawkins_adventure_store.h"
#include "hawkins_narrative_store.h"
#include "hawkins_llm/runtime.h"
#include "hawkins_sd_backup.h"
#include "sd_files.h"
#include "gnss.h"
#include "ui_common.h"

#include <cstdio>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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

hawkins::MissionState missions{};
uint32_t missionSequence = 0U;
bool missionsReady = false;
bool missionsStorageError = false;
bool showMissions = false;

hawkins::AdventureState adventure{};
uint32_t adventureSequence = 0U;
bool adventureReady = false;
bool adventureStorageError = false;

hawkins::NarrativeMemory narrative{};
uint32_t narrativeSequence = 0U;
bool narrativeReady = false;
bool narrativeStorageError = false;
bool showInventory = false;
bool showSkills = false;
bool showFirstContact = false;
bool showStorage = false;
bool restoreConfirmation = false;

bool showAiNarration = false;
bool aiPending = false;
bool aiWorkerRunning = false;
bool aiWorkerDone = false;
bool aiWorkerOk = false;
bool aiTimedOut = false;
uint32_t aiRequestAtMs = 0U;
char aiNarration[hawkins::kHawkinsAiTextCapacity + 1U] = {};
char aiChoices[hawkins::kHawkinsAiChoiceCount][hawkins::kHawkinsAiChoiceCapacity + 1U] = {};
uint8_t aiChoiceCount = 0U;
char aiUserAction[hawkins::kHawkinsAiChoiceCapacity + 1U] = {};
char aiWorkerPrompt[256] = {};
char aiWorkerOutput[hawkins::kHawkinsAiTextCapacity + 1U] = {};
portMUX_TYPE aiWorkerMux = portMUX_INITIALIZER_UNLOCKED;
const char* aiStatusText = "IA LOCALE EN ATTENTE";

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
  const bool eventHere = exploration.hasOrigin != 0U &&
      exploration.lastEventCellKey == exploration.lastCellKey &&
      exploration.areaEventCount > 0U;
  if (eventHere) {
    snprintf(line, sizeof(line), "ICI : %s  INT %u",
             hawkins::eventTypeName(exploration.lastEventType),
             unsigned(exploration.lastEventIntensity));
    cv.setTextColor(ACCENT, TFT_BLACK);
  } else if (exploration.hasOrigin != 0U) {
    snprintf(line, sizeof(line), "ZONE %08lX : AUCUN SIGNAL",
             static_cast<unsigned long>(exploration.lastCellKey));
    cv.setTextColor(ICON_DIM, TFT_BLACK);
  } else {
    snprintf(line, sizeof(line), "ZONE EN ATTENTE DU POINT GPS");
    cv.setTextColor(ICON_DIM, TFT_BLACK);
  }
  cv.drawString(line, 5, 112);
  cv.setTextDatum(bottom_left);
  cv.drawString("P PERSO M MISS I OBJ K COMP A IA Q SCN", 5, SH - 2);
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
    cv.drawString("P PERSO M MISS I OBJ K COMP Q SCN D SD", 5, SH - 2);
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
  cv.drawString("DERNIER EVENEMENT MONDE", 5, 98);
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
  cv.drawString("P PERSO E TERRAIN M MISS A IA S SAVE", 5, SH - 2);
}

const char* currentMovementObjective() {
  if (!missionsReady) return "MISSIONS NON DISPONIBLES";
  const bool eventHere = exploration.hasOrigin != 0U &&
      exploration.lastEventCellKey == exploration.lastCellKey &&
      exploration.areaEventCount > 0U;

  if (hawkins::missionStatus(missions, hawkins::MissionId::FirstSignal) ==
      hawkins::MissionStatus::Active) {
    return "REPONDRE A LIA (Q)";
  }
  if (hawkins::missionStatus(missions, hawkins::MissionId::ThreeSectors) ==
      hawkins::MissionStatus::Active) {
    if (exploration.hasOrigin == 0U) return "OBTENIR LE POINT GPS DE DEPART";
    if (eventHere) return "CONTINUER VERS UN SECTEUR INEDIT";
    return "ALLER VERS UN SECTEUR INEDIT";
  }
  if (hawkins::missionStatus(missions, hawkins::MissionId::AnomalyTrail) ==
      hawkins::MissionStatus::Active) {
    if (eventHere &&
        exploration.lastEventType == hawkins::EventType::StrangeSignal) {
      return "SIGNAL DETECTE ICI - POURSUIVRE";
    }
    if (eventHere) return "POURSUIVRE LE TRAJET POUR UN SIGNAL";
    return "CHERCHER UN SIGNAL EN BOUGEANT";
  }
  return "OBJECTIFS ACTUELS TERMINES";
}


void hawkinsAiNarrationTask(void*) {
  char prompt[sizeof(aiWorkerPrompt)] = {};
  portENTER_CRITICAL(&aiWorkerMux);
  memcpy(prompt, aiWorkerPrompt, sizeof(prompt));
  portEXIT_CRITICAL(&aiWorkerMux);

  char generated[hawkins::kHawkinsAiTextCapacity + 1U] = {};
  const bool ok = hawkins::hawkinsLlmRuntimeGenerate(
      prompt, generated, sizeof(generated), 12U);

  portENTER_CRITICAL(&aiWorkerMux);
  snprintf(aiWorkerOutput, sizeof(aiWorkerOutput), "%s", generated);
  aiWorkerOk = ok;
  aiWorkerDone = true;
  portEXIT_CRITICAL(&aiWorkerMux);
  vTaskDelete(nullptr);
}

bool requestAiNarration() {
  if (aiPending || aiWorkerRunning) {
    aiStatusText = "UNE NARRATION EST DEJA EN COURS";
    dirty = true;
    return false;
  }
  if (!hawkins::hawkinsLlmRuntimeReady()) {
    aiStatusText = "IA LOCALE INDISPONIBLE - JEU ACTIF";
    dirty = true;
    return false;
  }

  const hawkins::WorldState* world = hawkins::hawkinsRuntimeWorld();
  const char* eventName = world
      ? hawkins::eventTypeName(world->latestEvent.type) : "HAWKINS";
  const uint32_t day = world ? world->simulatedDays : 0U;
  const uint8_t tension = world ? world->tension : 0U;
  const uint8_t anomaly = world ? world->anomalyLevel : 0U;
  const char* playerName = playerLoaded ? player.name : "Joueur";
  const char* className = playerLoaded
      ? hawkins::characterClass(player.classId).name : "Aucune classe";
  const uint8_t level = playerLoaded ? player.level : 0U;
  const uint8_t hp = playerLoaded ? player.hitPoints : 0U;
  const uint8_t stress = playerLoaded ? player.stress : 0U;
  const char* objective = currentMovementObjective();
  const char* action = aiUserAction[0] != '\0'
      ? aiUserAction : "observer";

  // Le modèle dispose de 32 tokens au total : une partie est réservée à la
  // réponse. L'ordre place l'état joueur, le monde et l'objectif avant
  // l'action afin que le runtime conserve le contexte le plus utile.
  char prompt[sizeof(aiWorkerPrompt)] = {};
  const int promptLength = snprintf(
      prompt, sizeof(prompt),
      "User: %s, %s, niveau %u, PV %u, stress %u. Jour %lu, tension %u, anomalie %u, %s. Mission: %s. Action: %s. Bot:",
      playerName, className, unsigned(level), unsigned(hp), unsigned(stress),
      static_cast<unsigned long>(day), unsigned(tension), unsigned(anomaly),
      eventName, objective, action);
  if (promptLength < 0 ||
      static_cast<size_t>(promptLength) >= sizeof(prompt)) {
    aiStatusText = "CONTEXTE IA TROP LONG";
    dirty = true;
    return false;
  }

  portENTER_CRITICAL(&aiWorkerMux);
  snprintf(aiWorkerPrompt, sizeof(aiWorkerPrompt), "%s", prompt);
  aiWorkerOutput[0] = '\0';
  aiWorkerDone = false;
  aiWorkerOk = false;
  portEXIT_CRITICAL(&aiWorkerMux);

  aiWorkerRunning = true;
  aiPending = true;
  aiTimedOut = false;
  aiRequestAtMs = millis();
  aiNarration[0] = '\0';
  aiChoiceCount = 0U;
  aiStatusText = "NARRATION LOCALE EN COURS...";
  aiUserAction[0] = '\0';

  // loopTask du firmware tourne sur le cœur 1. En lançant le worker IA
  // sur ce cœur, le moteur matriciel REZOR crée son mm_worker sur le cœur 0 ;
  // celui-ci peut calculer pendant que loopTask reste réveillable sur le cœur 1.
  const BaseType_t created = xTaskCreatePinnedToCore(
      hawkinsAiNarrationTask, "hawkins_ai", 8192, nullptr, 1, nullptr, 1);
  if (created != pdPASS) {
    aiWorkerRunning = false;
    aiPending = false;
    aiStatusText = "MEMOIRE INSUFFISANTE POUR L'IA";
    dirty = true;
    return false;
  }

  dirty = true;
  return true;
}

void drawAiNarrationPage() {
  cv.fillScreen(TFT_BLACK);
  drawPageHeader("HAWKINS / NARRATEUR", "IA EMBARQUEE", ACCENT);
  cv.setFont(&fonts::Font0);
  cv.setTextSize(1);
  cv.setTextDatum(top_left);
  cv.setTextColor(aiPending ? TFT_ORANGE : ACCENT, TFT_BLACK);
  cv.drawString(aiStatusText, 5, 19);

  if (aiNarration[0] != '\0') {
    // Fontes embarque : affichage ASCII pour rester lisible sur le petit LCD.
    char display[hawkins::kHawkinsAiTextCapacity + 1U] = {};
    size_t in = 0U, out = 0U;
    while (aiNarration[in] != '\0' && out + 1U < sizeof(display)) {
      const unsigned char c = static_cast<unsigned char>(aiNarration[in++]);
      if (c < 0x80U) display[out++] = static_cast<char>(c);
      else if (c == 0xC3U && aiNarration[in] != '\0') {
        const unsigned char accent = static_cast<unsigned char>(aiNarration[in++]);
        char plain = '?';
        switch (accent) {
          case 0x80: case 0xA0: plain = 'a'; break;
          case 0x87: case 0xA7: plain = 'c'; break;
          case 0x88: case 0x89: case 0xA8: case 0xA9: case 0xAA:
          case 0xAB: plain = 'e'; break;
          case 0x8E: case 0x8F: case 0xAE: case 0xAF: plain = 'i'; break;
          case 0x94: case 0xB4: plain = 'o'; break;
          case 0x99: case 0x9A: case 0xB9: case 0xBA: plain = 'u'; break;
          default: plain = '?'; break;
        }
        display[out++] = plain;
      } else {
        display[out++] = '?';
        if ((c & 0xF0U) == 0xE0U) in += 2U;
        else if ((c & 0xF8U) == 0xF0U) in += 3U;
      }
    }
    display[out] = '\0';
    int y = 33;
    size_t position = 0U;
    while (display[position] != '\0' && y <= 75) {
      char row[38];
      size_t n = 0U;
      while (display[position] != '\0' && n < sizeof(row) - 1U &&
             display[position] != '\n') {
        row[n++] = display[position++];
      }
      if (display[position] == '\n') ++position;
      row[n] = '\0';
      cv.setTextColor(TFT_WHITE, TFT_BLACK);
      cv.drawString(row, 5, y);
      y += 11;
    }
  } else if (!aiPending) {
    cv.setTextColor(ICON_DIM, TFT_BLACK);
    cv.drawString("AUCUNE NARRATION POUR LE MOMENT", 5, 47);
  }

  for (uint8_t i = 0U; i < aiChoiceCount && i < 3U; ++i) {
    char choiceLine[61];
    snprintf(choiceLine, sizeof(choiceLine), "%u %s",
             unsigned(i + 1U), aiChoices[i]);
    cv.setTextDatum(top_left);
    cv.setTextColor(ACCENT, TFT_BLACK);
    cv.drawString(choiceLine, 5, 84 + int(i) * 12);
  }
  cv.setTextDatum(bottom_left);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString("1-3 CHOISIR  R RELANCER  Q RETOUR", 5, SH - 2);
}

void drawMissionPage() {
  cv.fillScreen(TFT_BLACK);
  drawPageHeader("HAWKINS / MISSIONS", "CAMPAGNE LOCALE", ACCENT);
  cv.setFont(&fonts::Font0);
  cv.setTextSize(1);

  if (!missionsReady) {
    cv.setTextDatum(middle_center);
    cv.setTextColor(TFT_ORANGE, TFT_BLACK);
    cv.drawString(missionsStorageError ? "ERREUR SAUVEGARDE MISSIONS"
                                       : "MISSIONS INDISPONIBLES",
                  SW / 2, 54);
    cv.setTextColor(ICON_DIM, TFT_BLACK);
    cv.drawString("Aucune donnee n'a ete effacee.", SW / 2, 72);
  } else {
    for (uint8_t i = 0U; i < hawkins::kMissionCount; ++i) {
      const hawkins::MissionId id = static_cast<hawkins::MissionId>(i);
      const hawkins::MissionDef& def = hawkins::missionDefinition(id);
      const hawkins::MissionStatus status = hawkins::missionStatus(missions, id);
      const int y = 23 + static_cast<int>(i) * 29;
      cv.setTextDatum(top_left);
      cv.setTextColor(status == hawkins::MissionStatus::Complete ? ACCENT
                         : status == hawkins::MissionStatus::Active ? TFT_WHITE
                                                                    : ICON_DIM,
                      TFT_BLACK);
      cv.drawString(def.title, 6, y);
      const char* statusText =
          status == hawkins::MissionStatus::Complete ? "TERMINEE" :
          status == hawkins::MissionStatus::Active ? "ACTIVE" : "VERROU";
      cv.setTextDatum(top_right);
      cv.setTextColor(status == hawkins::MissionStatus::Complete ? ACCENT
                         : status == hawkins::MissionStatus::Active ? TFT_WHITE
                                                                    : ICON_DIM,
                      TFT_BLACK);
      cv.drawString(statusText, SW - 6, y);

      char progress[48];
      if (id == hawkins::MissionId::FirstSignal) {
        snprintf(progress, sizeof(progress), "RADIO : %s  +%u XP",
                 status == hawkins::MissionStatus::Complete
                     ? "REPONSE ENREGISTREE" : "CHOIX A FAIRE",
                 unsigned(hawkins::missionRewardXp(id)));
      } else {
        const uint32_t value = id == hawkins::MissionId::ThreeSectors
                                   ? missions.sectors : missions.discoveries;
        snprintf(progress, sizeof(progress), "%s : %lu/%lu  +%u XP",
                 id == hawkins::MissionId::ThreeSectors ? "SECTEURS" : "SIGNAUX",
                 (unsigned long)value, (unsigned long)def.target,
                 unsigned(hawkins::missionRewardXp(id)));
      }
      cv.setTextDatum(top_left);
      cv.setTextColor(ICON_DIM, TFT_BLACK);
      cv.drawString(progress, 6, y + 11);
      if (i + 1U < hawkins::kMissionCount) {
        cv.drawFastHLine(6, y + 25, SW - 12, DIM_BORDER);
      }
    }
  }

  cv.setTextDatum(top_left);
  cv.setTextColor(ACCENT, TFT_BLACK);
  cv.drawString("OBJECTIF ACTUEL", 6, 108);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString(currentMovementObjective(), 6, 119);
  cv.setTextDatum(bottom_left);
  cv.drawString("M RET I OBJ K COMP Q SCN D SD", 5, SH - 2);
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
  cv.drawString("P RET M MISS I OBJ K COMP Q SCN", 5, SH - 2);
}


void drawInventoryPage() {
  cv.fillScreen(TFT_BLACK);
  drawPageHeader("HAWKINS / INVENTAIRE", "OBJETS", ACCENT);
  cv.setFont(&fonts::Font0);
  cv.setTextSize(1);
  const hawkins::AdventureItem items[hawkins::kAdventureItemCount] = {
      hawkins::AdventureItem::Medkit, hawkins::AdventureItem::CalmingTea,
      hawkins::AdventureItem::SpareBattery, hawkins::AdventureItem::Evidence};
  for (uint8_t i = 0U; i < hawkins::kAdventureItemCount; ++i) {
    char line[42];
    snprintf(line, sizeof(line), "%u  %s", unsigned(i + 1U),
             hawkins::adventureItemName(items[i]));
    cv.setTextDatum(top_left);
    cv.setTextColor(i < 2U ? TFT_WHITE : ICON_DIM, TFT_BLACK);
    cv.drawString(line, 7, 23 + int(i) * 21);
    snprintf(line, sizeof(line), "QTE : %u", unsigned(adventure.items[i]));
    cv.setTextDatum(top_right);
    cv.setTextColor(ACCENT, TFT_BLACK);
    cv.drawString(line, SW - 7, 23 + int(i) * 21);
  }
  cv.setTextDatum(bottom_left);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString("1 SOINS  2 CALME  I RETOUR", 5, SH - 2);
}

void drawSkillsPage() {
  cv.fillScreen(TFT_BLACK);
  drawPageHeader("HAWKINS / COMPETENCES", "TALENTS", ACCENT);
  cv.setFont(&fonts::Font0);
  cv.setTextSize(1);
  for (uint8_t i = 0U; i < hawkins::kAdventureSkillCount; ++i) {
    const hawkins::AdventureSkill skill = static_cast<hawkins::AdventureSkill>(i);
    const hawkins::AdventureSkillDef& def = hawkins::adventureSkillDefinition(skill);
    char line[44];
    snprintf(line, sizeof(line), "%u %s  %s", unsigned(i + 1U), def.name,
             hawkins::adventureHasSkill(adventure, skill) ? "OK" : "A ACHETER");
    cv.setTextDatum(top_left);
    cv.setTextColor(hawkins::adventureHasSkill(adventure, skill) ? ACCENT : TFT_WHITE,
                    TFT_BLACK);
    cv.drawString(line, 5, 21 + int(i) * 22);
    snprintf(line, sizeof(line), "%s >= %u  PTS: %u",
             hawkins::characterAttributeName(def.attribute),
             unsigned(def.requiredValue), unsigned(player.skillPoints));
    cv.setTextColor(ICON_DIM, TFT_BLACK);
    cv.drawString(line, 15, 31 + int(i) * 22);
  }
  cv.setTextDatum(bottom_left);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString("1-4 DEBLOQUER  K RETOUR", 5, SH - 2);
}

void drawFirstContactPage() {
  cv.fillScreen(TFT_BLACK);
  const bool resolved = (adventure.storyFlags & 0x01U) != 0U;
  drawPageHeader(resolved ? "HAWKINS / LIA" : "HAWKINS / RADIO",
                 resolved ? "SUITE" : "DEBUT", ACCENT);
  cv.setFont(&fonts::Font0);
  cv.setTextSize(1);
  cv.setTextDatum(top_left);

  if (!resolved) {
    cv.setTextColor(TFT_WHITE, TFT_BLACK);
    cv.drawString("MINUIT. LA RADIO S'ALLUME SEULE.", 5, 20);
    cv.drawString("UN GRESILLEMENT COUVRE LA FREQUENCE.", 5, 32);
    cv.drawString("UNE VOIX CHUCHOTE :", 5, 44);
    cv.drawString("\"AIDEZ-MOI... JE VOUS EN PRIE.\"", 5, 56);

    cv.setTextColor(ACCENT, TFT_BLACK);
    cv.drawString("1 REPONDRE ET AIDER", 5, 75);
    cv.setTextColor(TFT_WHITE, TFT_BLACK);
    cv.drawString("2 ENQUETER SUR LE SIGNAL", 5, 88);
    cv.drawString("3 COUPER ET SE REPLIER", 5, 101);
  } else {
    const char* lines[4] = {};
    switch (adventure.firstContactChoice) {
      case 1U:
        lines[0] = "TU AS CHOISI D'AIDER.";
        lines[1] = "LIA : \"MERCI... TU M'ENTENDS ?\"";
        lines[2] = "\"ILS SONT TOUT PRES. ECOUTE.\"";
        lines[3] = "LE SIGNAL RESTE OUVERT.";
        break;
      case 2U:
        lines[0] = "TU AS CHOISI D'ENQUETER.";
        lines[1] = "LIA : \"TROIS IMPULSIONS...\"";
        lines[2] = "\"CE SIGNAL N'EST PAS UNE VOIX.\"";
        lines[3] = "UN SECOND SIGNAL APPARAIT.";
        break;
      case 3U:
        lines[0] = "TU AS CHOISI DE TE REPLIER.";
        lines[1] = "LIA : \"D'ACCORD. ELOIGNE-TOI.\"";
        lines[2] = "\"JE TE RECONTACTERAI. PROMIS.\"";
        lines[3] = "LA RADIO S'ETEINT LENTEMENT.";
        break;
      default:
        // Anciennes sauvegardes v1 : le choix avait ete consomme mais pas stocke.
        lines[0] = "TON ANCIEN CHOIX EST CONSERVE.";
        lines[1] = "LIA : \"JE SUIS TOUJOURS LA.\"";
        lines[2] = "LA SUITE NE MODIFIE PAS TON PROFIL.";
        lines[3] = "TU PEUX REPRENDRE TON HISTOIRE.";
        break;
    }
    for (uint8_t i = 0U; i < 4U; ++i) {
      cv.setTextColor(i == 0U ? ACCENT : TFT_WHITE, TFT_BLACK);
      cv.drawString(lines[i], 5, 25 + static_cast<int>(i) * 15);
    }
  }

  cv.setTextDatum(bottom_left);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString(resolved ? "A IA  Q RETOUR" : "1-3 CHOIX  A IA  Q RETOUR", 3, SH - 2);
}

void drawStoragePage() {
  cv.fillScreen(TFT_BLACK);
  drawPageHeader("HAWKINS / SAUVEGARDE", sdMounted ? "MICROSD" : "SANS SD",
                 sdMounted ? ACCENT : TFT_ORANGE);
  cv.setFont(&fonts::Font0);
  cv.setTextSize(1);
  char line[48];
  cv.setTextDatum(top_left);
  cv.setTextColor(TFT_WHITE, TFT_BLACK);
  if (sdMounted) {
    snprintf(line, sizeof(line), "CARTE : %llu MB",
             static_cast<unsigned long long>(sdSizeMB));
    cv.drawString(line, 6, 22);
    snprintf(line, sizeof(line), "UTILISE : %llu MB",
             static_cast<unsigned long long>(sdUsedMB));
    cv.setTextColor(ICON_DIM, TFT_BLACK);
    cv.drawString(line, 6, 35);
  } else {
    cv.drawString("CARTE NON MONTEE / FORMAT NON LU", 6, 27);
    cv.drawString("AUCUNE DONNEE DE LA CARTE EFFACEE", 6, 40);
  }
  cv.setTextColor(ACCENT, TFT_BLACK);
  snprintf(line, sizeof(line), "COPIE AUTO : %s",
           hawkins::sdBackupEnabled() ? "OUI" : "NON");
  cv.drawString(line, 6, 55);
  cv.setTextColor(TFT_WHITE, TFT_BLACK);
  snprintf(line, sizeof(line), "COPIE NUMERO : %lu",
           static_cast<unsigned long>(hawkins::sdBackupSequence()));
  cv.drawString(line, 6, 69);
  const bool narrativeSaveError = adventureStorageError ||
                                   narrativeStorageError;
  cv.setTextColor(narrativeSaveError ? TFT_ORANGE : ICON_DIM, TFT_BLACK);
  cv.drawString(adventureStorageError ? "ERREUR NVS AVENTURE"
                  : narrativeStorageError ? "ERREUR NVS RECIT"
                                           : hawkins::sdBackupStatusText(),
                6, 83);
  cv.setTextColor(TFT_WHITE, TFT_BLACK);
  cv.drawString(gpsStatusText, 6, 96);
  if (restoreConfirmation) {
    cv.setTextColor(TFT_ORANGE, TFT_BLACK);
    cv.drawString("R A NOUVEAU : RESTAURER LA PARTIE", 6, 111);
  } else {
    cv.setTextColor(ICON_DIM, TFT_BLACK);
    cv.drawString("A AUTO  B COPIE  R RESTAURER", 6, 111);
  }
  cv.setTextDatum(bottom_left);
  cv.setTextColor(ICON_DIM, TFT_BLACK);
  cv.drawString("D RETOUR", 5, SH - 2);
}

bool saveAdventureAndCharacter(const hawkins::AdventureState& nextAdventure,
                               const hawkins::CharacterState& nextPlayer) {
  uint32_t nextPlayerSequence = playerSequence;
  if (!hawkins::characterSaveCommit(nextPlayer, &nextPlayerSequence)) return false;
  uint32_t nextAdventureSequence = adventureSequence;
  if (!hawkins::adventureSaveCommit(nextAdventure, &nextAdventureSequence)) {
    uint32_t rollbackSequence = playerSequence;
    if (!hawkins::characterSaveCommit(player, &rollbackSequence)) {
      playerStorageError = true;
      setGpsStatus("ERREUR RESTAURATION DU PROFIL");
    }
    return false;
  }
  player = nextPlayer;
  playerSequence = nextPlayerSequence;
  adventure = nextAdventure;
  adventureSequence = nextAdventureSequence;
  adventureReady = true;
  playerStorageError = false;
  adventureStorageError = false;
  hawkins::sdBackupAutoSave();
  dirty = true;
  return true;
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

bool saveNarrativeMemory(const hawkins::NarrativeMemory& candidate,
                         bool createSdBackup = true) {
  uint32_t nextSequence = narrativeSequence;
  if (!hawkins::narrativeSaveCommit(candidate, &nextSequence)) {
    narrativeStorageError = true;
    setGpsStatus("MEMOIRE NARRATIVE NON SAUVEE");
    return false;
  }
  narrative = candidate;
  narrativeSequence = nextSequence;
  narrativeReady = true;
  narrativeStorageError = false;
  if (createSdBackup) hawkins::sdBackupAutoSave();
  dirty = true;
  return true;
}

bool addNarrativeRecord(hawkins::NarrativeRecordType type, uint32_t cellKey,
                        uint16_t detail, uint8_t intensity) {
  if (!narrativeReady) return false;
  hawkins::NarrativeMemory updated = narrative;
  if (!hawkins::narrativeRecord(updated, type, cellKey, detail, intensity)) {
    narrativeStorageError = true;
    return false;
  }
  return saveNarrativeMemory(updated);
}

void rememberFirstContact(uint8_t savedChoice) {
  if (!narrativeReady || (narrative.plotFlags & 0x01U) != 0U) return;
  hawkins::NarrativeMemory updated = narrative;
  // firstContactChoice=0 signifie une ancienne sauvegarde sans branche connue.
  const int8_t relationshipChange =
      savedChoice == 1U ? 10 : savedChoice == 2U ? 5 :
      savedChoice == 3U ? -10 : 0;
  if (!hawkins::narrativeSetPlot(updated, 0U, true) ||
      !hawkins::narrativeAdjustNpcRelationship(updated, 0U,
                                               relationshipChange) ||
      !hawkins::narrativeRecord(
          updated, hawkins::NarrativeRecordType::FirstContact, 0U,
          savedChoice,
          updated.relationships[0])) {
    narrativeStorageError = true;
    return;
  }
  saveNarrativeMemory(updated);
}

void rememberMissionCompletion(hawkins::MissionId id) {
  if (!narrativeReady) return;
  const uint8_t missionIndex = static_cast<uint8_t>(id);
  if (missionIndex >= hawkins::kMissionCount) return;
  const uint8_t plotId = static_cast<uint8_t>(missionIndex + 1U);
  const uint8_t mask = static_cast<uint8_t>(1U << plotId);
  if ((narrative.plotFlags & mask) != 0U) return;
  hawkins::NarrativeMemory updated = narrative;
  if (!hawkins::narrativeSetPlot(updated, plotId, true) ||
      !hawkins::narrativeRecord(
          updated, hawkins::NarrativeRecordType::MissionComplete, 0U,
          missionIndex, 100U)) {
    narrativeStorageError = true;
    return;
  }
  saveNarrativeMemory(updated, false);
}


void applyCompletedMissionRewards() {
  if (!missionsReady || !playerLoaded) return;
  for (uint8_t i = 0U; i < hawkins::kMissionCount; ++i) {
    const hawkins::MissionId id = static_cast<hawkins::MissionId>(i);
    if (hawkins::missionStatus(missions, id) != hawkins::MissionStatus::Complete) {
      continue;
    }
    const uint8_t mask = static_cast<uint8_t>(1U << i);
    if ((player.missionRewardMask & mask) != 0U) continue;

    hawkins::CharacterState updated = player;
    if (!hawkins::awardCharacterMissionXpOnce(
            updated, i, hawkins::missionRewardXp(id)) ||
        !savePlayerState(updated)) {
      playerStorageError = true;
      setGpsStatus("RECOMPENSE EN ATTENTE - ERREUR NVS");
      return;
    }
  }
}

bool completeFirstSignalMission() {
  if (!missionsReady) return false;
  if (hawkins::missionStatus(missions, hawkins::MissionId::FirstSignal) ==
      hawkins::MissionStatus::Complete) {
    return true;
  }

  hawkins::MissionState updated = missions;
  if (!hawkins::completeFirstSignal(updated)) return false;
  uint32_t nextSequence = missionSequence;
  if (!hawkins::missionSaveCommit(updated, &nextSequence)) {
    missionsStorageError = true;
    setGpsStatus("MISSION RADIO NON SAUVEGARDEE");
    return false;
  }

  missions = updated;
  missionSequence = nextSequence;
  missionsStorageError = false;
  rememberMissionCompletion(hawkins::MissionId::FirstSignal);
  applyCompletedMissionRewards();
  // La copie SD facultative suit la NVS et la recompense, jamais l'inverse.
  hawkins::sdBackupAutoSave();
  return true;
}

bool commitMissionProgress(uint32_t cellKey, bool discovery) {
  if (!missionsReady) return false;
  hawkins::MissionState updated = missions;
  hawkins::recordMissionSector(updated, cellKey, discovery);
  if (updated.sectors == missions.sectors &&
      updated.discoveries == missions.discoveries &&
      updated.lastCellKey == missions.lastCellKey &&
      updated.hasLastCell == missions.hasLastCell) {
    return true;  // La cellule a deja ete prise en compte avant une coupure.
  }
  uint32_t nextSequence = missionSequence;
  if (!hawkins::missionSaveCommit(updated, &nextSequence)) {
    missionsStorageError = true;
    setGpsStatus("ERREUR SAUVEGARDE MISSIONS");
    return false;
  }
  missions = updated;
  missionSequence = nextSequence;
  for (uint8_t i = 0U; i < hawkins::kMissionCount; ++i) {
    const hawkins::MissionId id = static_cast<hawkins::MissionId>(i);
    if (hawkins::missionStatus(missions, id) ==
        hawkins::MissionStatus::Complete) {
      rememberMissionCompletion(id);
    }
  }
  applyCompletedMissionRewards();
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
      applyCompletedMissionRewards();
      hawkins::sdBackupAutoSave();
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

void hawkinsAppAiUpdate() {
  bool completed = false;
  bool succeeded = false;
  char generated[hawkins::kHawkinsAiTextCapacity + 1U] = {};

  portENTER_CRITICAL(&aiWorkerMux);
  if (aiWorkerDone) {
    completed = true;
    succeeded = aiWorkerOk;
    snprintf(generated, sizeof(generated), "%s", aiWorkerOutput);
    aiWorkerDone = false;
  }
  portEXIT_CRITICAL(&aiWorkerMux);

  if (completed) {
    aiWorkerRunning = false;
    if (aiTimedOut) {
      aiTimedOut = false;
      return;
    }
    aiPending = false;
    if (succeeded && generated[0] != '\0') {
      snprintf(aiNarration, sizeof(aiNarration), "%s", generated);
      snprintf(aiChoices[0], sizeof(aiChoices[0]), "%s", "Observer");
      snprintf(aiChoices[1], sizeof(aiChoices[1]), "%s", "Repondre");
      snprintf(aiChoices[2], sizeof(aiChoices[2]), "%s", "Se cacher");
      aiChoiceCount = 3U;
      aiStatusText = "NARRATION GENEREE SUR LE CARDPUTER";
      addNarrativeRecord(hawkins::NarrativeRecordType::AiNarration, 0U,
                         static_cast<uint16_t>(aiChoiceCount), 0U);
      Serial.printf("[hawkins-llm] narration locale recue: %u octets, %u choix\n",
                    static_cast<unsigned>(strlen(aiNarration)),
                    static_cast<unsigned>(aiChoiceCount));
    } else {
      aiNarration[0] = '\0';
      aiChoiceCount = 0U;
      aiStatusText = "IA INDISPONIBLE - JEU LOCAL ACTIF";
      Serial.println("[hawkins-llm] generation interrompue ou sans texte");
    }
    dirty = true;
  }

  if (aiPending && millis() - aiRequestAtMs > 270000U) {
    aiPending = false;
    aiTimedOut = true;
    aiStatusText = "DELAI IA DEPASSE - JEU TOUJOURS ACTIF";
    dirty = true;
  }
}

void hawkinsAppEnter() {
  const bool modelReady = hawkins::hawkinsLlmRuntimeInit();
  aiStatusText = modelReady ? "IA LOCALE EMBARQUEE PRETE"
                            : "IA LOCALE INDISPONIBLE - JEU ACTIF";
  Serial.printf("[hawkins-llm] modele embarque %s; le jeu reste autonome\n",
                modelReady ? "pret" : "indisponible");
  showAiNarration = false;
  candidateActive = false;
  lastSampleMs = 0U;
  explorerReady = false;
  explorerStorageError = false;
  showExploration = true;
  showCharacter = false;
  showMissions = false;
  missionsReady = false;
  missionsStorageError = false;
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

  const hawkins::MissionSaveLoadStatus missionLoad =
      hawkins::missionSaveLoad(missions, &missionSequence);
  if (missionLoad == hawkins::MissionSaveLoadStatus::Loaded) {
    missionsReady = true;
  } else if (missionLoad == hawkins::MissionSaveLoadStatus::Empty) {
    hawkins::initializeMissions(missions);
    missionsReady = hawkins::missionSaveCommit(missions, &missionSequence);
    missionsStorageError = !missionsReady;
  } else {
    missionsStorageError = true;
  }
  if (missionsReady && playerLoaded) applyCompletedMissionRewards();

  adventureReady = false;
  adventureStorageError = false;
  const hawkins::AdventureSaveLoadStatus adventureLoad =
      hawkins::adventureSaveLoad(adventure, &adventureSequence);
  if (adventureLoad == hawkins::AdventureSaveLoadStatus::Loaded) {
    adventureReady = true;
  } else if (adventureLoad == hawkins::AdventureSaveLoadStatus::Empty) {
    hawkins::initializeAdventure(adventure);
    adventureReady = hawkins::adventureSaveCommit(adventure, &adventureSequence);
    adventureStorageError = !adventureReady;
  } else {
    adventureStorageError = true;
  }
  const hawkins::NarrativeSaveLoadStatus narrativeLoad =
      hawkins::narrativeSaveLoad(narrative, &narrativeSequence);
  if (narrativeLoad == hawkins::NarrativeSaveLoadStatus::Loaded) {
    narrativeReady = true;
    narrativeStorageError = false;
  } else if (narrativeLoad == hawkins::NarrativeSaveLoadStatus::Empty) {
    hawkins::initializeNarrativeMemory(narrative);
    narrativeReady = hawkins::narrativeSaveCommit(
        narrative, &narrativeSequence);
    narrativeStorageError = !narrativeReady;
  } else {
    narrativeReady = false;
    narrativeStorageError = true;
  }

  hawkins::sdBackupInit();
  if (playerLoaded && adventureReady &&
      (adventure.storyFlags & 0x01U) != 0U) {
    // Reconstruit la trace narrative si une sauvegarde plus ancienne ne la contenait pas.
    rememberFirstContact(adventure.firstContactChoice);
  }
  if (missionsReady) {
    for (uint8_t i = 0U; i < hawkins::kMissionCount; ++i) {
      const hawkins::MissionId id = static_cast<hawkins::MissionId>(i);
      if (hawkins::missionStatus(missions, id) ==
          hawkins::MissionStatus::Complete) {
        rememberMissionCompletion(id);
      }
    }
  }
  if (playerLoaded && adventureReady &&
      (adventure.storyFlags & 0x01U) != 0U) {
    // Reprise idempotente si l'appareil s'est eteint apres le choix de Lia
    // mais avant l'enregistrement de la mission et de sa recompense.
    if (!completeFirstSignalMission()) {
      setGpsStatus("CHOIX CONSERVE - MISSION A REPRENDRE");
    }
  }
  showInventory = showSkills = showFirstContact = showStorage = false;
  restoreConfirmation = false;

  characterCreationStep = 0U;
  selectedClass = 0U;
  memset(characterName, 0, sizeof(characterName));
  characterNameLength = 0U;
  dirty = true;
}

void hawkinsAppUpdate() {
  hawkinsAppAiUpdate();
  const uint32_t now = millis();
  if (lastSampleMs != 0U && now - lastSampleMs < 1000U) return;
  lastSampleMs = now;
  if (missionsReady && playerLoaded && adventureReady &&
      (adventure.storyFlags & 0x01U) != 0U &&
      hawkins::missionStatus(missions, hawkins::MissionId::FirstSignal) ==
          hawkins::MissionStatus::Active) {
    if (completeFirstSignalMission()) {
      setGpsStatus(playerStorageError
                       ? "MISSION REPRISE - RECOMPENSE A REPRENDRE"
                       : "MISSION REPRISE - CARTOGRAPHIE ACTIVE");
    }
  }
  if (missionsReady && playerLoaded &&
      hawkins::missionStatus(missions, hawkins::MissionId::FirstSignal) !=
          hawkins::MissionStatus::Locked) {
    applyCompletedMissionRewards();
  }

  if (!explorerReady) {
    setGpsStatus(explorerStorageError ? "ERREUR MEMOIRE NVS"
                                      : "EXPLORATION INDISPONIBLE");
    candidateActive = false;
    return;
  }
  if (!playerLoaded) {
    gpsSats = 0U;
    candidateActive = false;
    setGpsStatus(playerStorageError ? "PROFIL INDISPONIBLE - NVS"
                                    : "CREER PERSONNAGE AVANT JOUER");
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

  const hawkins::ExplorationState previousExploration = exploration;
  const hawkins::ExplorationResult result =
      hawkins::observeExplorationCell(exploration, candidateCellKey);
  candidateActive = false;
  if (result == hawkins::ExplorationResult::Invalid) {
    setGpsStatus("POSITION NON VALIDEE");
    return;
  }
  if (result == hawkins::ExplorationResult::SameCell) {
    setGpsStatus("POSITION STABLE - SECTEUR CONNU");
    dirty = true;
    return;
  }
  if (result == hawkins::ExplorationResult::Revisited) {
    if (!hawkins::explorationSaveCommit(exploration, &explorationSequence)) {
      exploration = previousExploration;
      explorerStorageError = true;
      explorerReady = false;
      setGpsStatus("ERREUR SAUVEGARDE DU RETOUR");
      return;
    }
    setGpsStatus("SECTEUR DEJA VISITE - OBJECTIF ACTUALISE");
    hawkins::sdBackupAutoSave();
    dirty = true;
    return;
  }
  if (result == hawkins::ExplorationResult::OriginSet) {
    // Le premier fix GPS définit uniquement le point de départ. Il ne déclenche
    // ni événement ni mission : le joueur commence réellement à explorer ensuite.
    if (!hawkins::explorationSaveCommit(exploration, &explorationSequence)) {
      exploration = previousExploration;
      explorerStorageError = true;
      explorerReady = false;
      setGpsStatus("ERREUR SAUVEGARDE GPS");
      return;
    }
    setGpsStatus("POINT DE DEPART ENREGISTRE");
    hawkins::sdBackupAutoSave();
  } else {
    bool discovery = result == hawkins::ExplorationResult::NewDiscovery;
    const bool localEvent = hawkins::generateLocalAreaEvent(
        exploration, *world, candidateCellKey, discovery);
    // Un signal aléatoire rencontré sur un nouveau secteur compte aussi comme
    // une découverte de mission. Les signaux garantis ne sont comptés qu'une fois.
    if (localEvent &&
        exploration.lastEventType == hawkins::EventType::StrangeSignal) {
      discovery = true;
    }
    if (!commitMissionProgress(candidateCellKey, discovery)) {
      exploration = previousExploration;
      candidateActive = false;
      setGpsStatus("MISSION NON SAUVEGARDEE - REESSAYER");
      return;
    }
    if (!hawkins::explorationSaveCommit(exploration, &explorationSequence)) {
      explorerStorageError = true;
      explorerReady = false;
      setGpsStatus("GPS EN ATTENTE DE SAUVEGARDE");
      return;
    }

    const bool daySaved = hawkins::hawkinsRuntimeAdvanceDays(1U);
    const hawkins::NarrativeRecordType narrativeType =
        localEvent &&
        exploration.lastEventType == hawkins::EventType::StrangeSignal
            ? hawkins::NarrativeRecordType::LocalSignal
            : hawkins::NarrativeRecordType::SectorDiscovery;
    const uint16_t narrativeDetail = localEvent
        ? static_cast<uint16_t>(exploration.lastEventType)
        : static_cast<uint16_t>(0U);
    addNarrativeRecord(narrativeType, candidateCellKey, narrativeDetail,
                       localEvent ? exploration.lastEventIntensity : 0U);
    const uint16_t reward = discovery ? 50U : 25U;
    awardExplorationExperience(reward);
    applyCompletedMissionRewards();
    hawkins::sdBackupAutoSave();
    if (discovery) {
      setGpsStatus(daySaved ? "SIGNAL LOCAL DECOUVERT"
                            : "SIGNAL LOCAL - SAUVEGARDE MONDE");
    } else if (localEvent) {
      setGpsStatus(daySaved ? "EVENEMENT LOCAL DETECTE"
                            : "EVENEMENT LOCAL - SAUVEGARDE MONDE");
    } else {
      setGpsStatus(daySaved ? "NOUVEAU SECTEUR EXPLORE"
                            : "SECTEUR TROUVE - SAUVEGARDE MONDE");
    }
  }
  dirty = true;
}

void hawkinsAppKey(char key) {
  if (showInventory) {
    if (key == 'i' || key == 'I') {
      showInventory = false;
      dirty = true;
      return;
    }
    if ((key == '1' || key == '2') && playerLoaded && adventureReady) {
      hawkins::CharacterState nextPlayer = player;
      hawkins::AdventureState nextAdventure = adventure;
      const hawkins::AdventureItem item = key == '1'
          ? hawkins::AdventureItem::Medkit : hawkins::AdventureItem::CalmingTea;
      if (hawkins::adventureUseItem(nextAdventure, nextPlayer, item) &&
          saveAdventureAndCharacter(nextAdventure, nextPlayer)) {
        addNarrativeRecord(hawkins::NarrativeRecordType::ItemUsed, 0U,
                           static_cast<uint16_t>(item), 0U);
        setGpsStatus("OBJET UTILISE");
      } else {
        setGpsStatus("OBJET INDISPONIBLE / ECHEC SAUVEGARDE");
      }
      dirty = true;
      return;
    }
    return;
  }

  if (showSkills) {
    if (key == 'k' || key == 'K') {
      showSkills = false;
      dirty = true;
      return;
    }
    if (key >= '1' && key <= '4' && playerLoaded && adventureReady) {
      hawkins::CharacterState nextPlayer = player;
      hawkins::AdventureState nextAdventure = adventure;
      const hawkins::AdventureSkill skill =
          static_cast<hawkins::AdventureSkill>(key - '1');
      if (hawkins::adventureUnlockSkill(nextAdventure, nextPlayer, skill) &&
          saveAdventureAndCharacter(nextAdventure, nextPlayer)) {
        addNarrativeRecord(hawkins::NarrativeRecordType::SkillUnlocked, 0U,
                           static_cast<uint16_t>(skill), 0U);
        setGpsStatus("COMPETENCE DEBLOQUEE");
      } else {
        setGpsStatus("POINT OU CARACTERISTIQUE INSUFFISANT");
      }
      dirty = true;
      return;
    }
    return;
  }

  if (showAiNarration) {
    if (key == 'q' || key == 'Q' || key == 'a' || key == 'A') {
      showAiNarration = false;
      dirty = true;
      return;
    }
    if (key == 'r' || key == 'R') {
      requestAiNarration();
      return;
    }
    if (key >= '1' && key <= '3' &&
        static_cast<uint8_t>(key - '1') < aiChoiceCount && !aiPending) {
      const uint8_t selected = static_cast<uint8_t>(key - '1');
      snprintf(aiUserAction, sizeof(aiUserAction), "%s", aiChoices[selected]);
      if (requestAiNarration()) {
        addNarrativeRecord(hawkins::NarrativeRecordType::AiChoice, 0U,
                           static_cast<uint16_t>(selected + 1U), 0U);
      }
      return;
    }
    return;
  }

  if (showFirstContact) {
    if (key == 'a' || key == 'A') {
      showAiNarration = true;
      requestAiNarration();
      dirty = true;
      return;
    }
    if (key == 'q' || key == 'Q') {
      showFirstContact = false;
      dirty = true;
      return;
    }
    if (key >= '1' && key <= '3' && playerLoaded && adventureReady &&
        (adventure.storyFlags & 0x01U) == 0U) {
      hawkins::CharacterState nextPlayer = player;
      hawkins::AdventureState nextAdventure = adventure;
      const hawkins::FirstContactChoice choice =
          static_cast<hawkins::FirstContactChoice>(key - '1');
      if (hawkins::adventureResolveFirstContact(nextAdventure, nextPlayer, choice) &&
          saveAdventureAndCharacter(nextAdventure, nextPlayer)) {
        rememberFirstContact(nextAdventure.firstContactChoice);
        if (completeFirstSignalMission()) {
          const bool mappingAlreadyComplete =
              hawkins::missionStatus(missions, hawkins::MissionId::ThreeSectors) ==
              hawkins::MissionStatus::Complete;
          setGpsStatus(playerStorageError
                           ? "CHOIX OK - RECOMPENSE A REPRENDRE"
                           : mappingAlreadyComplete
                                 ? "CHOIX ENREGISTRE - PISTE ACTIVE"
                                 : "CHOIX ENREGISTRE - CARTOGRAPHIE ACTIVE");
        } else {
          // Le choix reste enregistre ; la reconciliation reprendra la mission
          // au prochain chargement sans autoriser un second choix/recompense.
          setGpsStatus("CHOIX CONSERVE - MISSION A REPRENDRE");
        }
      } else {
        setGpsStatus("CHOIX IMPOSSIBLE / ECHEC SAUVEGARDE");
      }
      dirty = true;
      return;
    }
    return;
  }

  if (showStorage) {
    if (restoreConfirmation) {
      if (key == 'r' || key == 'R') {
        if (hawkins::sdBackupRestore()) {
          hawkins::hawkinsRuntimeInit();
          hawkinsAppEnter();
          showStorage = true;
          setGpsStatus("RESTAURATION SD TERMINEE");
        } else {
          setGpsStatus(hawkins::sdBackupStatusText());
          restoreConfirmation = false;
        }
        dirty = true;
        return;
      }
      if (key == 'd' || key == 'D') {
        restoreConfirmation = false;
        dirty = true;
        return;
      }
      return;
    }
    if (key == 'd' || key == 'D') {
      showStorage = false;
      dirty = true;
      return;
    }
    if (key == 'a' || key == 'A') {
      if (hawkins::sdBackupSetEnabled(!hawkins::sdBackupEnabled())) {
        setGpsStatus(hawkins::sdBackupEnabled()
                         ? "COPIE AUTOMATIQUE ACTIVE"
                         : "COPIE AUTOMATIQUE DESACTIVEE");
      } else {
        setGpsStatus("SD ABSENTE OU COPIE IMPOSSIBLE");
      }
      dirty = true;
      return;
    }
    if (key == 'b' || key == 'B') {
      setGpsStatus(hawkins::sdBackupSaveNow()
                       ? "COPIE SD VERIFIEE"
                       : hawkins::sdBackupStatusText());
      dirty = true;
      return;
    }
    if (key == 'r' || key == 'R') {
      restoreConfirmation = true;
      dirty = true;
      return;
    }
    return;
  }
  if (showMissions) {
    if (key == 'm' || key == 'M') {
      showMissions = false;
      dirty = true;
      return;
    }
    if (key == 'e' || key == 'E') {
      showMissions = false;
      showExploration = true;
      dirty = true;
      return;
    }
    if (key == 'p' || key == 'P') {
      showCharacter = true;
      dirty = true;
      return;
    }
  }

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
    if (key == 'm' || key == 'M') {
      showCharacter = false;
      showMissions = true;
      dirty = true;
      return;
    }
    if (key == 's' || key == 'S') {
      if (playerLoaded && savePlayerState(player)) {
        hawkins::sdBackupAutoSave();
        setGpsStatus("PERSONNAGE SAUVEGARDE");
      } else {
        setGpsStatus("ERREUR SAUVEGARDE PERSONNAGE");
      }
      dirty = true;
      return;
    }
  }

  if (key == 'a' || key == 'A') {
    showAiNarration = true;
    requestAiNarration();
    dirty = true;
  } else if (key == 'i' || key == 'I') {
    if (playerLoaded && adventureReady) showInventory = true;
    dirty = true;
  } else if (key == 'k' || key == 'K') {
    if (playerLoaded && adventureReady) showSkills = true;
    dirty = true;
  } else if (key == 'q' || key == 'Q') {
    if (playerLoaded && adventureReady) showFirstContact = true;
    dirty = true;
  } else if (key == 'd' || key == 'D') {
    showStorage = true;
    dirty = true;
  } else if (key == 'p' || key == 'P') {
    showCharacter = true;
    dirty = true;
  } else if (key == 'm' || key == 'M') {
    showMissions = true;
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
    const bool missionsSaved = missionsReady &&
        hawkins::missionSaveCommit(missions, &missionSequence);
    const bool adventureSaved = adventureReady &&
        hawkins::adventureSaveCommit(adventure, &adventureSequence);
    const bool sdSaved = hawkins::sdBackupAutoSave();
    setGpsStatus(worldSaved && exploreSaved && characterSaved && missionsSaved &&
                         adventureSaved && sdSaved
                     ? "SAUVEGARDE LOCALE OK"
                     : "ERREUR DE SAUVEGARDE");
    dirty = true;
  }
}

void drawHawkinsApp() {
  if (showAiNarration) {
    drawAiNarrationPage();
    return;
  }
  if (showInventory) {
    drawInventoryPage();
    return;
  }
  if (showSkills) {
    drawSkillsPage();
    return;
  }
  if (showFirstContact) {
    drawFirstContactPage();
    return;
  }
  if (showStorage) {
    drawStoragePage();
    return;
  }
  if (showCharacter) {
    drawCharacterPage();
    return;
  }
  if (showMissions) {
    drawMissionPage();
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
