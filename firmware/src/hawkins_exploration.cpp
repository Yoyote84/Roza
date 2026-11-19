#include "hawkins_exploration.h"

#include <math.h>

namespace hawkins {
namespace {

constexpr uint32_t kSaveMagic = 0x45585748U;  // "HWXE" en little-endian
constexpr size_t kCrcOffset = 12U;
constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthRadiusMeters = 6378137.0;
constexpr double kMercatorMaxLatitude = 85.05112878;

void write16(uint8_t* p, uint16_t value) {
  p[0] = static_cast<uint8_t>(value);
  p[1] = static_cast<uint8_t>(value >> 8U);
}

void write32(uint8_t* p, uint32_t value) {
  for (unsigned i = 0; i < 4U; ++i) {
    p[i] = static_cast<uint8_t>(value >> (8U * i));
  }
}

uint16_t read16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) |
         static_cast<uint16_t>(static_cast<uint16_t>(p[1]) << 8U);
}

uint32_t read32(const uint8_t* p) {
  uint32_t value = 0;
  for (unsigned i = 0; i < 4U; ++i) {
    value |= static_cast<uint32_t>(p[i]) << (8U * i);
  }
  return value;
}

uint32_t crc32(const uint8_t* bytes, size_t length) {
  uint32_t crc = 0xFFFFFFFFU;
  for (size_t i = 0; i < length; ++i) {
    crc ^= bytes[i];
    for (unsigned bit = 0; bit < 8U; ++bit) {
      const uint32_t mask = 0U - (crc & 1U);
      crc = (crc >> 1U) ^ (0xEDB88320U & mask);
    }
  }
  return ~crc;
}

uint32_t hashCell(int32_t x, int32_t y, uint32_t seed) {
  uint32_t hash = 2166136261U ^ seed;
  const uint32_t values[2] = {
      static_cast<uint32_t>(x), static_cast<uint32_t>(y)};
  for (unsigned n = 0; n < 2U; ++n) {
    for (unsigned byte = 0; byte < 4U; ++byte) {
      hash ^= static_cast<uint8_t>(values[n] >> (byte * 8U));
      hash *= 16777619U;
    }
  }
  // Mélange final pour mieux répartir les cellules voisines.
  hash ^= hash >> 16U;
  hash *= 0x7FEB352DU;
  hash ^= hash >> 15U;
  hash *= 0x846CA68BU;
  hash ^= hash >> 16U;
  return hash == 0U ? 1U : hash;
}

void encodePayload(const ExplorationState& state, uint8_t* output) {
  write32(output + 0U, state.schemaVersion);
  write32(output + 4U, state.sectorsVisited);
  write32(output + 8U, state.discoveries);
  write32(output + 12U, state.lastCellKey);
  for (size_t i = 0; i < kTrackedCellCapacity; ++i) {
    write32(output + 16U + i * 4U, state.knownCellKeys[i]);
  }
  output[112] = state.knownCount;
  output[113] = state.replaceIndex;
  output[114] = state.hasOrigin;
  output[115] = 0U;
}

bool decodePayload(const uint8_t* input, ExplorationState& state) {
  state = ExplorationState{};
  state.schemaVersion = read32(input + 0U);
  state.sectorsVisited = read32(input + 4U);
  state.discoveries = read32(input + 8U);
  state.lastCellKey = read32(input + 12U);
  for (size_t i = 0; i < kTrackedCellCapacity; ++i) {
    state.knownCellKeys[i] = read32(input + 16U + i * 4U);
  }
  state.knownCount = input[112];
  state.replaceIndex = input[113];
  state.hasOrigin = input[114];
  state.reserved = input[115];
  return isValidExplorationState(state);
}

}  // namespace

void initializeExploration(ExplorationState& state) {
  state = ExplorationState{};
  state.schemaVersion = kExplorationSchemaVersion;
}

bool isValidExplorationState(const ExplorationState& state) {
  if (state.schemaVersion != kExplorationSchemaVersion ||
      state.knownCount > kTrackedCellCapacity ||
      state.replaceIndex >= kTrackedCellCapacity ||
      state.hasOrigin > 1U || state.discoveries > state.sectorsVisited ||
      state.reserved != 0U) {
    return false;
  }
  if (state.hasOrigin == 0U) {
    return state.knownCount == 0U && state.sectorsVisited == 0U &&
           state.discoveries == 0U && state.lastCellKey == 0U;
  }
  if (state.knownCount == 0U) return false;

  bool lastFound = false;
  for (size_t i = 0; i < state.knownCount; ++i) {
    if (state.knownCellKeys[i] == state.lastCellKey) lastFound = true;
    for (size_t j = i + 1U; j < state.knownCount; ++j) {
      if (state.knownCellKeys[i] == state.knownCellKeys[j]) return false;
    }
  }
  return lastFound;
}

