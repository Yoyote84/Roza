#include "hawkins_missions.h"

#include <string.h>

namespace hawkins {
namespace {

const MissionDef kMissions[kMissionCount] = {
    {"LE PREMIER SIGNAL", "Decouvrir un premier signal anormal", 1U},
    {"CARTOGRAPHIE", "Explorer trois secteurs distincts", 3U},
    {"PISTE DE L'ANOMALIE", "Trouver deux signaux", 2U},
};

constexpr uint32_t kSaveMagic = 0x484D5748U;  // "HWMH" en little-endian
constexpr uint16_t kSaveFormatVersion = 1U;
constexpr size_t kCrcOffset = 12U;

void write16(uint8_t* output, uint16_t value) {
  output[0] = static_cast<uint8_t>(value);
  output[1] = static_cast<uint8_t>(value >> 8U);
}

void write32(uint8_t* output, uint32_t value) {
  for (unsigned i = 0; i < 4U; ++i) {
    output[i] = static_cast<uint8_t>(value >> (8U * i));
  }
}

uint16_t read16(const uint8_t* input) {
  return static_cast<uint16_t>(input[0]) |
         static_cast<uint16_t>(static_cast<uint16_t>(input[1]) << 8U);
}

uint32_t read32(const uint8_t* input) {
  uint32_t value = 0U;
  for (unsigned i = 0; i < 4U; ++i) {
    value |= static_cast<uint32_t>(input[i]) << (8U * i);
  }
  return value;
}

uint32_t crc32(const uint8_t* input, size_t length) {
  uint32_t crc = 0xFFFFFFFFU;
  for (size_t i = 0; i < length; ++i) {
    crc ^= input[i];
    for (unsigned bit = 0; bit < 8U; ++bit) {
      const uint32_t mask = 0U - (crc & 1U);
      crc = (crc >> 1U) ^ (0xEDB88320U & mask);
    }
  }
  return ~crc;
}

void encodePayload(const MissionState& state, uint8_t* output) {
  write32(output, state.schemaVersion);
  for (size_t i = 0; i < kMissionCount; ++i) {
    output[4U + i] = state.status[i];
  }
  write32(output + 7U, state.sectors);
  write32(output + 11U, state.discoveries);
  write32(output + 15U, state.lastCellKey);
  output[19] = state.hasLastCell;
}

bool decodePayload(const uint8_t* input, MissionState& state) {
  state = MissionState{};
  state.schemaVersion = read32(input);
  for (size_t i = 0; i < kMissionCount; ++i) {
    state.status[i] = input[4U + i];
  }
  state.sectors = read32(input + 7U);
  state.discoveries = read32(input + 11U);
  state.lastCellKey = read32(input + 15U);
  state.hasLastCell = input[19];
  return isValidMissionState(state);
}

}  // namespace

const MissionDef& missionDefinition(MissionId id) {
  const uint8_t index = static_cast<uint8_t>(id);
  return kMissions[index < kMissionCount ? index : 0U];
}

void initializeMissions(MissionState& state) {
  state = MissionState{};
  state.schemaVersion = kMissionSchemaVersion;
  state.status[static_cast<uint8_t>(MissionId::FirstSignal)] =
      static_cast<uint8_t>(MissionStatus::Active);
}

bool isValidMissionState(const MissionState& state) {
  if (state.schemaVersion != kMissionSchemaVersion || state.hasLastCell > 1U ||
      (state.hasLastCell == 0U && state.lastCellKey != 0U)) return false;

  for (size_t i = 0; i < kMissionCount; ++i) {
    if (state.status[i] > static_cast<uint8_t>(MissionStatus::Complete)) {
      return false;
    }
  }

  if (missionStatus(state, MissionId::FirstSignal) == MissionStatus::Locked) {
    return false;
  }
  if (missionStatus(state, MissionId::ThreeSectors) != MissionStatus::Locked &&
      missionStatus(state, MissionId::FirstSignal) != MissionStatus::Complete) {
    return false;
  }
  if (missionStatus(state, MissionId::AnomalyTrail) != MissionStatus::Locked &&
      missionStatus(state, MissionId::ThreeSectors) != MissionStatus::Complete) {
    return false;
  }

  if (missionStatus(state, MissionId::FirstSignal) == MissionStatus::Complete &&
      state.discoveries < kMissions[0].target) return false;
  if (missionStatus(state, MissionId::ThreeSectors) == MissionStatus::Complete &&
      state.sectors < kMissions[1].target) return false;
  if (missionStatus(state, MissionId::AnomalyTrail) == MissionStatus::Complete &&
      state.discoveries < kMissions[2].target) return false;

  return true;
}

MissionStatus missionStatus(const MissionState& state, MissionId id) {
  const uint8_t index = static_cast<uint8_t>(id);
  if (index >= kMissionCount) return MissionStatus::Locked;
  return static_cast<MissionStatus>(state.status[index]);
}

uint8_t recordMissionSector(MissionState& state, uint32_t cellKey,
                            bool discovery) {
  if (!isValidMissionState(state) || cellKey == 0U) return 0U;
  if (state.hasLastCell != 0U && state.lastCellKey == cellKey) return 0U;

  state.lastCellKey = cellKey;
  state.hasLastCell = 1U;
  if (state.sectors < UINT32_MAX) ++state.sectors;
  if (discovery && state.discoveries < UINT32_MAX) ++state.discoveries;

  uint8_t completedMask = 0U;
  bool changed = true;
  while (changed) {
    changed = false;
    for (uint8_t i = 0; i < kMissionCount; ++i) {
      if (state.status[i] != static_cast<uint8_t>(MissionStatus::Active)) {
        continue;
      }
      const uint32_t progress =
          i == static_cast<uint8_t>(MissionId::ThreeSectors)
              ? state.sectors
              : state.discoveries;
      if (progress < kMissions[i].target) continue;

      state.status[i] = static_cast<uint8_t>(MissionStatus::Complete);
      completedMask = static_cast<uint8_t>(completedMask | (1U << i));
      if (i + 1U < kMissionCount) {
        state.status[i + 1U] = static_cast<uint8_t>(MissionStatus::Active);
      }
      changed = true;
    }
  }
  return completedMask;
}

uint16_t missionRewardXp(MissionId id) {
  switch (id) {
    case MissionId::FirstSignal: return 100U;
    case MissionId::ThreeSectors: return 75U;
    case MissionId::AnomalyTrail: return 150U;
    default: return 0U;
  }
}

bool encodeMissionSave(const MissionState& state, uint32_t sequence,
                       uint8_t* output, size_t capacity) {
  if (!output || capacity < kMissionSaveEncodedSize || sequence == 0U ||
      !isValidMissionState(state)) {
    return false;
  }

  memset(output, 0, kMissionSaveEncodedSize);
  write32(output, kSaveMagic);
  write16(output + 4U, kSaveFormatVersion);
  write16(output + 6U, static_cast<uint16_t>(kMissionSavePayloadSize));
  write32(output + 8U, sequence);
  encodePayload(state, output + kMissionSaveHeaderSize);
  write32(output + kCrcOffset, crc32(output, kMissionSaveEncodedSize));
  return true;
}

bool decodeMissionSave(const uint8_t* input, size_t length,
                       MissionState& state, uint32_t& sequence) {
  if (!input || length != kMissionSaveEncodedSize ||
      read32(input) != kSaveMagic ||
      read16(input + 4U) != kSaveFormatVersion ||
      read16(input + 6U) != kMissionSavePayloadSize) {
    return false;
  }

  uint8_t check[kMissionSaveEncodedSize];
  memcpy(check, input, sizeof(check));
  const uint32_t storedCrc = read32(check + kCrcOffset);
  write32(check + kCrcOffset, 0U);
  if (crc32(check, sizeof(check)) != storedCrc) return false;

  const uint32_t candidateSequence = read32(input + 8U);
  if (candidateSequence == 0U) return false;

  MissionState decoded{};
  if (!decodePayload(input + kMissionSaveHeaderSize, decoded)) return false;
  state = decoded;
  sequence = candidateSequence;
  return true;
}

}  // namespace hawkins
