#include "hawkins_world.h"
#include "hawkins_save_codec.h"
#include "hawkins_exploration.h"
#include "hawkins_character.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

using namespace hawkins;

static void testWrite16(uint8_t* p, uint16_t value) {
  p[0] = static_cast<uint8_t>(value);
  p[1] = static_cast<uint8_t>(value >> 8U);
}
static void testWrite32(uint8_t* p, uint32_t value) {
  for (unsigned i = 0U; i < 4U; ++i) {
    p[i] = static_cast<uint8_t>(value >> (8U * i));
  }
}
static uint32_t testCrc32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFFU;
  for (size_t i = 0U; i < length; ++i) {
    crc ^= data[i];
    for (unsigned bit = 0U; bit < 8U; ++bit) {
      const uint32_t mask = 0U - (crc & 1U);
      crc = (crc >> 1U) ^ (0xEDB88320U & mask);
    }
  }
  return ~crc;
}

static void testInitialization() {
  WorldState world{};
  initializeWorld(world, 12345U, 1800000000ULL);
  assert(world.schemaVersion == kWorldSchemaVersion);
  assert(world.seed == 12345U);
  assert(world.startEpochSeconds == 1800000000ULL);
  assert(world.simulatedDays == 0U);
  assert(world.eventCount == 0U);
  assert(world.tension == 12U);
  assert(world.anomalyLevel == 4U);
}

static void testTimeAndNoRewind() {
  WorldState world{};
  const uint64_t start = 1800000000ULL;
  initializeWorld(world, 7U, start);
  AdvanceSummary result = advanceWorld(world, start + kSecondsPerDay - 1U);
  assert(result.daysProcessed == 0U);
  assert(world.simulatedDays == 0U);
  result = advanceWorld(world, start + 3U * kSecondsPerDay);
  assert(result.daysProcessed == 3U);
  assert(world.simulatedDays == 3U);
  result = advanceWorld(world, start + kSecondsPerDay);
  assert(result.daysProcessed == 0U);
  assert(world.simulatedDays == 3U);
}

static void testDeterministicEvolution() {
  WorldState a{};
  WorldState b{};
  const uint64_t start = 1800000000ULL;
  initializeWorld(a, 0x12345678U, start);
  initializeWorld(b, 0x12345678U, start);
  const uint64_t now = start + 900U * kSecondsPerDay;
  AdvanceSummary ar = advanceWorld(a, now);
  AdvanceSummary br = advanceWorld(b, now);
  assert(ar.daysProcessed == 900U);
  assert(br.daysProcessed == 900U);
  assert(a.simulatedDays == b.simulatedDays);
  assert(a.randomState == b.randomState);
  assert(a.tension == b.tension);
  assert(a.anomalyLevel == b.anomalyLevel);
  assert(a.factionActivity == b.factionActivity);
  assert(a.eventCount == b.eventCount);
  assert(a.latestEvent.sequence == b.latestEvent.sequence);
  assert(a.latestEvent.type == b.latestEvent.type);
}

static void testTenYearWorldDoesNotEnd() {
  WorldState world{};
  const uint64_t start = 1800000000ULL;
  initializeWorld(world, 99U, start);
  const uint64_t now = start + 3652ULL * kSecondsPerDay;
  AdvanceSummary result = advanceWorld(world, now);
  assert(result.daysProcessed == 3652U);
  assert(!result.catchUpPending);
  assert(world.simulatedDays == 3652U);
  assert(world.eventCount > 0U);
}

static void testBoundedCatchUp() {
  WorldState world{};
  const uint64_t start = 1800000000ULL;
  initializeWorld(world, 42U, start);
  const uint64_t now = start + 9000ULL * kSecondsPerDay;
  AdvanceSummary first = advanceWorld(world, now);
  assert(first.daysProcessed == kMaxCatchUpDaysPerCall);
  assert(first.catchUpPending);
  uint32_t totalProcessed = first.daysProcessed;
  AdvanceSummary next = first;
  while (next.catchUpPending) {
    next = advanceWorld(world, now);
    totalProcessed += next.daysProcessed;
  }
  assert(totalProcessed == 9000U);
  assert(world.simulatedDays == 9000U);
}

