#include "hawkins_ai_context.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

namespace hawkins {
namespace {

bool appendFormat(char* output, size_t capacity, size_t& used,
                  const char* format, ...) {
  if (used >= capacity) return false;
  va_list args;
  va_start(args, format);
  const int written = vsnprintf(output + used, capacity - used, format, args);
  va_end(args);
  if (written < 0 || static_cast<size_t>(written) >= capacity - used) {
    output[capacity - 1U] = '\0';
    return false;
  }
  used += static_cast<size_t>(written);
  return true;
}

const char* missionStatusLabel(MissionStatus status) {
  switch (status) {
    case MissionStatus::Locked: return "verrouillee";
    case MissionStatus::Active: return "active";
    case MissionStatus::Complete: return "terminee";
    default: return "invalide";
  }
}

}  // namespace

bool formatHawkinsAiContext(const CharacterState& character,
                            const AdventureState& adventure,
                            const MissionState& missions,
                            const ExplorationState& exploration,
                            const WorldState& world,
                            const NarrativeMemory& narrative,
                            char* output, size_t capacity) {
  if (!output || capacity == 0U) return false;
  output[0] = '\0';
  if (!isValidCharacter(character) || !isValidAdventureState(adventure) ||
      !isValidMissionState(missions) ||
      !isValidExplorationState(exploration) ||
      !isValidNarrativeMemory(narrative)) {
    return false;
  }

  size_t used = 0U;
  if (!appendFormat(
          output, capacity, used,
          "ROLE:Narrateur Hawkins. Francais. Recit bref et coherent. "
          "Ne modifie jamais les stats, objets ou missions. "
          "MONDE:J%lu T%u A%u F%u E:%s. ",
          static_cast<unsigned long>(world.simulatedDays),
          static_cast<unsigned>(world.tension),
          static_cast<unsigned>(world.anomalyLevel),
          static_cast<unsigned>(world.factionActivity),
          eventTypeName(world.latestEvent.type))) {
    return false;
  }

  const CharacterClass& playerClass = characterClass(character.classId);
  if (!appendFormat(
          output, capacity, used,
          "JOUEUR:%s/%s N%u PV%u ST%u XP%lu SP%u "
          "ATTR%u-%u-%u-%u-%u-%u. ",
          character.name, playerClass.name,
          static_cast<unsigned>(character.level),
          static_cast<unsigned>(character.hitPoints),
          static_cast<unsigned>(character.stress),
          static_cast<unsigned long>(character.xp),
          static_cast<unsigned>(character.skillPoints),
          static_cast<unsigned>(character.attributes[Strength]),
          static_cast<unsigned>(character.attributes[Endurance]),
          static_cast<unsigned>(character.attributes[Agility]),
          static_cast<unsigned>(character.attributes[Intelligence]),
          static_cast<unsigned>(character.attributes[Perception]),
          static_cast<unsigned>(character.attributes[Charisma]))) {
    return false;
  }

  if (!appendFormat(
          output, capacity, used,
          "SAC:MED%u THE%u PILE%u PREUVE%u. COMP:",
          static_cast<unsigned>(adventure.items[static_cast<uint8_t>(AdventureItem::Medkit)]),
          static_cast<unsigned>(adventure.items[static_cast<uint8_t>(AdventureItem::CalmingTea)]),
          static_cast<unsigned>(adventure.items[static_cast<uint8_t>(AdventureItem::SpareBattery)]),
          static_cast<unsigned>(adventure.items[static_cast<uint8_t>(AdventureItem::Evidence)]))) {
    return false;
  }

  for (uint8_t i = 0U; i < kAdventureSkillCount; ++i) {
    const auto skill = static_cast<AdventureSkill>(i);
    if (!appendFormat(
            output, capacity, used, "%s:%u%s",
            adventureSkillDefinition(skill).name,
            static_cast<unsigned>(adventureHasSkill(adventure, skill) ? 1U : 0U),
            i + 1U == kAdventureSkillCount ? ". " : ",")) {
      return false;
    }
  }

  if (!appendFormat(output, capacity, used,
                    "MISSIONS:")) return false;
  for (uint8_t i = 0U; i < kMissionCount; ++i) {
    const MissionId id = static_cast<MissionId>(i);
    if (!appendFormat(
            output, capacity, used, "%s=%s%s",
            missionDefinition(id).title,
            missionStatusLabel(missionStatus(missions, id)),
            i + 1U == kMissionCount ? ". " : ",")) {
      return false;
    }
  }

  if (!appendFormat(
          output, capacity, used,
          "EXPLO:%lu secteurs/%lu decouvertes GPS:%s EVT:%s/%u CELL:%08lX. ",
          static_cast<unsigned long>(exploration.sectorsVisited),
          static_cast<unsigned long>(exploration.discoveries),
          exploration.hasOrigin ? "actif" : "sans-origine",
          eventTypeName(exploration.lastEventType),
          static_cast<unsigned>(exploration.lastEventIntensity),
          static_cast<unsigned long>(exploration.lastEventCellKey))) {
    return false;
  }

  char narrativeSummary[kNarrativeSummaryCapacity] = {};
  if (!formatNarrativeMemorySummary(narrative, narrativeSummary,
                                    sizeof(narrativeSummary))) {
    return false;
  }
  return appendFormat(output, capacity, used, "%s", narrativeSummary);
}

}  // namespace hawkins
