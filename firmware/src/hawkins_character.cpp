#include "hawkins_character.h"

#include <string.h>

namespace hawkins {
namespace {

const CharacterClass kClasses[kCharacterClassCount] = {
    {"EXPLORATEUR", "Mobilite et terrain", "Bonus : Agilite / Perception",
     {4, 5, 8, 4, 7, 3}},
    {"TECHNICIEN", "Machines et bricolage", "Bonus : Intelligence",
     {3, 5, 4, 8, 6, 5}},
    {"ENQUETEUR", "Indices et observation", "Bonus : Perception",
     {3, 4, 5, 7, 8, 4}},
    {"PROTECTEUR", "Force et resistance", "Bonus : Force / Endurance",
     {8, 8, 4, 3, 5, 4}},
    {"MEDECIN", "Soins et sang-froid", "Bonus : Endurance / Intelligence",
     {3, 7, 4, 7, 6, 5}},
    {"SENSITIF", "Resiste aux anomalies", "Bonus : Perception / Charisme",
     {3, 4, 5, 7, 7, 6}},
};

constexpr uint32_t kSaveMagic = 0x48435748U;  // "HWCH" en little-endian
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

bool isAllowedNameCharacter(char c) {
  return (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         c == ' ' || c == '-' || c == '_';
}

void encodePayload(const CharacterState& character, uint8_t* output) {
  write32(output + 0U, character.schemaVersion);
  write32(output + 4U, character.xp);
  output[8] = character.classId;
  output[9] = character.level;
  output[10] = character.skillPoints;
  output[11] = character.hitPoints;
  output[12] = character.stress;
  output[13] = character.nameLength;
  for (size_t i = 0; i < CharacterAttributeCount; ++i) {
    output[14U + i] = character.attributes[i];
  }
  for (size_t i = 0; i < kCharacterNameCapacity; ++i) {
    output[20U + i] = static_cast<uint8_t>(character.name[i]);
  }
  output[32] = 0U;
  output[33] = 0U;
  output[34] = 0U;
  output[35] = 0U;
}

bool decodePayload(const uint8_t* input, CharacterState& character) {
  character = CharacterState{};
  character.schemaVersion = read32(input + 0U);
  character.xp = read32(input + 4U);
  character.classId = input[8];
  character.level = input[9];
  character.skillPoints = input[10];
  character.hitPoints = input[11];
  character.stress = input[12];
  character.nameLength = input[13];
  for (size_t i = 0; i < CharacterAttributeCount; ++i) {
    character.attributes[i] = input[14U + i];
  }
  for (size_t i = 0; i < kCharacterNameCapacity; ++i) {
    character.name[i] = static_cast<char>(input[20U + i]);
  }
  character.name[kCharacterNameCapacity] = '\0';
  return input[32] == 0U && input[33] == 0U &&
         input[34] == 0U && input[35] == 0U &&
         isValidCharacter(character);
}

}  // namespace

const CharacterClass& characterClass(uint8_t classId) {
  return kClasses[classId < kCharacterClassCount ? classId : 0U];
}

const char* characterAttributeName(CharacterAttribute attribute) {
  static const char* const names[CharacterAttributeCount] = {
      "FOR", "END", "AGI", "INT", "PER", "CHA"};
  const uint8_t index = static_cast<uint8_t>(attribute);
  return index < CharacterAttributeCount ? names[index] : "???";
}

uint8_t characterClassCount() {
  return kCharacterClassCount;
}

uint8_t characterMaxHitPoints(const CharacterState& character) {
  const uint16_t value = 12U +
      static_cast<uint16_t>(character.attributes[Endurance]) * 2U;
  return static_cast<uint8_t>(value > 255U ? 255U : value);
}

uint8_t characterMaxStress(const CharacterState& character) {
  const uint16_t value = 10U +
      static_cast<uint16_t>(character.attributes[Perception]) * 2U;
  return static_cast<uint8_t>(value > 255U ? 255U : value);
}

uint16_t characterXpForNextLevel(const CharacterState& character) {
  if (character.level >= kCharacterMaxLevel) return 0U;
  return static_cast<uint16_t>(character.level) * 100U;
}

bool createCharacter(CharacterState& character, uint8_t classId,
                     const char* name) {
  if (!name || classId >= kCharacterClassCount) return false;
  size_t length = 0U;
  while (length <= kCharacterNameCapacity && name[length] != '\0') ++length;
  if (length == 0U || length > kCharacterNameCapacity) return false;
  for (size_t i = 0; i < length; ++i) {
    if (!isAllowedNameCharacter(name[i])) return false;
  }
  if (name[0] == ' ' || name[length - 1U] == ' ') return false;

  CharacterState created{};
  created.schemaVersion = kCharacterSchemaVersion;
  created.classId = classId;
  created.level = 1U;
  created.nameLength = static_cast<uint8_t>(length);
  memcpy(created.name, name, length);
  created.name[length] = '\0';
  for (size_t i = 0; i < CharacterAttributeCount; ++i) {
    created.attributes[i] = kClasses[classId].attributes[i];
  }
  created.hitPoints = characterMaxHitPoints(created);
  created.stress = 0U;
  if (!isValidCharacter(created)) return false;
  character = created;
  return true;
}

bool isValidCharacter(const CharacterState& character) {
  if (character.schemaVersion != kCharacterSchemaVersion ||
      character.classId >= kCharacterClassCount ||
      character.level == 0U || character.level > kCharacterMaxLevel ||
      character.skillPoints > character.level - 1U ||
      character.nameLength == 0U ||
      character.nameLength > kCharacterNameCapacity) {
    return false;
  }
  for (size_t i = 0; i < CharacterAttributeCount; ++i) {
    if (character.attributes[i] == 0U || character.attributes[i] > 10U) {
      return false;
    }
  }
  if (character.hitPoints > characterMaxHitPoints(character) ||
      character.stress > characterMaxStress(character)) {
    return false;
  }
  if (character.name[character.nameLength] != '\0') return false;
  for (size_t i = 0; i < character.nameLength; ++i) {
    if (!isAllowedNameCharacter(character.name[i])) return false;
  }
  for (size_t i = character.nameLength + 1U;
       i <= kCharacterNameCapacity; ++i) {
    if (character.name[i] != '\0') return false;
  }
  if (character.level < kCharacterMaxLevel &&
      character.xp >= characterXpForNextLevel(character)) {
    return false;
  }
  return true;
}

uint8_t awardCharacterXp(CharacterState& character, uint16_t amount) {
  if (!isValidCharacter(character) || character.level >= kCharacterMaxLevel ||
      amount == 0U) {
    return 0U;
  }
  uint8_t gained = 0U;
  uint32_t remaining = amount;
  while (remaining > 0U && character.level < kCharacterMaxLevel) {
    const uint32_t needed = characterXpForNextLevel(character);
    const uint32_t room = needed - character.xp;
    if (remaining < room) {
      character.xp += remaining;
      remaining = 0U;
    } else {
      remaining -= room;
      character.xp = 0U;
      ++character.level;
      if (character.skillPoints < 255U) ++character.skillPoints;
      if (gained < 255U) ++gained;
    }
  }
  return gained;
}

bool encodeCharacterSave(const CharacterState& character, uint32_t sequence,
                         uint8_t* output, size_t capacity) {
  if (!output || capacity < kCharacterSaveEncodedSize ||
      sequence == 0U || !isValidCharacter(character)) {
    return false;
  }
  for (size_t i = 0; i < kCharacterSaveEncodedSize; ++i) output[i] = 0U;
  write32(output + 0U, kSaveMagic);
  write16(output + 4U, kSaveFormatVersion);
  write16(output + 6U, static_cast<uint16_t>(kCharacterSavePayloadSize));
  write32(output + 8U, sequence);
  encodePayload(character, output + kCharacterSaveHeaderSize);
  write32(output + kCrcOffset, crc32(output, kCharacterSaveEncodedSize));
  return true;
}

bool decodeCharacterSave(const uint8_t* input, size_t length,
                         CharacterState& character, uint32_t& sequence) {
  if (!input || length != kCharacterSaveEncodedSize ||
      read32(input + 0U) != kSaveMagic ||
      read16(input + 4U) != kSaveFormatVersion ||
      read16(input + 6U) != kCharacterSavePayloadSize) {
    return false;
  }
  uint8_t check[kCharacterSaveEncodedSize];
  memcpy(check, input, sizeof(check));
  const uint32_t storedCrc = read32(check + kCrcOffset);
  write32(check + kCrcOffset, 0U);
  if (crc32(check, sizeof(check)) != storedCrc) return false;

  const uint32_t candidateSequence = read32(input + 8U);
  if (candidateSequence == 0U) return false;
  CharacterState decoded{};
  if (!decodePayload(input + kCharacterSaveHeaderSize, decoded)) return false;
  character = decoded;
  sequence = candidateSequence;
  return true;
}

}  // namespace hawkins
