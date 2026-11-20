#include "hawkins_narrative_memory.h"
#include "hawkins_narrative_memory_codec.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

using namespace hawkins;

static void testInitializeAndValidate() {
  NarrativeMemory memory{};
  initializeNarrativeMemory(memory);
  assert(isValidNarrativeMemory(memory));
  assert(memory.nextSequence == 1U);
  assert(memory.recordCount == 0U);
  assert(memory.npcCount == 0U);
  assert(memory.relationships[0] == 50U);
  assert(narrativeLatest(memory) == nullptr);
  assert(narrativeRecordAt(memory, 0U) == nullptr);
}

static void testRelationshipsAndPlots() {
  NarrativeMemory memory{};
  initializeNarrativeMemory(memory);
  assert(narrativeAdjustNpcRelationship(memory, 0U, 25));
  assert(memory.relationships[0] == 75U && memory.npcCount == 1U);
  assert(narrativeAdjustNpcRelationship(memory, 0U, 100));
  assert(memory.relationships[0] == 100U);
  assert(narrativeAdjustNpcRelationship(memory, 0U, -127));
  assert(memory.relationships[0] == 0U);
  assert(!narrativeAdjustNpcRelationship(memory, kNarrativeNpcCount, 1));
  assert(narrativeSetPlot(memory, 3U, true));
  assert((memory.plotFlags & 0x08U) != 0U);
  assert(narrativeSetPlot(memory, 3U, false));
  assert((memory.plotFlags & 0x08U) == 0U);
  assert(!narrativeSetPlot(memory, kNarrativePlotCount, true));
  assert(isValidNarrativeMemory(memory));
}

static void testRecordRingAndChronology() {
  NarrativeMemory memory{};
  initializeNarrativeMemory(memory);
  for (uint16_t i = 1U; i <= 13U; ++i) {
    assert(narrativeRecord(memory, NarrativeRecordType::SectorDiscovery,
                           1000U + i, i, static_cast<uint8_t>(i)));
  }
  assert(memory.totalRecords == 13U);
  assert(memory.recordCount == kNarrativeMemoryRecordCount);
  assert(memory.replaceIndex == 1U);
  assert(memory.nextSequence == 14U);
  const NarrativeRecord* latest = narrativeLatest(memory);
  assert(latest && latest->sequence == 13U && latest->detail == 13U);
  const NarrativeRecord* oldest = narrativeRecordAt(memory, 0U);
  assert(oldest && oldest->sequence == 2U && oldest->detail == 2U);
  const NarrativeRecord* last = narrativeRecordAt(
      memory, kNarrativeMemoryRecordCount - 1U);
  assert(last && last->sequence == 13U);
  assert(isValidNarrativeMemory(memory));
}

static void testAiConversationMarkersPersist() {
  NarrativeMemory memory{};
  initializeNarrativeMemory(memory);
  assert(narrativeRecord(memory, NarrativeRecordType::AiNarration, 0U, 3U, 0U));
  assert(narrativeRecord(memory, NarrativeRecordType::AiChoice, 0U, 2U, 0U));
  const NarrativeRecord* narration = narrativeRecordAt(memory, 0U);
  const NarrativeRecord* choice = narrativeRecordAt(memory, 1U);
  assert(narration && narration->type == static_cast<uint8_t>(NarrativeRecordType::AiNarration));
  assert(narration->detail == 3U);
  assert(choice && choice->type == static_cast<uint8_t>(NarrativeRecordType::AiChoice));
  assert(choice->detail == 2U);
  assert(isValidNarrativeMemory(memory));
}

static void testInvalidStateRejected() {
  NarrativeMemory memory{};
  initializeNarrativeMemory(memory);
  memory.relationships[0] = 101U;
  assert(!isValidNarrativeMemory(memory));
  initializeNarrativeMemory(memory);
  memory.totalRecords = 1U;
  assert(!isValidNarrativeMemory(memory));
  initializeNarrativeMemory(memory);
  assert(narrativeRecord(memory, NarrativeRecordType::FirstContact,
                         0U, 2U, 100U));
  memory.records[0].intensity = 101U;
  assert(!isValidNarrativeMemory(memory));
}

