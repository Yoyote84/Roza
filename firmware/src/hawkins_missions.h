#pragma once

#include <stddef.h>
#include <stdint.h>

namespace hawkins {

constexpr uint32_t kMissionSchemaVersion = 1U;
constexpr uint8_t kMissionCount = 3U;
constexpr size_t kMissionSaveHeaderSize = 16U;
constexpr size_t kMissionSavePayloadSize = 15U;
constexpr size_t kMissionSaveEncodedSize =
    kMissionSaveHeaderSize + kMissionSavePayloadSize;

enum class MissionId : uint8_t {
  FirstSignal = 0,
  ThreeSectors,
  AnomalyTrail,
  Count
};

enum class MissionStatus : uint8_t {
  Locked = 0,
  Active,
  Complete
};

struct MissionState {
  uint32_t schemaVersion;
  uint8_t status[kMissionCount];
  uint32_t sectors;
  uint32_t discoveries;
};

struct MissionDef {
  const char* title;
  const char* description;
  uint32_t target;
};

const MissionDef& missionDefinition(MissionId id);
void initializeMissions(MissionState& state);
bool isValidMissionState(const MissionState& state);

// Enregistre un nouveau secteur et renvoie le masque des missions terminees
// par cet evenement. Chaque bit correspond a MissionId.
uint8_t recordMissionSector(MissionState& state, bool discovery);
uint16_t missionRewardXp(MissionId id);
MissionStatus missionStatus(const MissionState& state, MissionId id);

bool encodeMissionSave(const MissionState& state, uint32_t sequence,
                      uint8_t* output, size_t capacity);
bool decodeMissionSave(const uint8_t* input, size_t length,
                       MissionState& state, uint32_t& sequence);

}  // namespace hawkins
