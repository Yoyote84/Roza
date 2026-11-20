#pragma once
#include <stddef.h>
#include "globals.h"
void hawkinsAppEnter();
void hawkinsAppUpdate();
void hawkinsAppAiUpdate();
bool hawkinsAppHandleSerialLine(const char* line, size_t length);
void hawkinsAppKey(char key);
void drawHawkinsApp();
