#pragma once
#include <stdint.h>
namespace hawkins {
enum class MissionId : uint8_t { FirstSignal=0, ThreeSectors, AnomalyTrail, Count };
enum class MissionStatus : uint8_t { Locked=0, Active, Complete };
struct MissionState { uint32_t schemaVersion; uint8_t status[3]; uint16_t sectors; uint16_t discoveries; };
struct MissionDef { const char* title; const char* description; uint16_t target; };
constexpr uint32_t kMissionSchemaVersion=1U;
const MissionDef& missionDefinition(MissionId id);
void initializeMissions(MissionState& state);
bool isValidMissionState(const MissionState& state);
void recordMissionSector(MissionState& state, bool discovery);
uint16_t missionRewardXp(MissionId id);
MissionStatus missionStatus(const MissionState& state, MissionId id);
}