bool makeExplorationCellKey(double latitude, double longitude,
                            uint32_t worldSeed, uint32_t& cellKey) {
  if (!isfinite(latitude) || !isfinite(longitude) ||
      latitude < -90.0 || latitude > 90.0 ||
      longitude < -180.0 || longitude > 180.0) {
    return false;
  }

  double lat = latitude;
  if (lat > kMercatorMaxLatitude) lat = kMercatorMaxLatitude;
  if (lat < -kMercatorMaxLatitude) lat = -kMercatorMaxLatitude;
  const double latRadians = lat * kPi / 180.0;
  const double lonRadians = longitude * kPi / 180.0;
  const double xMeters = kEarthRadiusMeters * lonRadians;
  const double yMeters =
      kEarthRadiusMeters * log(tan(kPi / 4.0 + latRadians / 2.0));
  const double xCell = floor(xMeters / kExplorationCellSizeMeters);
  const double yCell = floor(yMeters / kExplorationCellSizeMeters);

  if (xCell < -1000000.0 || xCell > 1000000.0 ||
      yCell < -1000000.0 || yCell > 1000000.0) {
    return false;
  }
  cellKey = hashCell(static_cast<int32_t>(xCell),
                     static_cast<int32_t>(yCell), worldSeed);
  return true;
}

ExplorationResult observeExplorationCell(ExplorationState& state,
                                         uint32_t cellKey) {
  if (!isValidExplorationState(state) || cellKey == 0U) {
    return ExplorationResult::Invalid;
  }

  if (state.hasOrigin == 0U) {
    state.hasOrigin = 1U;
    state.lastCellKey = cellKey;
    state.knownCellKeys[0] = cellKey;
    state.knownCount = 1U;
    return ExplorationResult::OriginSet;
  }

  if (state.lastCellKey == cellKey) return ExplorationResult::SameCell;
  for (size_t i = 0; i < state.knownCount; ++i) {
    if (state.knownCellKeys[i] == cellKey) {
      state.lastCellKey = cellKey;
      return ExplorationResult::Revisited;
    }
  }

  if (state.sectorsVisited < UINT32_MAX) ++state.sectorsVisited;
  if (state.knownCount < kTrackedCellCapacity) {
    state.knownCellKeys[state.knownCount++] = cellKey;
  } else {
    state.knownCellKeys[state.replaceIndex] = cellKey;
    state.replaceIndex = static_cast<uint8_t>(
        (state.replaceIndex + 1U) % kTrackedCellCapacity);
  }
  state.lastCellKey = cellKey;

  // Une découverte narrative est garantie tous les trois nouveaux secteurs.
  // Le jeu reste entièrement déterministe et ne dépend d'aucune API distante.
  if (state.sectorsVisited != 0U && state.sectorsVisited % 3U == 0U) {
    if (state.discoveries < UINT32_MAX) ++state.discoveries;
    return ExplorationResult::NewDiscovery;
  }
  return ExplorationResult::NewSector;
}

bool encodeExplorationSave(const ExplorationState& state, uint32_t sequence,
                           uint8_t* output, size_t capacity) {
  if (!output || capacity < kExplorationSaveEncodedSize ||
      sequence == 0U || !isValidExplorationState(state)) {
    return false;
  }
  for (size_t i = 0; i < kExplorationSaveEncodedSize; ++i) output[i] = 0U;
  write32(output, kSaveMagic);
  write16(output + 4U, kExplorationSaveFormatVersion);
  write16(output + 6U, static_cast<uint16_t>(kExplorationSavePayloadSize));
  write32(output + 8U, sequence);
  encodePayload(state, output + kExplorationSaveHeaderSize);
  write32(output + kCrcOffset, crc32(output, kExplorationSaveEncodedSize));
  return true;
}

bool decodeExplorationSave(const uint8_t* input, size_t length,
                           ExplorationState& state, uint32_t& sequence) {
  if (!input || length != kExplorationSaveEncodedSize ||
      read32(input) != kSaveMagic ||
      read16(input + 4U) != kExplorationSaveFormatVersion ||
      read16(input + 6U) != kExplorationSavePayloadSize) {
    return false;
  }

  uint8_t check[kExplorationSaveEncodedSize];
  for (size_t i = 0; i < kExplorationSaveEncodedSize; ++i) check[i] = input[i];
  const uint32_t storedCrc = read32(check + kCrcOffset);
  write32(check + kCrcOffset, 0U);
  if (crc32(check, sizeof(check)) != storedCrc) return false;

  const uint32_t candidateSequence = read32(input + 8U);
  if (candidateSequence == 0U) return false;
  ExplorationState decoded{};
  if (!decodePayload(input + kExplorationSaveHeaderSize, decoded)) return false;
  state = decoded;
  sequence = candidateSequence;
  return true;
}

}  // namespace hawkins
