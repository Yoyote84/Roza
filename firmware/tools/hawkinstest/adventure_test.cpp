#include "hawkins_adventure.h"
#include <assert.h>
#include <stdio.h>

using namespace hawkins;

static void testStartingInventoryAndItems() {
  CharacterState player{};
  AdventureState game{};
  assert(createCharacter(player, 3U, "ELEVEN"));
  initializeAdventure(game);
  assert(isValidAdventureState(game));
  assert(game.items[static_cast<uint8_t>(AdventureItem::Medkit)] == 1U);
  assert(game.items[static_cast<uint8_t>(AdventureItem::CalmingTea)] == 1U);
  player.hitPoints = 5U;
  assert(adventureUseItem(game, player, AdventureItem::Medkit));
  assert(player.hitPoints == 13U);
  assert(game.items[static_cast<uint8_t>(AdventureItem::Medkit)] == 0U);
  player.stress = 15U;
  assert(adventureUseItem(game, player, AdventureItem::CalmingTea));
  assert(player.stress == 7U);
  assert(!adventureUseItem(game, player, AdventureItem::CalmingTea));
  assert(!adventureUseItem(game, player, AdventureItem::Medkit));
}

static void testSkills() {
  CharacterState player{};
  AdventureState game{};
  assert(createCharacter(player, 4U, "JOYCE"));
  initializeAdventure(game);
  assert(awardCharacterXp(player, 300U) == 2U);
  assert(player.skillPoints == 2U);
  assert(adventureUnlockSkill(game, player, AdventureSkill::FirstAid));
  assert(player.skillPoints == 1U);
  assert(adventureHasSkill(game, AdventureSkill::FirstAid));
  assert(!adventureUnlockSkill(game, player, AdventureSkill::FirstAid));
  assert(!adventureUnlockSkill(game, player, AdventureSkill::Mechanic));
  assert(player.skillPoints == 1U);
}

static void testDialogueChoicesAreOneShot() {
  CharacterState player{};
  AdventureState game{};
  assert(createCharacter(player, 2U, "NANCY"));
  initializeAdventure(game);
  const uint32_t oldXp = player.xp;
  assert(adventureResolveFirstContact(game, player, FirstContactChoice::Investigate));
  assert(player.xp >= oldXp);
  assert(game.items[static_cast<uint8_t>(AdventureItem::Evidence)] == 1U);
  assert(game.firstContactChoice == 2U);
  assert((game.storyFlags & 0x01U) != 0U);
  assert(!adventureResolveFirstContact(game, player, FirstContactChoice::Help));

  uint8_t bytes[kAdventureSaveEncodedSize];
  assert(encodeAdventureSave(game, 12U, bytes, sizeof(bytes)));
  AdventureState restored{};
  uint32_t sequence = 0U;
  assert(decodeAdventureSave(bytes, sizeof(bytes), restored, sequence));
  assert(sequence == 12U);
  assert(restored.firstContactChoice == 2U);
  assert((restored.storyFlags & 0x01U) != 0U);
}

static void testAllThreeDialogueBranchesPersist() {
  const FirstContactChoice choices[3] = {
      FirstContactChoice::Help,
      FirstContactChoice::Investigate,
      FirstContactChoice::Retreat};
  const char* names[3] = {"MIKE", "DUSTIN", "LUCAS"};

  for (uint8_t i = 0U; i < 3U; ++i) {
    CharacterState player{};
    AdventureState game{};
    assert(createCharacter(player, i, names[i]));
    initializeAdventure(game);
    assert(adventureResolveFirstContact(game, player, choices[i]));
    assert(game.firstContactChoice == static_cast<uint8_t>(i + 1U));

    uint8_t bytes[kAdventureSaveEncodedSize];
    assert(encodeAdventureSave(game, static_cast<uint32_t>(i + 1U),
                               bytes, sizeof(bytes)));
    AdventureState restored{};
    uint32_t sequence = 0U;
    assert(decodeAdventureSave(bytes, sizeof(bytes), restored, sequence));
    assert(restored.firstContactChoice == static_cast<uint8_t>(i + 1U));
    assert(isValidAdventureState(restored));
  }
}

static void testLegacyResolvedEncounterRemainsReadable() {
  AdventureState legacy{};
  initializeAdventure(legacy);
  // En v1, le premier contact pouvait etre resolu sans conserver le choix.
  legacy.storyFlags = 0x01U;
  assert(isValidAdventureState(legacy));

  uint8_t bytes[kAdventureSaveEncodedSize];
  assert(encodeAdventureSave(legacy, 5U, bytes, sizeof(bytes)));
  AdventureState restored{};
  uint32_t sequence = 0U;
  assert(decodeAdventureSave(bytes, sizeof(bytes), restored, sequence));
  assert(restored.storyFlags == 0x01U);
  assert(restored.firstContactChoice == 0U);
}

static void testCodecAndValidation() {
  AdventureState original{};
  initializeAdventure(original);
  adventureAddItem(original, AdventureItem::Evidence, 3U);
  original.unlockedSkills = 0x02U;
  uint8_t bytes[kAdventureSaveEncodedSize];
  assert(encodeAdventureSave(original, 9U, bytes, sizeof(bytes)));
  AdventureState restored{};
  uint32_t sequence = 0U;
  assert(decodeAdventureSave(bytes, sizeof(bytes), restored, sequence));
  assert(sequence == 9U);
  assert(restored.items[3] == 3U && restored.unlockedSkills == 0x02U);
  bytes[20] ^= 0x10U;
  assert(!decodeAdventureSave(bytes, sizeof(bytes), restored, sequence));
  assert(!decodeAdventureSave(bytes, sizeof(bytes) - 1U, restored, sequence));
  initializeAdventure(original);
  original.items[0] = 100U;
  assert(!isValidAdventureState(original));
}

int main() {
  testStartingInventoryAndItems();
  testSkills();
  testDialogueChoicesAreOneShot();
  testAllThreeDialogueBranchesPersist();
  testLegacyResolvedEncounterRemainsReadable();
  testCodecAndValidation();
  puts("HAWKINS ADVENTURE: inventaire, competences, choix et CRC OK.");
  return 0;
}
