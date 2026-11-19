#include "hawkins_runtime.h"
#include "globals.h"
#include "hawkins_nvs.h"
#include <esp_system.h>

namespace hawkins {
namespace {

// Valeur sentinelle interne : le temps du jeu est piloté par l'exploration,
// jamais par une horloge ou un service réseau.
constexpr uint64_t kOfflineWorldAnchorSeconds = 1ULL;

WorldState runtimeWorld{};
bool worldLoaded = false;
bool savePending = false;
uint32_t saveSequence = 0;
uint32_t lastTickMs = 0;
RuntimeStatus status = RuntimeStatus::Empty;

bool commitPending() {
  if (!savePending) return true;
  if (!worldSaveCommit(runtimeWorld, &saveSequence)) {
    status = RuntimeStatus::SaveError;
    return false;
  }
  savePending = false;
  status = RuntimeStatus::Ready;
  return true;
}

}  // namespace

void hawkinsRuntimeInit() {
  runtimeWorld = WorldState{};
  worldLoaded = false;
  savePending = false;
  saveSequence = 0;
  lastTickMs = 0;

  const WorldSaveLoadStatus result = worldSaveLoad(runtimeWorld, &saveSequence);
  switch (result) {
    case WorldSaveLoadStatus::Loaded:
      worldLoaded = true;
      status = RuntimeStatus::Ready;
      // Migration transparente de la sauvegarde des étapes précédentes :
      // même monde et mêmes statistiques, mais calendrier désormais 100 % local.
      if (runtimeWorld.startEpochSeconds != kOfflineWorldAnchorSeconds) {
        runtimeWorld.startEpochSeconds = kOfflineWorldAnchorSeconds;
        savePending = true;
      }
      break;
    case WorldSaveLoadStatus::Empty:
      status = RuntimeStatus::Empty;
      break;
    case WorldSaveLoadStatus::Corrupt:
      status = RuntimeStatus::CorruptSave;
      break;
    default:
      status = RuntimeStatus::StorageError;
      break;
  }
}

bool hawkinsRuntimeUpdate() {
  const uint32_t ms = millis();
  if (lastTickMs != 0U && ms - lastTickMs < 1000U) return false;
  lastTickMs = ms;

  if (savePending) {
    const RuntimeStatus before = status;
    if (!commitPending()) return true;
    if (status != before) return true;
  }

  if (!worldLoaded) {
    if (status != RuntimeStatus::Empty) return false;
    // Création immédiate : aucun GPS, NTP, Wi-Fi ou réglage de date requis.
    initializeWorld(runtimeWorld, esp_random(), kOfflineWorldAnchorSeconds);
    worldLoaded = true;
    savePending = true;
    commitPending();
    return true;
  }

  const RuntimeStatus before = status;
  status = RuntimeStatus::Ready;
  return before != status;
}

bool hawkinsRuntimeSaveNow() {
  if (!worldLoaded) return false;
  savePending = true;
  return commitPending();
}

bool hawkinsRuntimeAdvanceDays(uint32_t days) {
  if (!worldLoaded || days == 0U) return false;
  const AdvanceSummary result = advanceWorldByDays(runtimeWorld, days);
  if (result.daysProcessed == 0U) return false;
  savePending = true;
  return commitPending();
}

const WorldState* hawkinsRuntimeWorld() {
  return worldLoaded ? &runtimeWorld : nullptr;
}

RuntimeStatus hawkinsRuntimeStatus() {
  return status;
}

const char* hawkinsRuntimeStatusText() {
  switch (status) {
    case RuntimeStatus::Empty: return "NOUVEAU MONDE LOCAL";
    case RuntimeStatus::Ready: return "MONDE SAUVEGARDE";
    case RuntimeStatus::WaitingForClock: return "ATTENTE HORLOGE";
    case RuntimeStatus::ClockBeforeWorld: return "HORLOGE ANTERIEURE";
    case RuntimeStatus::SaveError: return "ERREUR SAUVEGARDE";
    case RuntimeStatus::CorruptSave: return "SAUVEGARDE CORROMPUE";
    case RuntimeStatus::StorageError: return "NVS INDISPONIBLE";
    default: return "ETAT INCONNU";
  }
}

}  // namespace hawkins
