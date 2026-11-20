#include "hawkins_exploration.h"

#include <math.h>
#include <string.h>

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
  uint32_t value = 0U;
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

uint32_t mix32(uint32_t value) {
  value ^= value >> 16U;
  value *= 0x7FEB352DU;
  value ^= value >> 15U;
  value *= 0x846CA68BU;
  value ^= value >> 16U;
  return value;
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
  hash = mix32(hash);
  return hash == 0U ? 1U : hash;
}

void encodePayload(const ExplorationState& state, uint8_t* output) {
  write32(output + 0U, state.schemaVersion);
  write32(output + 4U, state.sectorsVisited);
  write32(output + 8U, state.discoveries);
  write32(output + 12U, state.lastCellKey);
  for (size_t i = 0U; i < kTrackedCellCapacity; ++i) {
    write32(output + 16U + i * 4U, state.knownCellKeys[i]);
  }
  output[528] = state.knownCount;
  output[529] = state.replaceIndex;
  output[530] = state.hasOrigin;
  output[531] = state.reserved;
  write32(output + 532U, state.lastEventCellKey);
  write32(output + 536U, state.areaEventCount);
  output[540] = static_cast<uint8_t>(state.lastEventType);
  output[541] = state.lastEventIntensity;
  output[542] = static_cast<uint8_t>(state.eventReserved);
  output[543] = static_cast<uint8_t>(state.eventReserved >> 8U);
}

bool decodeCurrentPayload(const uint8_t* input, ExplorationState& state) {
  state = ExplorationState{};
  state.schemaVersion = read32(input + 0U);
  state.sectorsVisited = read32(input + 4U);
  state.discoveries = read32(input + 8U);
  state.lastCellKey = read32(input + 12U);
  for (size_t i = 0U; i < kTrackedCellCapacity; ++i) {
    state.knownCellKeys[i] = read32(input + 16U + i * 4U);
  }
  state.knownCount = input[528];
  state.replaceIndex = input[529];
  state.hasOrigin = input[530];
  state.reserved = input[531];
  state.lastEventCellKey = read32(input + 532U);
  state.areaEventCount = read32(input + 536U);
  state.lastEventType = static_cast<EventType>(input[540]);
  state.lastEventIntensity = input[541];
  state.eventReserved = static_cast<uint16_t>(input[542]) |
      static_cast<uint16_t>(static_cast<uint16_t>(input[543]) << 8U);
  return isValidExplorationState(state);
}

