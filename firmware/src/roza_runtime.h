#pragma once
#include <Arduino.h>
bool rozaRuntimeStart(const char* appId, const char* entryName);
void rozaRuntimeStop();
bool rozaRuntimeActive();
bool rozaRuntimeVisible();
void rozaRuntimeKey(char key);
void rozaRuntimeUpdate();
void rozaRuntimeDraw();
const char* rozaRuntimeLastError();
