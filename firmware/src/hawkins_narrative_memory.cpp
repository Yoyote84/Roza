#include "hawkins_narrative_memory.h"
#include <string.h>
namespace hawkins {
void initializeNarrativeMemory(NarrativeMemory& memory) {
  memory = NarrativeMemory{};
  memory.schemaVersion = kNarrativeMemorySchemaVersion;
  memory.nextSequence = 1U;
}
bool isValidNarrativeMemory(const NarrativeMemory& memory) {
  if(memory.schemaVersion!=kNarrativeMemorySchemaVersion ||
     memory.nextSequence==0U || memory.recordCount>kNarrativeMemoryRecordCount ||
     memory.replaceIndex>=kNarrativeMemoryRecordCount ||
     memory.npcCount>kNarrativeNpcCount) return false;
  if(memory.recordCount<kNarrativeMemoryRecordCount &&
     memory.replaceIndex!=0U) return false;
  for(uint8_t i=0;i<memory.recordCount;++i){
    const NarrativeRecord& r=memory.records[i];
    if(r.sequence==0U || r.type<static_cast<uint8_t>(NarrativeRecordType::FirstContact) ||
       r.type>static_cast<uint8_t>(NarrativeRecordType::NpcContact) ||
       r.intensity>100U) return false;
  }
  return true;
}
bool narrativeRecord(NarrativeMemory& memory,NarrativeRecordType type,
                    uint32_t cellKey,uint16_t detail,uint8_t intensity){
  if(!isValidNarrativeMemory(memory) ||
     static_cast<uint8_t>(type)<1U || static_cast<uint8_t>(type)>8U ||
     intensity>100U || memory.nextSequence==UINT32_MAX) return false;
  NarrativeRecord r{memory.nextSequence++,cellKey,detail,
                    static_cast<uint8_t>(type),intensity};
  if(memory.recordCount<kNarrativeMemoryRecordCount){
    memory.records[memory.recordCount++]=r;
  }else{
    memory.records[memory.replaceIndex]=r;
    memory.replaceIndex=static_cast<uint8_t>((memory.replaceIndex+1U)%kNarrativeMemoryRecordCount);
  }
  if(memory.totalRecords<UINT32_MAX) ++memory.totalRecords;
  return isValidNarrativeMemory(memory);
}
bool narrativeSetPlot(NarrativeMemory& memory,uint8_t plotId,bool open){
  if(!isValidNarrativeMemory(memory)||plotId>=kNarrativePlotCount) return false;
  const uint8_t bit=static_cast<uint8_t>(1U<<plotId);
  if(open) memory.plotFlags|=bit; else memory.plotFlags&=static_cast<uint8_t>(~bit);
  return true;
}
bool narrativeAdjustNpcRelationship(NarrativeMemory& memory,uint8_t npcId,int8_t delta){
  if(!isValidNarrativeMemory(memory)||npcId>=kNarrativeNpcCount) return false;
  int value=memory.relationships[npcId]+delta;
  if(value<kNarrativeMinRelationship)value=kNarrativeMinRelationship;
  if(value>kNarrativeMaxRelationship)value=kNarrativeMaxRelationship;
  memory.relationships[npcId]=static_cast<uint8_t>(value);
  if(memory.npcCount<=npcId)memory.npcCount=npcId+1U;
  return true;
}
const NarrativeRecord* narrativeLatest(const NarrativeMemory& memory){
  if(!isValidNarrativeMemory(memory)||memory.recordCount==0U)return nullptr;
  uint32_t latest=0U; const NarrativeRecord* out=nullptr;
  for(uint8_t i=0;i<memory.recordCount;++i){
    if(memory.records[i].sequence>latest){latest=memory.records[i].sequence;out=&memory.records[i];}
  }
  return out;
}
const NarrativeRecord* narrativeRecordAt(const NarrativeMemory& memory,uint8_t index){
  if(!isValidNarrativeMemory(memory)||index>=memory.recordCount)return nullptr;
  uint32_t target=0U;
  // Select chronological sequence order, not physical ring order.
  for(uint8_t rank=0;rank<=index;++rank){
    uint32_t best=UINT32_MAX; const NarrativeRecord* selected=nullptr;
    for(uint8_t i=0;i<memory.recordCount;++i){
      const uint32_t seq=memory.records[i].sequence;
      if(seq>target && seq<best){best=seq;selected=&memory.records[i];}
    }
    if(!selected)return nullptr;
    target=best;
    if(rank==index)return selected;
  }
  return nullptr;
}
}