static void testMetricsAndEventNames() {
  WorldState world{};
  initializeWorld(world, 3U, 1800000000ULL);
  for (uint32_t day = 1; day <= 5000U; ++day) {
    advanceWorld(world, world.startEpochSeconds +
                            static_cast<uint64_t>(day) * kSecondsPerDay);
  }
  assert(world.tension <= 100U);
  assert(world.anomalyLevel <= 100U);
  assert(world.factionActivity <= 100U);
  assert(world.latestEvent.intensity <= 100U);
  assert(world.latestEvent.sector < 32U);
  assert(eventTypeName(EventType::PortalFluctuation)[0] != '\0');
  assert(eventTypeName(EventType::None)[0] != '\0');
}

static void testSaveCodecRoundTrip() {
  WorldState original{};
  initializeWorld(original, 0xAABBCCDDU, 1800000000ULL);
  advanceWorld(original, original.startEpochSeconds + 900U * kSecondsPerDay);
  assert(isValidWorldState(original));
  uint8_t encoded[kWorldSaveEncodedSize];
  assert(encodeWorldSave(original, 17U, encoded, sizeof(encoded)));
  WorldState restored{};
  uint32_t seq = 0U;
  assert(decodeWorldSave(encoded, sizeof(encoded), restored, seq));
  assert(seq == 17U);
  assert(restored.schemaVersion == original.schemaVersion);
  assert(restored.seed == original.seed);
  assert(restored.startEpochSeconds == original.startEpochSeconds);
  assert(restored.simulatedDays == original.simulatedDays);
  assert(restored.randomState == original.randomState);
  assert(restored.eventCount == original.eventCount);
  assert(restored.latestEvent.sequence == original.latestEvent.sequence);
  assert(restored.latestEvent.type == original.latestEvent.type);
  assert(restored.latestEvent.intensity == original.latestEvent.intensity);
}

static void testSaveCodecRejectsDamage() {
  WorldState original{};
  initializeWorld(original, 55U, 1800000000ULL);
  uint8_t encoded[kWorldSaveEncodedSize];
  assert(encodeWorldSave(original, 1U, encoded, sizeof(encoded)));
  WorldState restored{};
  uint32_t seq = 0U;
  assert(!decodeWorldSave(encoded, sizeof(encoded) - 1U, restored, seq));
  encoded[20] ^= 0x40U;
  assert(!decodeWorldSave(encoded, sizeof(encoded), restored, seq));
  assert(!encodeWorldSave(original, 1U, encoded, sizeof(encoded) - 1U));
  assert(!encodeWorldSave(original, 0U, encoded, sizeof(encoded)));
}

static void testSaveCodecRejectsInvalidState() {
  WorldState invalid{};
  initializeWorld(invalid, 9U, 1800000000ULL);
  invalid.tension = 101U;
  uint8_t encoded[kWorldSaveEncodedSize];
  assert(!encodeWorldSave(invalid, 1U, encoded, sizeof(encoded)));
}

static void testOfflineWorldAdvance() {
  WorldState world{};
  initializeWorld(world, 0x12345678U, 1ULL);
  const AdvanceSummary first = advanceWorldByDays(world, 3U);
  assert(first.daysProcessed == 3U);
  assert(!first.catchUpPending);
  assert(world.simulatedDays == 3U);
  const AdvanceSummary capped = advanceWorldByDays(
      world, kMaxCatchUpDaysPerCall + 5U);
  assert(capped.daysProcessed == kMaxCatchUpDaysPerCall);
  assert(capped.catchUpPending);
  assert(world.simulatedDays == 3U + kMaxCatchUpDaysPerCall);
}

static void testExplorationCellGrid() {
  uint32_t a = 0U, nearby = 0U, far = 0U;
  assert(makeExplorationCellKey(48.8566, 2.3522, 77U, a));
  assert(makeExplorationCellKey(48.8567, 2.3523, 77U, nearby));
  assert(makeExplorationCellKey(48.8566, 2.3560, 77U, far));
  assert(a == nearby);
  assert(a != far);
  assert(!makeExplorationCellKey(91.0, 0.0, 77U, far));
  assert(!makeExplorationCellKey(0.0, NAN, 77U, far));
}

static void testExplorationVisitsAndDiscoveries() {
  ExplorationState state{};
  initializeExploration(state);
  assert(isValidExplorationState(state));
  assert(observeExplorationCell(state, 11U) == ExplorationResult::OriginSet);
  assert(state.sectorsVisited == 0U);
  assert(observeExplorationCell(state, 11U) == ExplorationResult::SameCell);
  assert(observeExplorationCell(state, 12U) == ExplorationResult::NewSector);
  assert(observeExplorationCell(state, 13U) == ExplorationResult::NewSector);
  assert(observeExplorationCell(state, 14U) == ExplorationResult::NewDiscovery);
  assert(state.sectorsVisited == 3U);
  assert(state.discoveries == 1U);
  assert(observeExplorationCell(state, 12U) == ExplorationResult::Revisited);
  assert(state.sectorsVisited == 3U);
  assert(isValidExplorationState(state));
}

