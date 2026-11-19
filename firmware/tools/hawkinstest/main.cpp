#include "hawkins_world.h"

#include <assert.h>
#include <stdio.h>

using namespace hawkins;

static void testInitialization() {
  WorldState world{};
  initializeWorld(world, 12345U, 1800000000ULL);
  assert(world.schemaVersion == kWorldSchemaVersion);
  assert(world.seed == 12345U);
  assert(world.startEpochSeconds == 1800000000ULL);
  assert(world.simulatedDays == 0U);
  assert(world.eventCount == 0U);
  assert(world.tension == 12U);
  assert(world.anomalyLevel == 4U);
}

static void testTimeAndNoRewind() {
  WorldState world{};
  const uint64_t start = 1800000000ULL;
  initializeWorld(world, 7U, start);

  AdvanceSummary result = advanceWorld(world, start + kSecondsPerDay - 1U);
  assert(result.daysProcessed == 0U);
  assert(world.simulatedDays == 0U);

  result = advanceWorld(world, start + 3U * kSecondsPerDay);
  assert(result.daysProcessed == 3U);
  assert(world.simulatedDays == 3U);

  result = advanceWorld(world, start + kSecondsPerDay);
  assert(result.daysProcessed == 0U);
  assert(world.simulatedDays == 3U);
}

static void testDeterministicEvolution() {
  WorldState a{};
  WorldState b{};
  const uint64_t start = 1800000000ULL;
  initializeWorld(a, 0x12345678U, start);
  initializeWorld(b, 0x12345678U, start);

  const uint64_t now = start + 900U * kSecondsPerDay;
  AdvanceSummary ar = advanceWorld(a, now);
  AdvanceSummary br = advanceWorld(b, now);

  assert(ar.daysProcessed == 900U);
  assert(br.daysProcessed == 900U);
  assert(a.simulatedDays == b.simulatedDays);
  assert(a.randomState == b.randomState);
  assert(a.tension == b.tension);
  assert(a.anomalyLevel == b.anomalyLevel);
  assert(a.factionActivity == b.factionActivity);
  assert(a.eventCount == b.eventCount);
  assert(a.latestEvent.sequence == b.latestEvent.sequence);
  assert(a.latestEvent.type == b.latestEvent.type);
}

static void testTenYearWorldDoesNotEnd() {
  WorldState world{};
  const uint64_t start = 1800000000ULL;
  initializeWorld(world, 99U, start);

  // Dix années civiles environ : le moteur n'a volontairement aucun état « terminé ».
  const uint64_t now = start + 3652ULL * kSecondsPerDay;
  AdvanceSummary result = advanceWorld(world, now);
  assert(result.daysProcessed == 3652U);
  assert(!result.catchUpPending);
  assert(world.simulatedDays == 3652U);
  assert(world.eventCount > 0U);
}

static void testBoundedCatchUp() {
  WorldState world{};
  const uint64_t start = 1800000000ULL;
  initializeWorld(world, 42U, start);
  const uint64_t now = start + 9000ULL * kSecondsPerDay;

  AdvanceSummary first = advanceWorld(world, now);
  assert(first.daysProcessed == kMaxCatchUpDaysPerCall);
  assert(first.catchUpPending);

  uint32_t totalProcessed = first.daysProcessed;
  AdvanceSummary next = first;
  while (next.catchUpPending) {
    next = advanceWorld(world, now);
    totalProcessed += next.daysProcessed;
  }
  assert(totalProcessed == 9000U);
  assert(world.simulatedDays == 9000U);
}

static void testMetricsAndEventNames() {
  WorldState world{};
  initializeWorld(world, 3U, 1800000000ULL);
  for (uint32_t day = 1; day <= 5000U; ++day) {
    advanceWorld(world, world.startEpochSeconds +
                            static_cast<uint64_t>(day) * kSecondsPerDay);
  }
  assert(world.tension <= 100U);
  assert(world.anomalyLevel <= 100U);
  assert(world.factionActivity <= 100U);
  assert(world.latestEvent.intensity <= 100U);
  assert(world.latestEvent.sector < 32U);
  assert(eventTypeName(EventType::PortalFluctuation)[0] != '\0');
  assert(eventTypeName(EventType::None)[0] != '\0');
}

int main() {
  testInitialization();
  testTimeAndNoRewind();
  testDeterministicEvolution();
  testTenYearWorldDoesNotEnd();
  testBoundedCatchUp();
  testMetricsAndEventNames();
  puts("HAWKINS: tous les tests du moteur de monde ont réussi.");
  return 0;
}
