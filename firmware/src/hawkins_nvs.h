#pragma once

#include "hawkins_world.h"
#include "hawkins_exploration.h"
#include "hawkins_character.h"
#include <stdint.h>

namespace hawkins {

enum class WorldSaveLoadStatus : uint8_t { Loaded, Empty, Corrupt, StorageError };
enum class ExplorationSaveLoadStatus : uint8_t { Loaded, Empty, Corrupt, StorageError };
enum class CharacterSaveLoadStatus : uint8_t { Loaded, Empty, Corrupt, StorageError };

WorldSaveLoadStatus worldSaveLoad(WorldState& world, uint32_t* sequence = nullptr);
bool worldSaveCommit(const WorldState& world, uint32_t* sequence = nullptr);

ExplorationSaveLoadStatus explorationSaveLoad(ExplorationState& state,
                                               uint32_t* sequence = nullptr);
bool explorationSaveCommit(const ExplorationState& state,
                           uint32_t* sequence = nullptr);

CharacterSaveLoadStatus characterSaveLoad(CharacterState& character,
                                           uint32_t* sequence = nullptr);
bool characterSaveCommit(const CharacterState& character,
                         uint32_t* sequence = nullptr);

}  // namespace hawkins
