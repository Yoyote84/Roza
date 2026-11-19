#include "hawkins_runtime.h"
#include "globals.h"
#include "hawkins_nvs.h"
#include <esp_system.h>
#include <time.h>
namespace hawkins { namespace {
const uint64_t kMinimumValidEpoch=1735689600ULL;
WorldState runtimeWorld{};bool worldLoaded=false;bool savePending=false;
uint32_t saveSequence=0,lastTickMs=0;RuntimeStatus status=RuntimeStatus::Empty;
bool systemEpoch(uint64_t& out){
 if(!timeSynced)return false;time_t now=time(nullptr);
 if(now<0||uint64_t(now)<kMinimumValidEpoch)return false;out=uint64_t(now);return true;
}
bool commitPending(){
 if(!savePending)return true;
 if(!worldSaveCommit(runtimeWorld,&saveSequence)){status=RuntimeStatus::SaveError;return false;}
 savePending=false;status=RuntimeStatus::Ready;return true;
}
}
void hawkinsRuntimeInit(){
 runtimeWorld=WorldState{};worldLoaded=false;savePending=false;saveSequence=0;lastTickMs=0;
 WorldSaveLoadStatus r=worldSaveLoad(runtimeWorld,&saveSequence);
 switch(r){
  case WorldSaveLoadStatus::Loaded:worldLoaded=true;status=RuntimeStatus::WaitingForClock;break;
  case WorldSaveLoadStatus::Empty:status=RuntimeStatus::Empty;break;
  case WorldSaveLoadStatus::Corrupt:status=RuntimeStatus::CorruptSave;break;
  default:status=RuntimeStatus::StorageError;break;
 }
}
bool hawkinsRuntimeUpdate(){
 uint32_t ms=millis();if(lastTickMs!=0&&ms-lastTickMs<1000U)return false;lastTickMs=ms;
 if(savePending){RuntimeStatus before=status;if(!commitPending())return true;if(status!=before)return true;}
 uint64_t epoch=0;bool clockOk=systemEpoch(epoch);
 if(!worldLoaded){
  if(status!=RuntimeStatus::Empty||!clockOk)return false;
  initializeWorld(runtimeWorld,esp_random(),epoch);worldLoaded=true;savePending=true;commitPending();return true;
 }
 if(!clockOk){RuntimeStatus before=status;status=RuntimeStatus::WaitingForClock;return before!=status;}
 if(epoch<runtimeWorld.startEpochSeconds){RuntimeStatus before=status;status=RuntimeStatus::ClockBeforeWorld;return before!=status;}
 AdvanceSummary r=advanceWorld(runtimeWorld,epoch);
 if(r.daysProcessed==0){RuntimeStatus before=status;status=RuntimeStatus::Ready;return before!=status;}
 savePending=true;commitPending();return true;
}
bool hawkinsRuntimeSaveNow(){if(!worldLoaded)return false;savePending=true;return commitPending();}
const WorldState* hawkinsRuntimeWorld(){return worldLoaded?&runtimeWorld:nullptr;}
RuntimeStatus hawkinsRuntimeStatus(){return status;}
const char* hawkinsRuntimeStatusText(){
 switch(status){
  case RuntimeStatus::Empty:return "NOUVEAU MONDE";
  case RuntimeStatus::Ready:return "MONDE SAUVEGARDE";
  case RuntimeStatus::WaitingForClock:return "ATTENTE HORLOGE";
  case RuntimeStatus::ClockBeforeWorld:return "HORLOGE ANTERIEURE";
  case RuntimeStatus::SaveError:return "ERREUR SAUVEGARDE";
  case RuntimeStatus::CorruptSave:return "SAUVEGARDE CORROMPUE";
  case RuntimeStatus::StorageError:return "NVS INDISPONIBLE";
  default:return "ETAT INCONNU";
 }
}
}