static void testExplorationSaveCodec() {
  ExplorationState original{};
  initializeExploration(original);
  observeExplorationCell(original, 100U);
  observeExplorationCell(original, 200U);
  observeExplorationCell(original, 300U);
  assert(observeExplorationCell(original, 400U) ==
         ExplorationResult::NewDiscovery);
  WorldState world{};
  initializeWorld(world, 0x12345678U, 1ULL);
  assert(generateLocalAreaEvent(original, world, 400U, true));
  assert(original.lastEventCellKey == 400U);
  assert(original.lastEventType == EventType::StrangeSignal);
  assert(original.lastEventIntensity >= 1U &&
         original.lastEventIntensity <= 100U);

  uint8_t encoded[kExplorationSaveEncodedSize];
  assert(encodeExplorationSave(original, 9U, encoded, sizeof(encoded)));
  ExplorationState restored{};
  uint32_t sequence = 0U;
  assert(decodeExplorationSave(encoded, sizeof(encoded), restored, sequence));
  assert(sequence == 9U);
  assert(restored.schemaVersion == kExplorationSchemaVersion);
  assert(restored.sectorsVisited == original.sectorsVisited);
  assert(restored.discoveries == original.discoveries);
  assert(restored.lastCellKey == original.lastCellKey);
  assert(restored.knownCount == original.knownCount);
  assert(restored.areaEventCount == 1U);
  assert(restored.lastEventCellKey == 400U);
  assert(restored.lastEventType == EventType::StrangeSignal);
  assert(restored.lastEventIntensity == original.lastEventIntensity);
  encoded[24] ^= 0x20U;
  assert(!decodeExplorationSave(encoded, sizeof(encoded), restored, sequence));
  assert(!decodeExplorationSave(encoded, sizeof(encoded) - 1U, restored, sequence));
}

static void testExplorationMemoryTracks128RecentSectors() {
  static_assert(kTrackedCellCapacity == 128U, "La memoire doit couvrir 128 secteurs");
  ExplorationState state{};
  initializeExploration(state);
  assert(observeExplorationCell(state, 1U) == ExplorationResult::OriginSet);

  // 129 secteurs distincts font dépasser la fenêtre : les 128 plus récents
  // restent connus, tandis que le premier secteur ancien sort de la mémoire.
  for (uint32_t key = 2U; key <= 130U; ++key) {
    const ExplorationResult result = observeExplorationCell(state, key);
    assert(result == ExplorationResult::NewSector ||
           result == ExplorationResult::NewDiscovery);
  }
  assert(state.knownCount == kTrackedCellCapacity);
  assert(state.sectorsVisited == 129U);
  assert(observeExplorationCell(state, 130U) == ExplorationResult::SameCell);
  assert(observeExplorationCell(state, 2U) == ExplorationResult::NewSector);
  assert(state.sectorsVisited == 130U);
  assert(state.knownCount == kTrackedCellCapacity);
  assert(isValidExplorationState(state));
}

static void testLegacyExplorationSaveMigratesWithoutLosingVisits() {
  uint8_t legacy[kLegacyExplorationSaveEncodedSize] = {};
  testWrite32(legacy + 0U, 0x45585748U);
  testWrite16(legacy + 4U, kLegacyExplorationSaveFormatVersion);
  testWrite16(legacy + 6U, kLegacyExplorationSavePayloadSize);
  testWrite32(legacy + 8U, 44U);
  uint8_t* payload = legacy + kExplorationSaveHeaderSize;
  testWrite32(payload + 0U, kLegacyExplorationSchemaVersion);
  testWrite32(payload + 4U, 17U);
  testWrite32(payload + 8U, 5U);
  testWrite32(payload + 12U, 202U);
  testWrite32(payload + 16U, 101U);
  testWrite32(payload + 20U, 202U);
  payload[112] = 2U;
  payload[113] = 0U;
  payload[114] = 1U;
  payload[115] = 0U;
  testWrite32(legacy + 12U, testCrc32(legacy, sizeof(legacy)));

  ExplorationState restored{};
  uint32_t sequence = 0U;
  assert(decodeExplorationSave(legacy, sizeof(legacy), restored, sequence));
  assert(sequence == 44U);
  assert(restored.schemaVersion == kExplorationSchemaVersion);
  assert(restored.sectorsVisited == 17U);
  assert(restored.discoveries == 5U);
  assert(restored.lastCellKey == 202U);
  assert(restored.knownCount == 2U);
  assert(restored.knownCellKeys[0] == 101U);
  assert(restored.knownCellKeys[1] == 202U);
  assert(restored.areaEventCount == 0U);
  assert(isValidExplorationState(restored));

  uint8_t migrated[kExplorationSaveEncodedSize];
  assert(encodeExplorationSave(restored, sequence + 1U,
                               migrated, sizeof(migrated)));
  ExplorationState reread{};
  uint32_t migratedSequence = 0U;
  assert(decodeExplorationSave(migrated, sizeof(migrated),
                               reread, migratedSequence));
  assert(migratedSequence == 45U);
  assert(reread.sectorsVisited == 17U && reread.knownCount == 2U);
}