bool decodeLegacyPayload(const uint8_t* input, ExplorationState& state) {
  state = ExplorationState{};
  if (read32(input + 0U) != kLegacyExplorationSchemaVersion) return false;

  // Les 24 entrées historiques sont copiées telles quelles dans la nouvelle
  // mémoire de 128 secteurs. Les nouveaux emplacements restent initialisés à zéro.
  state.schemaVersion = kExplorationSchemaVersion;
  state.sectorsVisited = read32(input + 4U);
  state.discoveries = read32(input + 8U);
  state.lastCellKey = read32(input + 12U);
  for (size_t i = 0U; i < kLegacyTrackedCellCapacity; ++i) {
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
      state.reserved != 0U || state.eventReserved != 0U) {
    return false;
  }

  const uint8_t eventType = static_cast<uint8_t>(state.lastEventType);
  if (state.areaEventCount == 0U) {
    if (state.lastEventCellKey != 0U || eventType != 0U ||
        state.lastEventIntensity != 0U) return false;
  } else if (state.lastEventCellKey == 0U ||
             eventType < static_cast<uint8_t>(EventType::StrangeSignal) ||
             eventType > static_cast<uint8_t>(EventType::PortalFluctuation) ||
             state.lastEventIntensity == 0U ||
             state.lastEventIntensity > 100U) {
    return false;
  }

  if (state.hasOrigin == 0U) {
    return state.knownCount == 0U && state.sectorsVisited == 0U &&
           state.discoveries == 0U && state.lastCellKey == 0U &&
           state.areaEventCount == 0U;
  }
  if (state.knownCount == 0U) return false;

  bool lastFound = false;
  for (size_t i = 0U; i < state.knownCount; ++i) {
    if (state.knownCellKeys[i] == 0U) return false;
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
  for (size_t i = 0U; i < state.knownCount; ++i) {
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

  // Une découverte narrative reste garantie tous les trois nouveaux secteurs.
  if (state.sectorsVisited != 0U && state.sectorsVisited % 3U == 0U) {
    if (state.discoveries < UINT32_MAX) ++state.discoveries;
    return ExplorationResult::NewDiscovery;
  }
  return ExplorationResult::NewSector;
}

bool generateLocalAreaEvent(ExplorationState& state, const WorldState& world,
                            uint32_t cellKey, bool discovery) {
  if (!isValidExplorationState(state) || cellKey == 0U ||
      state.hasOrigin == 0U || state.lastCellKey != cellKey ||
      state.lastEventCellKey == cellKey ||
      state.areaEventCount == UINT32_MAX ||
      world.schemaVersion != kWorldSchemaVersion ||
      world.tension > 100U || world.anomalyLevel > 100U ||
      world.factionActivity > 100U) {
    return false;
  }

  uint32_t entropy = world.seed ^ cellKey ^
      (state.sectorsVisited * 0x9E3779B9U) ^
      (world.simulatedDays * 0x85EBCA6BU);
  entropy = mix32(entropy);
  const uint32_t eventRoll = entropy % 100U;
  uint32_t eventChance = 18U + world.anomalyLevel / 2U +
                         world.tension / 4U + world.factionActivity / 8U;
  if (eventChance > 85U) eventChance = 85U;
  if (!discovery && eventRoll >= eventChance) return false;

  const uint32_t typeRoll = mix32(entropy ^ 0xA5A5F00DU) % 100U;
  EventType type = EventType::StrangeSignal;
  if (discovery) {
    type = EventType::StrangeSignal;
  } else if (world.anomalyLevel >= 65U && typeRoll < 35U) {
    type = EventType::PortalFluctuation;
  } else if (world.factionActivity >= 60U && typeRoll < 55U) {
    type = EventType::FactionMovement;
  } else if (world.tension >= 55U && typeRoll < 75U) {
    type = EventType::CreatureTrace;
  } else {
    switch (typeRoll % 4U) {
      case 0U: type = EventType::PowerAnomaly; break;
      case 1U: type = EventType::DistressCall; break;
      case 2U: type = EventType::StrangeSignal; break;
      default: type = EventType::CreatureTrace; break;
    }
  }

  const uint32_t pressure = static_cast<uint32_t>(world.tension) +
      static_cast<uint32_t>(world.anomalyLevel) +
      static_cast<uint32_t>(world.factionActivity);
  const uint32_t intensityRange = pressure / 3U + 1U;
  uint32_t intensity = 1U +
      (mix32(entropy ^ 0xC2B2AE35U) % intensityRange);
  if (intensity > 100U) intensity = 100U;

  state.lastEventCellKey = cellKey;
  ++state.areaEventCount;
  state.lastEventType = type;
  state.lastEventIntensity = static_cast<uint8_t>(intensity);
  // Un signal apparu pendant un déplacement normal est une vraie découverte
  // pour la mission "Piste de l'anomalie". Une découverte garantie a déjà
  // incrémenté ce compteur dans observeExplorationCell().
  if (type == EventType::StrangeSignal && !discovery &&
      state.discoveries < UINT32_MAX) {
    ++state.discoveries;
  }
  return isValidExplorationState(state);
}

bool encodeExplorationSave(const ExplorationState& state, uint32_t sequence,
                           uint8_t* output, size_t capacity) {
  if (!output || capacity < kExplorationSaveEncodedSize ||
      sequence == 0U || !isValidExplorationState(state)) {
    return false;
  }
  memset(output, 0, kExplorationSaveEncodedSize);
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
  if (!input || length < kLegacyExplorationSaveEncodedSize ||
      read32(input) != kSaveMagic) return false;

  const uint16_t formatVersion = read16(input + 4U);
  const uint16_t payloadSize = read16(input + 6U);
  const bool currentFormat =
      formatVersion == kExplorationSaveFormatVersion &&
      payloadSize == kExplorationSavePayloadSize &&
      length == kExplorationSaveEncodedSize;
  const bool legacyFormat =
      formatVersion == kLegacyExplorationSaveFormatVersion &&
      payloadSize == kLegacyExplorationSavePayloadSize &&
      length == kLegacyExplorationSaveEncodedSize;
  if (!currentFormat && !legacyFormat) return false;

  uint8_t check[kExplorationSaveEncodedSize];
  memcpy(check, input, length);
  const uint32_t storedCrc = read32(check + kCrcOffset);
  write32(check + kCrcOffset, 0U);
  if (crc32(check, length) != storedCrc) return false;

  const uint32_t candidateSequence = read32(input + 8U);
  if (candidateSequence == 0U) return false;

  ExplorationState decoded{};
  const bool valid = currentFormat
      ? decodeCurrentPayload(input + kExplorationSaveHeaderSize, decoded)
      : decodeLegacyPayload(input + kExplorationSaveHeaderSize, decoded);
  if (!valid) return false;

  state = decoded;
  sequence = candidateSequence;
  return true;
}

}  // namespace hawkins
