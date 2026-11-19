#pragma once

#include "hawkins_character.h"
#include <stddef.h>
#include <stdint.h>

namespace hawkins {

constexpr uint32_t kAdventureSchemaVersion = 1U;
constexpr uint8_t kAdventureItemCount = 4U;
constexpr uint8_t kAdventureSkillCount = 4U;
constexpr size_t kAdventureSaveHeaderSize = 16U;
constexpr size_t kAdventureSavePayloadSize = 12U;
constexpr size_t kAdventureSaveEncodedSize =
    kAdventureSaveHeaderSize + kAdventureSavePayloadSize;

enum class AdventureItem : uint8_t {
  Medkit = 0,
  CalmingTea,
  SpareBattery,
  Evidence
};

enum class AdventureSkill : uint8_t {
  FirstAid = 0,
  CalmMind,
  Mechanic,
  Investigator
};

enum class FirstContactChoice : uint8_t {
  Help = 0,
  Investigate,
  Retreat
};

struct AdventureState {
  uint32_t schemaVersion;
  uint8_t items[kAdventureItemCount];
  uint8_t unlockedSkills;
  uint8_t storyFlags;
  uint8_t reserved[2];
};

struct AdventureSkillDef {
  const char* name;
  const char* description;
  CharacterAttribute attribute;
  uint8_t requiredValue;
};

void initializeAdventure(AdventureState& state);
bool isValidAdventureState(const AdventureState& state);
const char* adventureItemName(AdventureItem item);
const AdventureSkillDef& adventureSkillDefinition(AdventureSkill skill);
bool adventureHasSkill(const AdventureState& state, AdventureSkill skill);
bool adventureAddItem(AdventureState& state, AdventureItem item, uint8_t amount = 1U);
bool adventureUseItem(AdventureState& state, CharacterState& character,
                      AdventureItem item);
bool adventureUnlockSkill(AdventureState& state, CharacterState& character,
                          AdventureSkill skill);
bool adventureResolveFirstContact(AdventureState& state,
                                  CharacterState& character,
                                  FirstContactChoice choice);

bool encodeAdventureSave(const AdventureState& state, uint32_t sequence,
                         uint8_t* output, size_t capacity);
bool decodeAdventureSave(const uint8_t* input, size_t length,
                         AdventureState& state, uint32_t& sequence);

}  // namespace hawkins
