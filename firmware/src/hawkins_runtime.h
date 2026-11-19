#pragma once
#include "hawkins_world.h"
#include <stdint.h>
namespace hawkins {
enum class RuntimeStatus:uint8_t{Empty,Ready,WaitingForClock,ClockBeforeWorld,SaveError,CorruptSave,StorageError};
void hawkinsRuntimeInit();
bool hawkinsRuntimeUpdate();
bool hawkinsRuntimeSaveNow();
const WorldState* hawkinsRuntimeWorld();
RuntimeStatus hawkinsRuntimeStatus();
const char* hawkinsRuntimeStatusText();
}
