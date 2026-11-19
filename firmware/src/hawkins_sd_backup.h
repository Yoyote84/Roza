#pragma once
#include <stdint.h>

namespace hawkins {

void sdBackupInit();
bool sdBackupEnabled();
bool sdBackupSetEnabled(bool enabled);
bool sdBackupSaveNow();
bool sdBackupRestore();
// Ne fait rien lorsque la copie automatique est désactivée.
bool sdBackupAutoSave();
uint32_t sdBackupSequence();
const char* sdBackupStatusText();

}  // namespace hawkins
