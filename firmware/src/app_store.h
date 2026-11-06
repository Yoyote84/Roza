#pragma once

#include "globals.h"

// Gestionnaire de paquets ROZA stockés sur la microSD.
// Cette première étape inspecte uniquement les manifestes : aucun code tiers n'est exécuté.
void appStoreEnter();
void appStoreExit();
void appStoreKey(char key);
void drawAppStore();
