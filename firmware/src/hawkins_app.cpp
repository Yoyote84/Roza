#include "hawkins_app.h"
#include "hawkins_runtime.h"
#include "hawkins_nvs.h"
#include "hawkins_exploration.h"
#include "hawkins_character.h"
#include "hawkins_missions.h"
#include "hawkins_adventure.h"
#include "hawkins_adventure_store.h"
#include "hawkins_sd_backup.h"
#include "sd_files.h"
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

hawkins::MissionState missions{};
uint32_t missionSequence = 0U;
bool missionsReady = false;
bool missionsStorageError = false;
bool showMissions = false;

hawkins::AdventureState adventure{};
uint32_t adventureSequence = 0U;
bool adventureReady = false;
bool adventureStorageError = false;
bool showInventory = false;
bool showSkills = false;
bool showFirstContact = false;
bool showStorage = false;
bool restoreConfirmation = false;

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
  cv.drawString("P PERSO M MISS I OBJ K COMP Q SCN D SD", 5, SH - 2);
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
  cv.drawString("P PERSO  E TERRAIN  M MISS.  S SAVE", 5, SH - 2);
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
  cv.drawString(resolved ? "Q RETOUR" : "1-3 CHOISIR  Q RETOUR", 3, SH - 2);
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
  cv.setTextColor(adventureStorageError ? TFT_ORANGE : ICON_DIM, TFT_BLACK);
  cv.drawString(adventureStorageError ? "ERREUR NVS AVENTURE"
                                     : hawkins::sdBackupStatusText(), 6, 83);
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

void hawkinsAppEnter() {
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
  hawkins::sdBackupInit();
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
    const bool localEvent = hawkins::generateLocalAreaEvent(
        exploration, *world, candidateCellKey, false);
    if (!hawkins::explorationSaveCommit(exploration, &explorationSequence)) {
      exploration = previousExploration;
      explorerStorageError = true;
      explorerReady = false;
      setGpsStatus("ERREUR SAUVEGARDE GPS");
      return;
    }
    setGpsStatus(localEvent ? "EVENEMENT LOCAL DANS LA ZONE"
                            : "POINT DE DEPART ENREGISTRE");
    hawkins::sdBackupAutoSave();
  } else {
    const bool discovery =
        result == hawkins::ExplorationResult::NewDiscovery;
    const bool localEvent = hawkins::generateLocalAreaEvent(
        exploration, *world, candidateCellKey, discovery);
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
        setGpsStatus("COMPETENCE DEBLOQUEE");
      } else {
        setGpsStatus("POINT OU CARACTERISTIQUE INSUFFISANT");
      }
      dirty = true;
      return;
    }
    return;
  }

  if (showFirstContact) {
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

  if (key == 'i' || key == 'I') {
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
