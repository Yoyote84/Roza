// Taux indicatif USDT -> CNY.
// Source principale : OKX C2C (prix acheteur direct).
// Repli sans clé : CoinPaprika USDT/USD × Frankfurter USD/CNY (estimation croisée).
// Ce repli n'est pas une cotation OTC et ne garantit pas un prix exécutable.
#pragma once
#include "globals.h"

void okxEnter();
void okxUpdate();
void okxKey(char k);
void drawOkx();

// URLs utilisées par les diagnostics série OKXDUMP.
void okxBuildUrl(char* out, size_t n);
void okxBuildFallbackUrl(char* out, size_t n);

