// Palette ROZA — instrument personnel de terrain, inspiration années 1940.
// Valeurs RGB565 pour l'écran ST7789 240x135. Aucun effet graphique ne dépend d'un accès réseau.
#pragma once
#include <stdint.h>

static constexpr uint16_t ROZA_INK         = 0x18E3; // encre / vert très sombre
static constexpr uint16_t ROZA_CARD        = 0x2124; // panneau olive sombre
static constexpr uint16_t ROZA_OLIVE_DARK  = 0x4A65; // olive patiné
static constexpr uint16_t ROZA_OLIVE       = 0x6B2A; // olive moyen
static constexpr uint16_t ROZA_OLIVE_LIGHT = 0xAE6E; // olive clair
static constexpr uint16_t ROZA_PAPER       = 0xEF18; // ivoire vieilli
static constexpr uint16_t ROZA_PAPER_DIM   = 0xC694; // ivoire atténué
static constexpr uint16_t ROZA_BURGUNDY    = 0x7987; // bordeaux réservé aux alertes
