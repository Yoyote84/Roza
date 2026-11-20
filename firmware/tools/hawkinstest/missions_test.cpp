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
  assert(first == 0U);
  assert(missionStatus(state, MissionId::FirstSignal) == MissionStatus::Active);
  assert(missionStatus(state, MissionId::ThreeSectors) == MissionStatus::Locked);
  assert(missionStatus(state, MissionId::AnomalyTrail) == MissionStatus::Locked);
  assert(state.sectors == 3U && state.discoveries == 1U);

  // La scene radio se termine seulement lorsque le joueur repond a Lia.
  assert(completeFirstSignal(state));
  assert(missionStatus(state, MissionId::FirstSignal) == MissionStatus::Complete);
  // Les trois secteurs deja parcourus sont reconnus au deverrouillage.
  assert(missionStatus(state, MissionId::ThreeSectors) == MissionStatus::Complete);
  assert(missionStatus(state, MissionId::AnomalyTrail) == MissionStatus::Active);

  const uint8_t last = recordMissionSector(state, 400U, true);
  assert((last & 0x04U) != 0U);
  assert(missionStatus(state, MissionId::AnomalyTrail) == MissionStatus::Complete);
  assert(completeFirstSignal(state)); // choix/migration idempotent
  assert(isValidMissionState(state));
  assert(missionRewardXp(MissionId::FirstSignal) == 100U);
  assert(missionRewardXp(MissionId::ThreeSectors) == 75U);
  assert(missionRewardXp(MissionId::AnomalyTrail) == 150U);
}

static void testMissionDuplicateAndCodec() {
  MissionState original{};
  initializeMissions(original);
  assert(completeFirstSignal(original));
  assert(recordMissionSector(original, 0x1234U, true) == 0U);
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

static void testFirstSignalCannotBeCompletedByGpsAlone() {
  MissionState state{};
  initializeMissions(state);
  for (uint32_t key = 1U; key <= 3U; ++key) {
    recordMissionSector(state, key, key == 3U);
  }
  assert(state.discoveries == 1U);
  assert(missionStatus(state, MissionId::FirstSignal) == MissionStatus::Active);
  assert(missionStatus(state, MissionId::ThreeSectors) == MissionStatus::Locked);
  assert(completeFirstSignal(state));
  assert(missionStatus(state, MissionId::FirstSignal) == MissionStatus::Complete);
  assert(missionStatus(state, MissionId::ThreeSectors) == MissionStatus::Complete);
  assert(missionStatus(state, MissionId::AnomalyTrail) == MissionStatus::Active);

  uint8_t encoded[kMissionSaveEncodedSize];
  assert(encodeMissionSave(state, 8U, encoded, sizeof(encoded)));
  MissionState restored{};
  uint32_t sequence = 0U;
  assert(decodeMissionSave(encoded, sizeof(encoded), restored, sequence));
  assert(sequence == 8U);
  assert(missionStatus(restored, MissionId::FirstSignal) == MissionStatus::Complete);
  assert(missionStatus(restored, MissionId::ThreeSectors) == MissionStatus::Complete);
  assert(missionStatus(restored, MissionId::AnomalyTrail) == MissionStatus::Active);
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
  testFirstSignalCannotBeCompletedByGpsAlone();
  testInvalidMissionState();
  puts("HAWKINS MISSIONS: progression, dedoublonnage et CRC OK.");
  return 0;
}
