#pragma once
#include "hawkins_world.h"
#include <stdint.h>
namespace hawkins {
enum class RuntimeStatus:uint8_t{Empty,Ready,WaitingForClock,ClockBeforeWorld,SaveError,CorruptSave,StorageError};
void hawkinsRuntimeInit();
bool hawkinsRuntimeUpdate();
bool hawkinsRuntimeSaveNow();
// Progression narrative locale : une journée de jeu par secteur inédit.
bool hawkinsRuntimeAdvanceDays(uint32_t days);
const WorldState* hawkinsRuntimeWorld();
RuntimeStatus hawkinsRuntimeStatus();
const char* hawkinsRuntimeStatusText();
}
