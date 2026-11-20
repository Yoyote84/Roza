#pragma once

#include "hawkins_adventure.h"
#include "hawkins_exploration.h"
#include "hawkins_missions.h"
#include "hawkins_narrative_memory.h"
#include "hawkins_world.h"

#include <stddef.h>

namespace hawkins {

// Contexte compact construit uniquement depuis l'etat reel du moteur Hawkins.
// Aucun effet de bord : la sortie est du texte destine au modele local.
constexpr size_t kHawkinsAiContextCapacity = 768U;
bool formatHawkinsAiContext(const CharacterState& character,
                            const AdventureState& adventure,
                            const MissionState& missions,
                            const ExplorationState& exploration,
                            const WorldState& world,
                            const NarrativeMemory& narrative,
                            char* output, size_t capacity);

}  // namespace hawkins
