#include "hawkins_world.h"

namespace hawkins {
namespace {

uint32_t nextRandom(WorldState& world) {
  // xorshift32 : petit générateur déterministe, adapté aux événements de jeu.
  // Il ne sert ni à la cryptographie ni à la génération de secrets.
  uint32_t x = world.randomState;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  if (x == 0) x = 0x6D2B79F5U;
  world.randomState = x;
  return x;
}

uint32_t randomBelow(WorldState& world, uint32_t upperBound) {
  return upperBound == 0 ? 0 : nextRandom(world) % upperBound;
}

uint8_t clampMetric(int value) {
  if (value < 0) return 0;
  if (value > 100) return 100;
  return static_cast<uint8_t>(value);
}

void simulateOneDay(WorldState& world, uint32_t day, AdvanceSummary& summary) {
  const int tensionDrift = static_cast<int>(randomBelow(world, 7)) - 4;
  const int anomalyDrift = static_cast<int>(randomBelow(world, 5)) - 3;
  const int factionDrift = static_cast<int>(randomBelow(world, 7)) - 4;

  world.tension = clampMetric(static_cast<int>(world.tension) + tensionDrift +
                              (world.anomalyLevel > 65 ? 1 : 0));
  world.anomalyLevel = clampMetric(static_cast<int>(world.anomalyLevel) + anomalyDrift +
                                   (world.tension > 65 ? 1 : 0));
  world.factionActivity = clampMetric(static_cast<int>(world.factionActivity) + factionDrift +
                                      (world.anomalyLevel > 70 ? 1 : 0));

  const uint32_t eventChance = 3U + world.tension / 10U + world.anomalyLevel / 7U +
                               world.factionActivity / 18U;
  if (randomBelow(world, 100U) >= eventChance) return;

  const uint32_t choice = randomBelow(world, 100U);
  EventType type = EventType::StrangeSignal;
  if (world.anomalyLevel >= 70 && choice < 45) {
    type = EventType::PortalFluctuation;
  } else if (world.factionActivity >= 65 && choice < 60) {
    type = EventType::FactionMovement;
  } else if (world.tension >= 60 && choice < 75) {
    type = EventType::CreatureTrace;
  } else {
    switch (choice % 5U) {
      case 0: type = EventType::StrangeSignal; break;
      case 1: type = EventType::PowerAnomaly; break;
      case 2: type = EventType::CreatureTrace; break;
      case 3: type = EventType::DistressCall; break;
      default: type = EventType::FactionMovement; break;
    }
  }

  const uint16_t pressure = static_cast<uint16_t>(world.tension) +
                            static_cast<uint16_t>(world.anomalyLevel) +
                            static_cast<uint16_t>(world.factionActivity);
  const uint32_t intensityRange = pressure / 3U + 1U;
  WorldEvent event{};
  event.sequence = ++world.eventCount;
  event.day = day;
  event.type = type;
  uint32_t intensity = 1U + randomBelow(world, intensityRange);
  if (intensity > 100U) intensity = 100U;
  event.intensity = static_cast<uint8_t>(intensity);
  event.sector = static_cast<uint8_t>(randomBelow(world, 32U));
  world.latestEvent = event;
  summary.eventsCreated++;
  summary.hasLatestEvent = true;
  summary.latestEvent = event;
}

}  // namespace

void initializeWorld(WorldState& world, uint32_t seed, uint64_t startEpochSeconds) {
  world = WorldState{};
  world.schemaVersion = kWorldSchemaVersion;
  world.seed = seed;
  world.startEpochSeconds = startEpochSeconds;
  world.randomState = seed == 0 ? 0xA341316CU : seed;
  world.tension = 12;
  world.anomalyLevel = 4;
  world.factionActivity = 8;
  world.latestEvent = WorldEvent{};
}

AdvanceSummary advanceWorld(WorldState& world, uint64_t nowEpochSeconds) {
  AdvanceSummary summary{};
  if (world.schemaVersion != kWorldSchemaVersion || world.startEpochSeconds == 0 ||
      nowEpochSeconds < world.startEpochSeconds) {
    return summary;
  }

  const uint64_t elapsedSeconds = nowEpochSeconds - world.startEpochSeconds;
  const uint64_t targetDay64 = elapsedSeconds / kSecondsPerDay;
  if (targetDay64 <= world.simulatedDays) return summary;

  const uint64_t dueDays = targetDay64 - world.simulatedDays;
  const uint32_t daysToProcess = dueDays > kMaxCatchUpDaysPerCall
                                     ? kMaxCatchUpDaysPerCall
                                     : static_cast<uint32_t>(dueDays);

  for (uint32_t i = 0; i < daysToProcess; ++i) {
    const uint32_t day = world.simulatedDays + 1U;
    simulateOneDay(world, day, summary);
    world.simulatedDays = day;
  }

  summary.daysProcessed = daysToProcess;
  summary.catchUpPending = targetDay64 > world.simulatedDays;
  return summary;
}

const char* eventTypeName(EventType type) {
  switch (type) {
    case EventType::StrangeSignal: return "Signal étrange";
    case EventType::PowerAnomaly: return "Anomalie électrique";
    case EventType::CreatureTrace: return "Trace de créature";
    case EventType::FactionMovement: return "Mouvement de faction";
    case EventType::DistressCall: return "Appel de détresse";
    case EventType::PortalFluctuation: return "Fluctuation de brèche";
    default: return "Aucun événement";
  }
}

}  // namespace hawkins
