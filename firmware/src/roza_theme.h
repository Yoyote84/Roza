// Palette ROZA — instrument personnel de terrain, inspiration années 1940.
// Valeurs RGB565 pour l'écran ST7789 240x135. Aucun effet graphique ne dépend d'un accès réseau.
#pragma once
#include <stdint.h>

static constexpr uint16_t ROZA_INK         = 0x31B5; // #303629 — encre / vert très sombre
static constexpr uint16_t ROZA_CARD        = 0x4454; // #424A24 — panneau olive sombre
static constexpr uint16_t ROZA_OLIVE_DARK  = 0x5B29; // #59654A — filet / olive patiné
static constexpr uint16_t ROZA_OLIVE       = 0x5B29; // #59654A — olive moyen
static constexpr uint16_t ROZA_OLIVE_LIGHT = 0x6388; // olive clair
static constexpr uint16_t ROZA_PAPER       = 0xEEF8; // ivoire vieilli
static constexpr uint16_t ROZA_PAPER_DIM   = 0xC5D2; // ivoire atténué
static constexpr uint16_t ROZA_BURGUNDY    = 0x79E7; // bordeaux réservé aux alertes
