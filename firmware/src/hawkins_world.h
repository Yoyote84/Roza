#pragma once

#include <stdint.h>

namespace hawkins {

constexpr uint32_t kWorldSchemaVersion = 1;
constexpr uint32_t kSecondsPerDay = 86400U;
// Limite le travail de rattrapage après une longue extinction. Ce n'est pas
// une limite de durée de campagne : plusieurs appels peuvent rattraper plus.
constexpr uint32_t kMaxCatchUpDaysPerCall = 4096U;

enum class EventType : uint8_t {
  None = 0,
  StrangeSignal,
  PowerAnomaly,
  CreatureTrace,
  FactionMovement,
  DistressCall,
  PortalFluctuation
};

struct WorldEvent {
  uint32_t sequence;
  uint32_t day;
  EventType type;
  uint8_t intensity;  // 1..100
  uint8_t sector;     // secteur de jeu abstrait, pas une coordonnée réelle
};

struct WorldState {
  uint32_t schemaVersion;
  uint32_t seed;
  uint64_t startEpochSeconds;
  uint32_t simulatedDays;
  uint32_t randomState;
  uint32_t eventCount;
  uint8_t tension;
  uint8_t anomalyLevel;
  uint8_t factionActivity;
  WorldEvent latestEvent;
};

struct AdvanceSummary {
  uint32_t daysProcessed;
  uint32_t eventsCreated;
  bool catchUpPending;
  bool hasLatestEvent;
  WorldEvent latestEvent;
};

// Initialise un monde déterministe à partir d'une graine et d'une heure fiable.
// Une graine identique produit la même évolution pour les mêmes dates.
void initializeWorld(WorldState& world, uint32_t seed, uint64_t startEpochSeconds);

// Fait évoluer le monde jusqu'au jour correspondant à nowEpochSeconds.
// Une heure nulle ou antérieure au démarrage ne fait jamais reculer le monde.
AdvanceSummary advanceWorld(WorldState& world, uint64_t nowEpochSeconds);

const char* eventTypeName(EventType type);

}  // namespace hawkins
