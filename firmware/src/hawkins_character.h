#pragma once

#include <stddef.h>
#include <stdint.h>

namespace hawkins {

constexpr uint32_t kCharacterSchemaVersion = 1U;
constexpr uint8_t kCharacterClassCount = 6U;
constexpr uint8_t kCharacterNameCapacity = 12U;
constexpr uint8_t kCharacterMaxLevel = 20U;
constexpr size_t kCharacterSaveHeaderSize = 16U;
constexpr size_t kCharacterSavePayloadSize = 36U;
constexpr size_t kCharacterSaveEncodedSize =
    kCharacterSaveHeaderSize + kCharacterSavePayloadSize;

enum CharacterAttribute : uint8_t {
  Strength = 0,
  Endurance,
  Agility,
  Intelligence,
  Perception,
  Charisma,
  CharacterAttributeCount
};

struct CharacterClass {
  const char* name;
  const char* role;
  const char* specialty;
  uint8_t attributes[CharacterAttributeCount];
};

struct CharacterState {
  uint32_t schemaVersion;
  uint32_t xp;
  uint8_t classId;
  uint8_t level;
  uint8_t skillPoints;
  uint8_t hitPoints;
  uint8_t stress;
  uint8_t nameLength;
  uint8_t attributes[CharacterAttributeCount];
  char name[kCharacterNameCapacity + 1U];
};

const CharacterClass& characterClass(uint8_t classId);
const char* characterAttributeName(CharacterAttribute attribute);
uint8_t characterClassCount();
uint8_t characterMaxHitPoints(const CharacterState& character);
uint8_t characterMaxStress(const CharacterState& character);
uint16_t characterXpForNextLevel(const CharacterState& character);

bool createCharacter(CharacterState& character, uint8_t classId,
                     const char* name);
bool isValidCharacter(const CharacterState& character);

// Ajoute des XP et applique automatiquement les niveaux gagnés.
// Retourne le nombre de niveaux obtenus.
uint8_t awardCharacterXp(CharacterState& character, uint16_t amount);

bool encodeCharacterSave(const CharacterState& character, uint32_t sequence,
                         uint8_t* output, size_t capacity);
bool decodeCharacterSave(const uint8_t* input, size_t length,
                         CharacterState& character, uint32_t& sequence);

}  // namespace hawkins
