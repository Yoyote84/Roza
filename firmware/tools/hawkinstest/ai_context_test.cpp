#include "hawkins_ai_context.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

using namespace hawkins;

static void testContextUsesRealGameState() {
  CharacterState character{};
  assert(createCharacter(character, 0U, "ELI"));
  AdventureState adventure{};
  initializeAdventure(adventure);
  MissionState missions{};
  initializeMissions(missions);
  ExplorationState exploration{};
  initializeExploration(exploration);
  WorldState world{};
  initializeWorld(world, 12345U, 1000U);
  NarrativeMemory narrative{};
  initializeNarrativeMemory(narrative);
  assert(narrativeRecord(narrative, NarrativeRecordType::FirstContact,
                         0U, 1U, 60U));
  assert(narrativeSetPlot(narrative, 0U, true));

  char context[kHawkinsAiContextCapacity] = {};
  assert(formatHawkinsAiContext(character, adventure, missions, exploration,
                                world, narrative, context, sizeof(context)));
  assert(strstr(context, "ROLE:Narrateur Hawkins") != nullptr);
  assert(strstr(context, "JOUEUR:ELI") != nullptr);
  assert(strstr(context, "SAC:MED1") != nullptr);
  assert(strstr(context, "MISSIONS:") != nullptr);
  assert(strstr(context, "EXPLO:0 secteurs") != nullptr);
  assert(strstr(context, "RECIT:1") != nullptr);
}

static void testContextRejectsInvalidAndSmallBuffers() {
  CharacterState character{};
  assert(createCharacter(character, 0U, "ELI"));
  AdventureState adventure{};
  initializeAdventure(adventure);
  MissionState missions{};
  initializeMissions(missions);
  ExplorationState exploration{};
  initializeExploration(exploration);
  WorldState world{};
  initializeWorld(world, 1U, 1U);
  NarrativeMemory narrative{};
  initializeNarrativeMemory(narrative);

  char tiny[8] = {};
  assert(!formatHawkinsAiContext(character, adventure, missions, exploration,
                                 world, narrative, tiny, sizeof(tiny)));
  assert(tiny[sizeof(tiny) - 1U] == '\0');
  adventure.items[0] = 255U;
  char context[kHawkinsAiContextCapacity] = {};
  assert(!formatHawkinsAiContext(character, adventure, missions, exploration,
                                 world, narrative, context, sizeof(context)));
}

int main() {
  testContextUsesRealGameState();
  testContextRejectsInvalidAndSmallBuffers();
  puts("HAWKINS: contexte reel pour IA hors ligne valide.");
  return 0;
}
