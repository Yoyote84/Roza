#include "hawkins_missions.h"
namespace hawkins {
namespace {
const MissionDef defs[3]={
 {"LE PREMIER SIGNAL","Decouvrir un premier signal anormal",1},
 {"CARTOGRAPHIE","Explorer trois secteurs distincts",3},
 {"PISTE DE L'ANOMALIE","Trouver deux signaux",2}
};
}
const MissionDef& missionDefinition(MissionId id){return defs[static_cast<uint8_t>(id)<3?static_cast<uint8_t>(id):0];}
void initializeMissions(MissionState& s){s=MissionState{};s.schemaVersion=kMissionSchemaVersion;s.status[0]=static_cast<uint8_t>(MissionStatus::Active);}
bool isValidMissionState(const MissionState& s){if(s.schemaVersion!=kMissionSchemaVersion||s.sectors>65535U||s.discoveries>65535U)return false;for(uint8_t v:s.status)if(v>static_cast<uint8_t>(MissionStatus::Complete))return false;return true;}
MissionStatus missionStatus(const MissionState& s,MissionId id){const uint8_t i=static_cast<uint8_t>(id);return i<3?static_cast<MissionStatus>(s.status[i]):MissionStatus::Locked;}
void recordMissionSector(MissionState& s,bool discovery){
 if(!isValidMissionState(s))return;
 if(s.sectors<65535U)++s.sectors;
 if(discovery&&s.discoveries<65535U)++s.discoveries;
 if(s.sectors>=defs[1].target)s.status[1]=static_cast<uint8_t>(MissionStatus::Complete);
 if(s.discoveries>=defs[2].target)s.status[2]=static_cast<uint8_t>(MissionStatus::Complete);
 if(s.discoveries>=defs[0].target)s.status[0]=static_cast<uint8_t>(MissionStatus::Complete);
}
uint16_t missionRewardXp(MissionId id){switch(id){case MissionId::FirstSignal:return 100;case MissionId::ThreeSectors:return 75;case MissionId::AnomalyTrail:return 150;default:return 0;}}
}
