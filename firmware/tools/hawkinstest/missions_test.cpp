#include "hawkins_missions.h"
#include <assert.h>
#include <stdio.h>
int main(){
 hawkins::MissionState s{}; hawkins::initializeMissions(s);
 assert(hawkins::isValidMissionState(s));
 assert(hawkins::missionStatus(s,hawkins::MissionId::FirstSignal)==hawkins::MissionStatus::Active);
 hawkins::recordMissionSector(s,false);
 hawkins::recordMissionSector(s,true);
 assert(hawkins::missionStatus(s,hawkins::MissionId::FirstSignal)==hawkins::MissionStatus::Complete);
 hawkins::recordMissionSector(s,false);
 hawkins::recordMissionSector(s,true);
 assert(hawkins::missionStatus(s,hawkins::MissionId::ThreeSectors)==hawkins::MissionStatus::Complete);
 assert(hawkins::missionStatus(s,hawkins::MissionId::AnomalyTrail)==hawkins::MissionStatus::Complete);
 assert(hawkins::missionRewardXp(hawkins::MissionId::AnomalyTrail)==150);
 puts("HAWKINS MISSIONS: tests reussis.");
}
