#pragma once
#include "hawkins_narrative_memory.h"
#include <stddef.h>
namespace hawkins {
constexpr size_t kNarrativeSaveHeaderSize = 16U;
constexpr size_t kNarrativeSavePayloadSize =
    20U + static_cast<size_t>(kNarrativeMemoryRecordCount) * 12U;
constexpr size_t kNarrativeSaveEncodedSize =
    kNarrativeSaveHeaderSize + kNarrativeSavePayloadSize;
bool encodeNarrativeMemorySave(const NarrativeMemory& memory, uint32_t sequence,
                               uint8_t* output, size_t capacity);
bool decodeNarrativeMemorySave(const uint8_t* input, size_t length,
                               NarrativeMemory& memory, uint32_t& sequence);
}
