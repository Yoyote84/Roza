#pragma once

#include <stddef.h>
#include <stdint.h>

namespace hawkins {

constexpr uint32_t kExplorationSchemaVersion = 1U;
constexpr size_t kTrackedCellCapacity = 24U;
constexpr double kExplorationCellSizeMeters = 250.0;
constexpr uint32_t kExplorationConfirmMs = 10000U;
constexpr uint16_t kExplorationSaveFormatVersion = 1U;
constexpr size_t kExplorationSaveHeaderSize = 16U;
constexpr size_t kExplorationSavePayloadSize = 116U;
constexpr size_t kExplorationSaveEncodedSize =
    kExplorationSaveHeaderSize + kExplorationSavePayloadSize;

struct ExplorationState {
  uint32_t schemaVersion;
  uint32_t sectorsVisited;
  uint32_t discoveries;
  uint32_t lastCellKey;
  uint32_t knownCellKeys[kTrackedCellCapacity];
  uint8_t knownCount;
  uint8_t replaceIndex;
  uint8_t hasOrigin;
  uint8_t reserved;
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

// Convertit une coordonnée WGS-84 en identifiant de maille.
// Seul cet identifiant est conservé : aucune latitude/longitude brute n'est sauvegardée.
bool makeExplorationCellKey(double latitude, double longitude,
                            uint32_t worldSeed, uint32_t& cellKey);

ExplorationResult observeExplorationCell(ExplorationState& state,
                                         uint32_t cellKey);

// Format binaire explicite et protégé par CRC-32 pour les deux emplacements NVS.
bool encodeExplorationSave(const ExplorationState& state, uint32_t sequence,
                           uint8_t* output, size_t capacity);
bool decodeExplorationSave(const uint8_t* input, size_t length,
                           ExplorationState& state, uint32_t& sequence);

}  // namespace hawkins