static void testSummary() {
  NarrativeMemory memory{};
  initializeNarrativeMemory(memory);
  assert(narrativeSetPlot(memory, 0U, true));
  assert(narrativeAdjustNpcRelationship(memory, 0U, 10));
  assert(narrativeRecord(memory, NarrativeRecordType::FirstContact,
                         0U, 1U, 60U));
  char summary[kNarrativeSummaryCapacity] = {};
  assert(formatNarrativeMemorySummary(memory, summary, sizeof(summary)));
  assert(strstr(summary, "RECIT:1") != nullptr);
  assert(strstr(summary, "LIA:60") != nullptr);
  char shortSummary[4] = {};
  assert(!formatNarrativeMemorySummary(memory, shortSummary,
                                        sizeof(shortSummary)));
  assert(shortSummary[sizeof(shortSummary) - 1U] == '\0');
}

static void testCodecRoundTripAndCrc() {
  static_assert(kNarrativeSavePayloadSize == 164U,
                "Le payload doit occuper exactement 20 + 12*12 octets");
  NarrativeMemory original{};
  initializeNarrativeMemory(original);
  assert(narrativeSetPlot(original, 0U, true));
  assert(narrativeAdjustNpcRelationship(original, 0U, 12));
  for (uint16_t i = 0U; i < 15U; ++i) {
    const NarrativeRecordType type =
        i == 0U ? NarrativeRecordType::FirstContact :
        i == 1U ? NarrativeRecordType::AiNarration :
        i == 2U ? NarrativeRecordType::AiChoice :
                  NarrativeRecordType::SectorDiscovery;
    assert(narrativeRecord(original, type, 0x1000U + i, i,
                           static_cast<uint8_t>(i)));
  }
  uint8_t bytes[kNarrativeSaveEncodedSize] = {};
  assert(encodeNarrativeMemorySave(original, 42U, bytes, sizeof(bytes)));
  NarrativeMemory restored{};
  uint32_t sequence = 0U;
  assert(decodeNarrativeMemorySave(bytes, sizeof(bytes), restored, sequence));
  assert(sequence == 42U);
  assert(restored.totalRecords == original.totalRecords);
  assert(restored.nextSequence == original.nextSequence);
  assert(restored.recordCount == original.recordCount);
  assert(restored.replaceIndex == original.replaceIndex);
  assert(restored.plotFlags == original.plotFlags);
  assert(restored.relationships[0] == original.relationships[0]);
  for (uint8_t i = 0U; i < restored.recordCount; ++i) {
    const NarrativeRecord* a = narrativeRecordAt(original, i);
    const NarrativeRecord* b = narrativeRecordAt(restored, i);
    assert(a && b && a->sequence == b->sequence &&
           a->cellKey == b->cellKey && a->detail == b->detail &&
           a->type == b->type && a->intensity == b->intensity);
  }
  assert(!decodeNarrativeMemorySave(bytes, sizeof(bytes) - 1U,
                                    restored, sequence));
  bytes[30] ^= 0x80U;
  assert(!decodeNarrativeMemorySave(bytes, sizeof(bytes), restored, sequence));
  assert(!encodeNarrativeMemorySave(original, 0U, bytes, sizeof(bytes)));
  assert(!encodeNarrativeMemorySave(original, 1U, bytes,
                                     sizeof(bytes) - 1U));
}

int main() {
  testInitializeAndValidate();
  testRelationshipsAndPlots();
  testRecordRingAndChronology();
  testAiConversationMarkersPersist();
  testInvalidStateRejected();
  testSummary();
  testCodecRoundTripAndCrc();
  puts("HAWKINS: memoire narrative et codec CRC valides.");
  return 0;
}