static void testLocalAreaEventsStayAttachedToTheirGpsSector() {
  WorldState world{};
  initializeWorld(world, 0xAABBCCDDU, 1ULL);
  // Pression élevée : ce secteur ordinaire déclenche un événement local
  // déterministe (sans être une découverte narrative garantie).
  world.tension = 100U;
  world.anomalyLevel = 100U;
  world.factionActivity = 0U;
  ExplorationState state{};
  initializeExploration(state);
  assert(observeExplorationCell(state, 0x11111111U) ==
         ExplorationResult::OriginSet);
  assert(observeExplorationCell(state, 0x22222222U) ==
         ExplorationResult::NewSector);

  const uint32_t eventCell = state.lastCellKey;
  ExplorationState sameState = state;
  assert(generateLocalAreaEvent(state, world, eventCell, false));
  assert(generateLocalAreaEvent(sameState, world, eventCell, false));
  assert(state.areaEventCount == 1U);
  assert(state.lastEventCellKey == eventCell);
  assert(state.lastEventCellKey == state.lastCellKey);
  assert(state.lastEventType == EventType::CreatureTrace);
  assert(state.lastEventIntensity == sameState.lastEventIntensity);
  assert(!generateLocalAreaEvent(state, world, eventCell, false));
  assert(state.areaEventCount == 1U);

  assert(observeExplorationCell(state, 0x33333333U) ==
         ExplorationResult::NewSector);
  assert(state.lastCellKey == 0x33333333U);
  assert(state.lastEventCellKey == eventCell);
  assert(state.lastEventCellKey != state.lastCellKey);
  assert(isValidExplorationState(state));
}

static void testOrdinaryLocalSignalAdvancesDiscoveryCounter() {
  WorldState world{};
  initializeWorld(world, 1U, 1ULL);
  ExplorationState state{};
  initializeExploration(state);
  assert(observeExplorationCell(state, 1U) == ExplorationResult::OriginSet);
  assert(observeExplorationCell(state, 37U) == ExplorationResult::NewSector);
  assert(state.discoveries == 0U);

  // Avec cette graine/zone réelle, le tirage déterministe produit un signal.
  assert(generateLocalAreaEvent(state, world, 37U, false));
  assert(state.lastEventType == EventType::StrangeSignal);
  assert(state.lastEventCellKey == 37U);
  assert(state.discoveries == 1U);
  assert(isValidExplorationState(state));

  // L'indicateur discovery=true correspond à la découverte périodique déjà
  // comptée par observeExplorationCell : aucun double comptage.
  ExplorationState guaranteed{};
  initializeExploration(guaranteed);
  observeExplorationCell(guaranteed, 100U);
  observeExplorationCell(guaranteed, 101U);
  observeExplorationCell(guaranteed, 102U);
  assert(observeExplorationCell(guaranteed, 103U) ==
         ExplorationResult::NewDiscovery);
  assert(guaranteed.discoveries == 1U);
  WorldState guaranteedWorld{};
  initializeWorld(guaranteedWorld, 99U, 1ULL);
  assert(generateLocalAreaEvent(guaranteed, guaranteedWorld, 103U, true));
  assert(guaranteed.discoveries == 1U);
}

