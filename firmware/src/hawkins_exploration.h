#pragma once

#include "hawkins_world.h"
#include <stddef.h>
#include <stdint.h>

namespace hawkins {

constexpr uint32_t kLegacyExplorationSchemaVersion = 1U;
constexpr uint32_t kExplorationSchemaVersion = 2U;
constexpr size_t kLegacyTrackedCellCapacity = 24U;
constexpr size_t kTrackedCellCapacity = 128U;
constexpr double kExplorationCellSizeMeters = 250.0;
constexpr uint32_t kExplorationConfirmMs = 10000U;
constexpr uint16_t kLegacyExplorationSaveFormatVersion = 1U;
constexpr size_t kLegacyExplorationSavePayloadSize = 116U;
constexpr size_t kLegacyExplorationSaveEncodedSize = 132U;
constexpr uint16_t kExplorationSaveFormatVersion = 2U;
constexpr size_t kExplorationSaveHeaderSize = 16U;
constexpr size_t kExplorationSavePayloadSize = 544U;
constexpr size_t kExplorationSaveEncodedSize =
    kExplorationSaveHeaderSize + kExplorationSavePayloadSize;

struct ExplorationState {
  uint32_t schemaVersion;
  uint32_t sectorsVisited;
  uint32_t discoveries;
  uint32_t lastCellKey;
  // Mémoire circulaire des 128 dernières mailles visitées (identifiants hachés).
  uint32_t knownCellKeys[kTrackedCellCapacity];
  uint8_t knownCount;
  uint8_t replaceIndex;
  uint8_t hasOrigin;
  uint8_t reserved;
  // Dernier événement local et maille GPS exacte à laquelle il est rattaché.
  uint32_t lastEventCellKey;
  uint32_t areaEventCount;
  EventType lastEventType;
  uint8_t lastEventIntensity;
  uint16_t eventReserved;
};

enum class ExplorationResult : uint8_t {
  Invalid = 0,
  OriginSet,
  SameCell,
  Revisited,
  NewSector,
  NewDiscovery
};

void initializeExploration(ExplorationState& state);
bool isValidExplorationState(const ExplorationState& state);

// Convertit une coordonnée WGS-84 en identifiant de maille de 250 m.
// Seul cet identifiant haché est conservé : aucune coordonnée brute n'est sauvegardée.
bool makeExplorationCellKey(double latitude, double longitude,
                            uint32_t worldSeed, uint32_t& cellKey);

ExplorationResult observeExplorationCell(ExplorationState& state,
                                         uint32_t cellKey);

// Crée au plus un événement déterministe par secteur visité. Une découverte
// produit toujours un signal ; les autres événements dépendent de l'état du monde.
// L'événement est lié à cellKey, la maille GPS réelle, jamais à un lieu inventé.
bool generateLocalAreaEvent(ExplorationState& state, const WorldState& world,
                            uint32_t cellKey, bool discovery);

// Format binaire versionné et protégé par CRC-32. Le décodeur migre aussi les
// anciennes sauvegardes v1/24 secteurs sans effacer leur progression.
bool encodeExplorationSave(const ExplorationState& state, uint32_t sequence,
                           uint8_t* output, size_t capacity);
bool decodeExplorationSave(const uint8_t* input, size_t length,
                           ExplorationState& state, uint32_t& sequence);

}  // namespace hawkins
