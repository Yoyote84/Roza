#include "hawkins_adventure.h"

#include <string.h>

namespace hawkins {
namespace {

constexpr uint32_t kSaveMagic = 0x41574848U;  // "HHWA" little-endian
constexpr uint16_t kSaveFormatVersion = 1U;
constexpr size_t kCrcOffset = 12U;
constexpr uint8_t kFirstContactResolved = 0x01U;

const AdventureSkillDef kSkills[kAdventureSkillCount] = {
    {"PREMIERS SOINS", "Les trousses soignent davantage", Endurance, 7U},
    {"SANG-FROID", "Le the calme davantage le stress", Perception, 7U},
    {"BRICOLAGE", "Bonus narratif lors des scenes techniques", Intelligence, 8U},
    {"OBSERVATION", "Bonus narratif lors des enquetes", Perception, 8U},
};

void write16(uint8_t* output, uint16_t value) {
  output[0] = static_cast<uint8_t>(value);
  output[1] = static_cast<uint8_t>(value >> 8U);
}
void write32(uint8_t* output, uint32_t value) {
  for (unsigned i = 0; i < 4U; ++i) output[i] = static_cast<uint8_t>(value >> (8U * i));
}
uint16_t read16(const uint8_t* input) {
  return static_cast<uint16_t>(input[0]) |
         static_cast<uint16_t>(static_cast<uint16_t>(input[1]) << 8U);
}
uint32_t read32(const uint8_t* input) {
  uint32_t value = 0U;
  for (unsigned i = 0; i < 4U; ++i) value |= static_cast<uint32_t>(input[i]) << (8U * i);
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
void encodePayload(const AdventureState& state, uint8_t* output) {
  write32(output, state.schemaVersion);
  for (size_t i = 0; i < kAdventureItemCount; ++i) output[4U + i] = state.items[i];
  output[8] = state.unlockedSkills;
  output[9] = state.storyFlags;
  output[10] = state.reserved[0];
  output[11] = state.reserved[1];
}
bool decodePayload(const uint8_t* input, AdventureState& state) {
  state = AdventureState{};
  state.schemaVersion = read32(input);
  for (size_t i = 0; i < kAdventureItemCount; ++i) state.items[i] = input[4U + i];
  state.unlockedSkills = input[8];
  state.storyFlags = input[9];
  state.reserved[0] = input[10];
  state.reserved[1] = input[11];
  return isValidAdventureState(state);
}
uint8_t bounded(uint8_t value, uint8_t maximum) {
  return value > maximum ? maximum : value;
}

}  // namespace

void initializeAdventure(AdventureState& state) {
  state = AdventureState{};
  state.schemaVersion = kAdventureSchemaVersion;
  state.items[static_cast<uint8_t>(AdventureItem::Medkit)] = 1U;
  state.items[static_cast<uint8_t>(AdventureItem::CalmingTea)] = 1U;
  state.items[static_cast<uint8_t>(AdventureItem::SpareBattery)] = 1U;
}

bool isValidAdventureState(const AdventureState& state) {
  if (state.schemaVersion != kAdventureSchemaVersion ||
      (state.unlockedSkills & 0xF0U) != 0U ||
      (state.storyFlags & static_cast<uint8_t>(~kFirstContactResolved)) != 0U ||
      state.reserved[0] != 0U || state.reserved[1] != 0U) return false;
  for (size_t i = 0; i < kAdventureItemCount; ++i) {
    if (state.items[i] > 99U) return false;
  }
  return true;
}

const char* adventureItemName(AdventureItem item) {
  switch (item) {
    case AdventureItem::Medkit: return "TROUSSE DE SOINS";
    case AdventureItem::CalmingTea: return "THE CALMANT";
    case AdventureItem::SpareBattery: return "PILE AA";
    case AdventureItem::Evidence: return "PREUVE";
    default: return "OBJET INCONNU";
  }
}

const AdventureSkillDef& adventureSkillDefinition(AdventureSkill skill) {
  const uint8_t index = static_cast<uint8_t>(skill);
  return kSkills[index < kAdventureSkillCount ? index : 0U];
}

bool adventureHasSkill(const AdventureState& state, AdventureSkill skill) {
  const uint8_t index = static_cast<uint8_t>(skill);
  return isValidAdventureState(state) && index < kAdventureSkillCount &&
         (state.unlockedSkills & static_cast<uint8_t>(1U << index)) != 0U;
}

bool adventureAddItem(AdventureState& state, AdventureItem item, uint8_t amount) {
  const uint8_t index = static_cast<uint8_t>(item);
  if (!isValidAdventureState(state) || index >= kAdventureItemCount || amount == 0U) return false;
  const unsigned total = static_cast<unsigned>(state.items[index]) + amount;
  state.items[index] = static_cast<uint8_t>(total > 99U ? 99U : total);
  return true;
}

bool adventureUseItem(AdventureState& state, CharacterState& character,
                      AdventureItem item) {
  const uint8_t index = static_cast<uint8_t>(item);
  if (!isValidAdventureState(state) || !isValidCharacter(character) ||
      index >= kAdventureItemCount || state.items[index] == 0U) return false;

  if (item == AdventureItem::Medkit) {
    if (character.hitPoints >= characterMaxHitPoints(character)) return false;
    const uint8_t amount = adventureHasSkill(state, AdventureSkill::FirstAid) ? 12U : 8U;
    character.hitPoints = bounded(static_cast<uint8_t>(character.hitPoints + amount),
                                  characterMaxHitPoints(character));
  } else if (item == AdventureItem::CalmingTea) {
    if (character.stress == 0U) return false;
    const uint8_t amount = adventureHasSkill(state, AdventureSkill::CalmMind) ? 12U : 8U;
    character.stress = character.stress > amount
                           ? static_cast<uint8_t>(character.stress - amount) : 0U;
  } else if (item == AdventureItem::SpareBattery) {
    // Une pile est une ressource d'enquête, pas un soin : elle ne se consomme
    // qu'à travers une scène qui en a réellement besoin.
    return false;
  } else {
    return false;
  }
  --state.items[index];
  return isValidCharacter(character);
}

bool adventureUnlockSkill(AdventureState& state, CharacterState& character,
                          AdventureSkill skill) {
  const uint8_t index = static_cast<uint8_t>(skill);
  if (!isValidAdventureState(state) || !isValidCharacter(character) ||
      index >= kAdventureSkillCount || character.skillPoints == 0U ||
      adventureHasSkill(state, skill)) return false;
  const AdventureSkillDef& def = kSkills[index];
  if (character.attributes[def.attribute] < def.requiredValue) return false;
  --character.skillPoints;
  state.unlockedSkills = static_cast<uint8_t>(state.unlockedSkills | (1U << index));
  return isValidCharacter(character) && isValidAdventureState(state);
}

bool adventureResolveFirstContact(AdventureState& state,
                                  CharacterState& character,
                                  FirstContactChoice choice) {
  if (!isValidAdventureState(state) || !isValidCharacter(character) ||
      (state.storyFlags & kFirstContactResolved) != 0U ||
      static_cast<uint8_t>(choice) > static_cast<uint8_t>(FirstContactChoice::Retreat)) return false;

  switch (choice) {
    case FirstContactChoice::Help: {
      const bool success = character.attributes[Charisma] >= 7U ||
                           character.attributes[Endurance] >= 7U;
      awardCharacterXp(character, success ? 30U : 10U);
      if (success) adventureAddItem(state, AdventureItem::Medkit, 1U);
      else character.stress = bounded(static_cast<uint8_t>(character.stress + 3U),
                                      characterMaxStress(character));
      break;
    }
    case FirstContactChoice::Investigate: {
      const bool success = character.attributes[Perception] >= 7U ||
                           character.attributes[Intelligence] >= 8U ||
                           adventureHasSkill(state, AdventureSkill::Investigator) ||
                           adventureHasSkill(state, AdventureSkill::Mechanic);
      awardCharacterXp(character, success ? 30U : 12U);
      if (success) adventureAddItem(state, AdventureItem::Evidence, 1U);
      else {
        character.hitPoints = character.hitPoints > 2U
                                  ? static_cast<uint8_t>(character.hitPoints - 2U) : 1U;
        character.stress = bounded(static_cast<uint8_t>(character.stress + 4U),
                                   characterMaxStress(character));
      }
      break;
    }
    case FirstContactChoice::Retreat:
      awardCharacterXp(character, 5U);
      character.stress = character.stress > 3U
                             ? static_cast<uint8_t>(character.stress - 3U) : 0U;
      adventureAddItem(state, AdventureItem::SpareBattery, 1U);
      break;
  }
  state.storyFlags = static_cast<uint8_t>(state.storyFlags | kFirstContactResolved);
  return isValidCharacter(character) && isValidAdventureState(state);
}

bool encodeAdventureSave(const AdventureState& state, uint32_t sequence,
                         uint8_t* output, size_t capacity) {
  if (!output || capacity < kAdventureSaveEncodedSize || sequence == 0U ||
      !isValidAdventureState(state)) return false;
  memset(output, 0, kAdventureSaveEncodedSize);
  write32(output, kSaveMagic);
  write16(output + 4U, kSaveFormatVersion);
  write16(output + 6U, static_cast<uint16_t>(kAdventureSavePayloadSize));
  write32(output + 8U, sequence);
  encodePayload(state, output + kAdventureSaveHeaderSize);
  write32(output + kCrcOffset, crc32(output, kAdventureSaveEncodedSize));
  return true;
}

bool decodeAdventureSave(const uint8_t* input, size_t length,
                         AdventureState& state, uint32_t& sequence) {
  if (!input || length != kAdventureSaveEncodedSize ||
      read32(input) != kSaveMagic || read16(input + 4U) != kSaveFormatVersion ||
      read16(input + 6U) != kAdventureSavePayloadSize) return false;
  uint8_t check[kAdventureSaveEncodedSize];
  memcpy(check, input, sizeof(check));
  const uint32_t storedCrc = read32(check + kCrcOffset);
  write32(check + kCrcOffset, 0U);
  if (crc32(check, sizeof(check)) != storedCrc) return false;
  const uint32_t candidateSequence = read32(input + 8U);
  if (candidateSequence == 0U) return false;
  AdventureState decoded{};
  if (!decodePayload(input + kAdventureSaveHeaderSize, decoded)) return false;
  state = decoded;
  sequence = candidateSequence;
  return true;
}

}  // namespace hawkins
