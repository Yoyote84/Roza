#include "hawkins_narrative_memory_codec.h"
#include <string.h>

namespace hawkins {
namespace {
constexpr uint32_t kMagic = 0x4D4E5748U;  // HWNM
constexpr uint16_t kVersion = 1U;
constexpr size_t kCrcOffset = 12U;

void write16(uint8_t* output, uint16_t value) {
  output[0] = static_cast<uint8_t>(value);
  output[1] = static_cast<uint8_t>(value >> 8U);
}
void write32(uint8_t* output, uint32_t value) {
  for (unsigned i = 0U; i < 4U; ++i) {
    output[i] = static_cast<uint8_t>(value >> (i * 8U));
  }
}
uint16_t read16(const uint8_t* input) {
  return static_cast<uint16_t>(input[0]) |
         static_cast<uint16_t>(static_cast<uint16_t>(input[1]) << 8U);
}
uint32_t read32(const uint8_t* input) {
  uint32_t value = 0U;
  for (unsigned i = 0U; i < 4U; ++i) {
    value |= static_cast<uint32_t>(input[i]) << (i * 8U);
  }
  return value;
}
uint32_t crc32(const uint8_t* data, size_t length) {
  uint32_t value = 0xFFFFFFFFU;
  for (size_t i = 0U; i < length; ++i) {
    value ^= data[i];
    for (unsigned bit = 0U; bit < 8U; ++bit) {
      const uint32_t mask = 0U - (value & 1U);
      value = (value >> 1U) ^ (0xEDB88320U & mask);
    }
  }
  return ~value;
}
void encodePayload(const NarrativeMemory& memory, uint8_t* output) {
  write32(output, memory.schemaVersion);
  write32(output + 4U, memory.nextSequence);
  write32(output + 8U, memory.totalRecords);
  output[12] = memory.recordCount;
  output[13] = memory.replaceIndex;
  output[14] = memory.plotFlags;
  output[15] = memory.npcCount;
  for (uint8_t i = 0U; i < kNarrativeNpcCount; ++i) {
    output[16U + i] = memory.relationships[i];
  }
  for (uint8_t i = 0U; i < kNarrativeMemoryRecordCount; ++i) {
    const NarrativeRecord& record = memory.records[i];
    const size_t offset = 20U + 12U * i;
    write32(output + offset, record.sequence);
    write32(output + offset + 4U, record.cellKey);
    write16(output + offset + 8U, record.detail);
    output[offset + 10U] = record.type;
    output[offset + 11U] = record.intensity;
  }
}
bool decodePayload(const uint8_t* input, NarrativeMemory& memory) {
  memory = NarrativeMemory{};
  memory.schemaVersion = read32(input);
  memory.nextSequence = read32(input + 4U);
  memory.totalRecords = read32(input + 8U);
  memory.recordCount = input[12];
  memory.replaceIndex = input[13];
  memory.plotFlags = input[14];
  memory.npcCount = input[15];
  for (uint8_t i = 0U; i < kNarrativeNpcCount; ++i) {
    memory.relationships[i] = input[16U + i];
  }
  for (uint8_t i = 0U; i < kNarrativeMemoryRecordCount; ++i) {
    const size_t offset = 20U + 12U * i;
    NarrativeRecord& record = memory.records[i];
    record.sequence = read32(input + offset);
    record.cellKey = read32(input + offset + 4U);
    record.detail = read16(input + offset + 8U);
    record.type = input[offset + 10U];
    record.intensity = input[offset + 11U];
  }
  return isValidNarrativeMemory(memory);
}
}  // namespace

bool encodeNarrativeMemorySave(const NarrativeMemory& memory,
                               uint32_t sequence, uint8_t* output,
                               size_t capacity) {
  if (!output || capacity < kNarrativeSaveEncodedSize || sequence == 0U ||
      !isValidNarrativeMemory(memory)) {
    return false;
  }
  memset(output, 0, kNarrativeSaveEncodedSize);
  write32(output, kMagic);
  write16(output + 4U, kVersion);
  write16(output + 6U, static_cast<uint16_t>(kNarrativeSavePayloadSize));
  write32(output + 8U, sequence);
  encodePayload(memory, output + kNarrativeSaveHeaderSize);
  write32(output + kCrcOffset, crc32(output, kNarrativeSaveEncodedSize));
  return true;
}

bool decodeNarrativeMemorySave(const uint8_t* input, size_t length,
                               NarrativeMemory& memory, uint32_t& sequence) {
  if (!input || length != kNarrativeSaveEncodedSize ||
      read32(input) != kMagic || read16(input + 4U) != kVersion ||
      read16(input + 6U) != kNarrativeSavePayloadSize) {
    return false;
  }
  uint8_t check[kNarrativeSaveEncodedSize];
  memcpy(check, input, sizeof(check));
  const uint32_t storedCrc = read32(check + kCrcOffset);
  write32(check + kCrcOffset, 0U);
  if (crc32(check, sizeof(check)) != storedCrc) return false;

  const uint32_t candidateSequence = read32(input + 8U);
  if (candidateSequence == 0U) return false;
  NarrativeMemory decoded{};
  if (!decodePayload(input + kNarrativeSaveHeaderSize, decoded)) return false;
  memory = decoded;
  sequence = candidateSequence;
  return true;
}
}  // namespace hawkins
