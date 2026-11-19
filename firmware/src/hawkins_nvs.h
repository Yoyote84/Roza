#pragma once
#include "hawkins_world.h"
#include <stdint.h>
namespace hawkins {
enum class WorldSaveLoadStatus:uint8_t{Loaded,Empty,Corrupt,StorageError};
WorldSaveLoadStatus worldSaveLoad(WorldState&,uint32_t* sequence=nullptr);
bool worldSaveCommit(const WorldState&,uint32_t* sequence=nullptr);
}
