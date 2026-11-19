#include "hawkins_missions.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

using namespace hawkins;

static void testMissionSequence() {
  MissionState state{};
  initializeMissions(state);
  assert(isValidMissionState(state));
  assert(missionStatus(state, MissionId::FirstSignal) == MissionStatus::Active);
  assert(missionStatus(state, MissionId::ThreeSectors) == MissionStatus::Locked);

  assert(recordMissionSector(state, 100U, false) == 0U);
  assert(recordMissionSector(state, 200U, false) == 0U);
  const uint8_t first = recordMissionSector(state, 300U, true);
  assert((first & 0x01U) != 0U);
  assert((first & 0x02U) != 0U);  // Cartographie deja remplie a son deblocage.
  assert(missionStatus(state, MissionId::FirstSignal) == MissionStatus::Complete);
  assert(missionStatus(state, MissionId::ThreeSectors) == MissionStatus::Complete);
  assert(missionStatus(state, MissionId::AnomalyTrail) == MissionStatus::Active);

  const uint8_t last = recordMissionSector(state, 400U, true);
  assert((last & 0x04U) != 0U);
  assert(missionStatus(state, MissionId::AnomalyTrail) == MissionStatus::Complete);
  assert(isValidMissionState(state));
  assert(missionRewardXp(MissionId::FirstSignal) == 100U);
  assert(missionRewardXp(MissionId::ThreeSectors) == 75U);
  assert(missionRewardXp(MissionId::AnomalyTrail) == 150U);
}

static void testMissionDuplicateAndCodec() {
  MissionState original{};
  initializeMissions(original);
  assert(recordMissionSector(original, 0x1234U, true) == 0x01U);
  const uint32_t sectorsBefore = original.sectors;
  const uint32_t discoveriesBefore = original.discoveries;
  assert(recordMissionSector(original, 0x1234U, true) == 0U);
  assert(original.sectors == sectorsBefore);
  assert(original.discoveries == discoveriesBefore);

  uint8_t encoded[kMissionSaveEncodedSize];
  assert(encodeMissionSave(original, 7U, encoded, sizeof(encoded)));
  MissionState restored{};
  uint32_t sequence = 0U;
  assert(decodeMissionSave(encoded, sizeof(encoded), restored, sequence));
  assert(sequence == 7U);
  assert(restored.sectors == original.sectors);
  assert(restored.discoveries == original.discoveries);
  assert(restored.status[0] == original.status[0]);
  assert(restored.lastCellKey == original.lastCellKey);
  assert(restored.hasLastCell == original.hasLastCell);

  encoded[20] ^= 0x20U;
  assert(!decodeMissionSave(encoded, sizeof(encoded), restored, sequence));
  assert(!decodeMissionSave(encoded, sizeof(encoded) - 1U, restored, sequence));
  assert(!encodeMissionSave(original, 0U, encoded, sizeof(encoded)));
}

static void testInvalidMissionState() {
  MissionState invalid{};
  initializeMissions(invalid);
  invalid.status[2] = static_cast<uint8_t>(MissionStatus::Active);
  assert(!isValidMissionState(invalid));
  initializeMissions(invalid);
  invalid.hasLastCell = 2U;
  assert(!isValidMissionState(invalid));
}

int main() {
  testMissionSequence();
  testMissionDuplicateAndCodec();
  testInvalidMissionState();
  puts("HAWKINS MISSIONS: progression, dedoublonnage et CRC OK.");
  return 0;
}
