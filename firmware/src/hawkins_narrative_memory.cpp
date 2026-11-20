#include "hawkins_narrative_memory.h"

#include <stdio.h>
#include <string.h>

namespace hawkins {

void initializeNarrativeMemory(NarrativeMemory& memory) {
  memory = NarrativeMemory{};
  memory.schemaVersion = kNarrativeMemorySchemaVersion;
  memory.nextSequence = 1U;
  for (uint8_t i = 0U; i < kNarrativeNpcCount; ++i) {
    memory.relationships[i] = 50U;
  }
}

bool isValidNarrativeMemory(const NarrativeMemory& memory) {
  if (memory.schemaVersion != kNarrativeMemorySchemaVersion ||
      memory.nextSequence == 0U ||
      memory.recordCount > kNarrativeMemoryRecordCount ||
      memory.replaceIndex >= kNarrativeMemoryRecordCount ||
      memory.npcCount > kNarrativeNpcCount ||
      memory.totalRecords < memory.recordCount ||
      memory.nextSequence != memory.totalRecords + 1U ||
      (memory.recordCount < kNarrativeMemoryRecordCount &&
       memory.replaceIndex != 0U)) {
    return false;
  }
  for (uint8_t i = 0U; i < kNarrativeNpcCount; ++i) {
    if (memory.relationships[i] > kNarrativeMaxRelationship) return false;
  }
  for (uint8_t i = 0U; i < memory.recordCount; ++i) {
    const NarrativeRecord& record = memory.records[i];
    if (record.sequence == 0U || record.sequence >= memory.nextSequence ||
        record.sequence > memory.totalRecords ||
        record.type < static_cast<uint8_t>(NarrativeRecordType::FirstContact) ||
        record.type > static_cast<uint8_t>(NarrativeRecordType::NpcContact) ||
        record.intensity > 100U) {
      return false;
    }
    for (uint8_t j = 0U; j < i; ++j) {
      if (memory.records[j].sequence == record.sequence) return false;
    }
  }
  return true;
}

bool narrativeRecord(NarrativeMemory& memory, NarrativeRecordType type,
                     uint32_t cellKey, uint16_t detail, uint8_t intensity) {
  const uint8_t rawType = static_cast<uint8_t>(type);
  if (!isValidNarrativeMemory(memory) || rawType < 1U || rawType > 8U ||
      intensity > 100U || memory.nextSequence == UINT32_MAX ||
      memory.totalRecords == UINT32_MAX) {
    return false;
  }

  const NarrativeRecord record{memory.nextSequence++, cellKey, detail,
                               rawType, intensity};
  if (memory.recordCount < kNarrativeMemoryRecordCount) {
    memory.records[memory.recordCount++] = record;
  } else {
    memory.records[memory.replaceIndex] = record;
    memory.replaceIndex = static_cast<uint8_t>(
        (memory.replaceIndex + 1U) % kNarrativeMemoryRecordCount);
  }
  ++memory.totalRecords;
  return isValidNarrativeMemory(memory);
}

bool narrativeSetPlot(NarrativeMemory& memory, uint8_t plotId, bool open) {
  if (!isValidNarrativeMemory(memory) || plotId >= kNarrativePlotCount) {
    return false;
  }
  const uint8_t bit = static_cast<uint8_t>(1U << plotId);
  if (open) memory.plotFlags = static_cast<uint8_t>(memory.plotFlags | bit);
  else memory.plotFlags = static_cast<uint8_t>(memory.plotFlags & ~bit);
  return true;
}

bool narrativeAdjustNpcRelationship(NarrativeMemory& memory, uint8_t npcId,
                                     int8_t delta) {
  if (!isValidNarrativeMemory(memory) || npcId >= kNarrativeNpcCount) {
    return false;
  }
  int value = static_cast<int>(memory.relationships[npcId]) +
              static_cast<int>(delta);
  if (value < kNarrativeMinRelationship) value = kNarrativeMinRelationship;
  if (value > kNarrativeMaxRelationship) value = kNarrativeMaxRelationship;
  memory.relationships[npcId] = static_cast<uint8_t>(value);
  if (memory.npcCount <= npcId) {
    memory.npcCount = static_cast<uint8_t>(npcId + 1U);
  }
  return true;
}

const NarrativeRecord* narrativeLatest(const NarrativeMemory& memory) {
  if (!isValidNarrativeMemory(memory) || memory.recordCount == 0U) return nullptr;
  const NarrativeRecord* latest = nullptr;
  for (uint8_t i = 0U; i < memory.recordCount; ++i) {
    if (!latest || memory.records[i].sequence > latest->sequence) {
      latest = &memory.records[i];
    }
  }
  return latest;
}

const NarrativeRecord* narrativeRecordAt(const NarrativeMemory& memory,
                                         uint8_t chronologicalIndex) {
  if (!isValidNarrativeMemory(memory) ||
      chronologicalIndex >= memory.recordCount) {
    return nullptr;
  }
  uint32_t previous = 0U;
  for (uint8_t rank = 0U; rank <= chronologicalIndex; ++rank) {
    const NarrativeRecord* selected = nullptr;
    for (uint8_t i = 0U; i < memory.recordCount; ++i) {
      const NarrativeRecord& candidate = memory.records[i];
      if (candidate.sequence > previous &&
          (!selected || candidate.sequence < selected->sequence)) {
        selected = &candidate;
      }
    }
    if (!selected) return nullptr;
    previous = selected->sequence;
    if (rank == chronologicalIndex) return selected;
  }
  return nullptr;
}

bool formatNarrativeMemorySummary(const NarrativeMemory& memory,
                                  char* output, size_t capacity) {
  if (!output || capacity == 0U || !isValidNarrativeMemory(memory)) return false;
  const int written = snprintf(
      output, capacity, "RECIT:%lu EVT:%u/%u FLAGS:%02X NPC:%u LIA:%u",
      static_cast<unsigned long>(memory.totalRecords),
      static_cast<unsigned>(memory.recordCount),
      static_cast<unsigned>(kNarrativeMemoryRecordCount),
      static_cast<unsigned>(memory.plotFlags),
      static_cast<unsigned>(memory.npcCount),
      static_cast<unsigned>(memory.relationships[0]));
  if (written < 0 || static_cast<size_t>(written) >= capacity) {
    output[capacity - 1U] = '\0';
    return false;
  }

  const uint8_t contextCount =
      memory.recordCount < 2U ? memory.recordCount : 2U;
  for (uint8_t i = 0U; i < contextCount; ++i) {
    const NarrativeRecord* record =
        narrativeRecordAt(memory, static_cast<uint8_t>(
            memory.recordCount - contextCount + i));
    if (!record) return false;
    const size_t used = strlen(output);
    const size_t remaining = capacity - used;
    const int appended = snprintf(
        output + used, remaining, " |%u:%u",
        static_cast<unsigned>(record->type),
        static_cast<unsigned>(record->detail));
    if (appended < 0 || static_cast<size_t>(appended) >= remaining) {
      output[capacity - 1U] = '\0';
      return false;
    }
  }
  return true;
}

}  // namespace hawkins