static void testCharacterCreationAndAttributes() {
  CharacterState character{};
  assert(createCharacter(character, 0U, "NANCY"));
  assert(isValidCharacter(character));
  assert(strcmp(character.name, "NANCY") == 0);
  assert(character.level == 1U);
  assert(character.xp == 0U);
  assert(character.skillPoints == 0U);
  assert(character.hitPoints == characterMaxHitPoints(character));
  assert(character.stress == 0U);
  assert(characterClassCount() == 6U);
  assert(character.attributes[Agility] == 8U);
  assert(characterMaxHitPoints(character) == 22U);
  assert(characterMaxStress(character) == 24U);
  assert(!createCharacter(character, 6U, "INVALID"));
  assert(!createCharacter(character, 0U, ""));
  assert(!createCharacter(character, 0U, "NAME WITH TOO LONG"));
  assert(!createCharacter(character, 0U, "BAD/NAME"));
  assert(!createCharacter(character, 0U, " LEADING"));
}

static void testCharacterExperienceAndLevels() {
  CharacterState character{};
  assert(createCharacter(character, 1U, "DUSTIN"));
  assert(awardCharacterXp(character, 99U) == 0U);
  assert(character.level == 1U && character.xp == 99U);
  assert(awardCharacterXp(character, 1U) == 1U);
  assert(character.level == 2U && character.xp == 0U);
  assert(character.skillPoints == 1U);
  assert(awardCharacterXp(character, 250U) == 1U);
  assert(character.level == 3U && character.xp == 50U);
  assert(character.skillPoints == 2U);
  for (uint8_t i = character.level; i < kCharacterMaxLevel; ++i) {
    awardCharacterXp(character, 2000U);
  }
  assert(character.level == kCharacterMaxLevel);
  const uint32_t savedXp = character.xp;
  awardCharacterXp(character, 500U);
  assert(character.xp == savedXp);
  assert(isValidCharacter(character));
}

static void testCharacterMissionRewardIsIdempotent() {
  CharacterState character{};
  assert(createCharacter(character, 1U, "DUSTIN"));
  assert(awardCharacterMissionXpOnce(character, 0U, 100U));
  assert(character.level == 2U && character.xp == 0U);
  assert((character.missionRewardMask & 0x01U) != 0U);
  assert(awardCharacterMissionXpOnce(character, 0U, 100U));
  assert(character.level == 2U && character.xp == 0U);
  assert(awardCharacterMissionXpOnce(character, 1U, 75U));
  assert(character.level == 2U && character.xp == 75U);
  assert(!awardCharacterMissionXpOnce(character, 3U, 50U));
  assert(isValidCharacter(character));
}

static void testCharacterSaveCodec() {
  CharacterState original{};
  assert(createCharacter(original, 4U, "MAX MAYFIELD"));
  awardCharacterXp(original, 135U);
  assert(awardCharacterMissionXpOnce(original, 1U, 20U));
  uint8_t encoded[kCharacterSaveEncodedSize];
  assert(encodeCharacterSave(original, 23U, encoded, sizeof(encoded)));
  CharacterState restored{};
  uint32_t sequence = 0U;
  assert(decodeCharacterSave(encoded, sizeof(encoded), restored, sequence));
  assert(sequence == 23U);
  assert(strcmp(restored.name, original.name) == 0);
  assert(restored.classId == original.classId);
  assert(restored.level == original.level);
  assert(restored.xp == original.xp);
  assert(restored.skillPoints == original.skillPoints);
  assert(memcmp(restored.attributes, original.attributes,
                sizeof(original.attributes)) == 0);
  encoded[24] ^= 0x10U;
  assert(!decodeCharacterSave(encoded, sizeof(encoded), restored, sequence));
  assert(!decodeCharacterSave(encoded, sizeof(encoded) - 1U, restored, sequence));
}

int main() {
  testInitialization();
  testTimeAndNoRewind();
  testDeterministicEvolution();
  testTenYearWorldDoesNotEnd();
  testBoundedCatchUp();
  testMetricsAndEventNames();
  testSaveCodecRoundTrip();
  testSaveCodecRejectsDamage();
  testSaveCodecRejectsInvalidState();
  testOfflineWorldAdvance();
  testExplorationCellGrid();
  testExplorationVisitsAndDiscoveries();
  testExplorationSaveCodec();
  testExplorationMemoryTracks128RecentSectors();
  testLegacyExplorationSaveMigratesWithoutLosingVisits();
  testLocalAreaEventsStayAttachedToTheirGpsSector();
  testOrdinaryLocalSignalAdvancesDiscoveryCounter();
  testCharacterCreationAndAttributes();
  testCharacterExperienceAndLevels();
  testCharacterMissionRewardIsIdempotent();
  testCharacterSaveCodec();
  puts("HAWKINS: tous les tests monde, exploration et personnage ont reussi.");
  return 0;
}
