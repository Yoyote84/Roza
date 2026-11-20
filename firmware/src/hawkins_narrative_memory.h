#pragma once
#include <stddef.h>
#include <stdint.h>

namespace hawkins {
constexpr uint32_t kNarrativeMemorySchemaVersion = 1U;
constexpr uint8_t kNarrativeMemoryRecordCount = 12U;
constexpr uint8_t kNarrativeNpcCount = 4U;
constexpr uint8_t kNarrativePlotCount = 8U;
constexpr uint8_t kNarrativeSummaryCapacity = 96U;
constexpr uint8_t kNarrativeMaxRelationship = 100U;
constexpr uint8_t kNarrativeMinRelationship = 0U;

enum class NarrativeRecordType : uint8_t {
  FirstContact = 1,
  LocalSignal,
  SectorDiscovery,
  MissionComplete,
  ItemUsed,
  SkillUnlocked,
  PlotChanged,
  NpcContact
};
struct NarrativeRecord {
  uint32_t sequence;
  uint32_t cellKey;
  uint16_t detail;
  uint8_t type;
  uint8_t intensity;
};
struct NarrativeMemory {
  uint32_t schemaVersion;
  uint32_t nextSequence;
  uint32_t totalRecords;
  uint8_t recordCount;
  uint8_t replaceIndex;
  uint8_t plotFlags;
  uint8_t npcCount;
  uint8_t relationships[kNarrativeNpcCount];
  NarrativeRecord records[kNarrativeMemoryRecordCount];
};
void initializeNarrativeMemory(NarrativeMemory& memory);
bool isValidNarrativeMemory(const NarrativeMemory& memory);
bool narrativeRecord(NarrativeMemory& memory, NarrativeRecordType type,
                     uint32_t cellKey, uint16_t detail, uint8_t intensity);
bool narrativeSetPlot(NarrativeMemory& memory, uint8_t plotId, bool open);
bool narrativeAdjustNpcRelationship(NarrativeMemory& memory, uint8_t npcId,
                                     int8_t delta);
const NarrativeRecord* narrativeLatest(const NarrativeMemory& memory);
const NarrativeRecord* narrativeRecordAt(const NarrativeMemory& memory,
                                         uint8_t chronologicalIndex);
// Produit un résumé compact et déterministe pour le futur contexte de l'IA.
bool formatNarrativeMemorySummary(const NarrativeMemory& memory,
                                  char* output, size_t capacity);
}
