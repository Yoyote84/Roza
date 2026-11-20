#include "hawkins_narrative_memory_codec.h"
#include <string.h>
namespace hawkins {
namespace {
constexpr uint32_t kMagic=0x4D4E5748U; // HWNM
constexpr uint16_t kVersion=1U;
constexpr size_t kCrcOffset=12U;
void w16(uint8_t*p,uint16_t v){p[0]=uint8_t(v);p[1]=uint8_t(v>>8U);}
void w32(uint8_t*p,uint32_t v){for(unsigned i=0;i<4U;i++)p[i]=uint8_t(v>>(i*8U));}
uint16_t r16(const uint8_t*p){return uint16_t(p[0])|uint16_t(uint16_t(p[1])<<8U);}
uint32_t r32(const uint8_t*p){uint32_t v=0;for(unsigned i=0;i<4U;i++)v|=uint32_t(p[i])<<(i*8U);return v;}
uint32_t crc(const uint8_t*d,size_t n){uint32_t c=0xFFFFFFFFU;for(size_t i=0;i<n;i++){c^=d[i];for(unsigned b=0;b<8U;b++){uint32_t m=0U-(c&1U);c=(c>>1U)^(0xEDB88320U&m);}}return ~c;}
void payloadEncode(const NarrativeMemory&m,uint8_t*p){
 w32(p,m.schemaVersion);w32(p+4,m.nextSequence);w32(p+8,m.totalRecords);
 p[12]=m.recordCount;p[13]=m.replaceIndex;p[14]=m.plotFlags;p[15]=m.npcCount;
 for(uint8_t i=0;i<kNarrativeNpcCount;i++)p[16+i]=m.relationships[i];
 for(uint8_t i=0;i<kNarrativeMemoryRecordCount;i++){
  const NarrativeRecord&r=m.records[i];size_t o=20U+12U*i;
  w32(p+o,r.sequence);w32(p+o+4U,r.cellKey);w16(p+o+8U,r.detail);
  p[o+10U]=r.type;p[o+11U]=r.intensity;
 }
}
bool payloadDecode(const uint8_t*p,NarrativeMemory&m){
 m= NarrativeMemory{};m.schemaVersion=r32(p);m.nextSequence=r32(p+4);m.totalRecords=r32(p+8);
 m.recordCount=p[12];m.replaceIndex=p[13];m.plotFlags=p[14];m.npcCount=p[15];
 for(uint8_t i=0;i<kNarrativeNpcCount;i++)m.relationships[i]=p[16+i];
 for(uint8_t i=0;i<kNarrativeMemoryRecordCount;i++){
  size_t o=20U+12U*i;NarrativeRecord&r=m.records[i];
  r.sequence=r32(p+o);r.cellKey=r32(p+o+4U);r.detail=r16(p+o+8U);r.type=p[o+10U];r.intensity=p[o+11U];
 }
 return isValidNarrativeMemory(m);
}
}
bool encodeNarrativeMemorySave(const NarrativeMemory&m,uint32_t sequence,uint8_t*out,size_t cap){
 if(!out||cap<kNarrativeSaveEncodedSize||sequence==0U||!isValidNarrativeMemory(m))return false;
 memset(out,0,kNarrativeSaveEncodedSize);w32(out,kMagic);w16(out+4,kVersion);w16(out+6,uint16_t(kNarrativeSavePayloadSize));w32(out+8,sequence);
 payloadEncode(m,out+kNarrativeSaveHeaderSize);w32(out+kCrcOffset,crc(out,kNarrativeSaveEncodedSize));return true;
}
bool decodeNarrativeMemorySave(const uint8_t*in,size_t n,NarrativeMemory&m,uint32_t&seq){
 if(!in||n!=kNarrativeSaveEncodedSize||r32(in)!=kMagic||r16(in+4)!=kVersion||r16(in+6)!=kNarrativeSavePayloadSize)return false;
 uint8_t check[kNarrativeSaveEncodedSize];memcpy(check,in,n);uint32_t stored=r32(check+kCrcOffset);w32(check+kCrcOffset,0U);if(crc(check,n)!=stored)return false;
 uint32_t candidate=r32(in+8);if(candidate==0U)return false;NarrativeMemory tmp{};if(!payloadDecode(in+kNarrativeSaveHeaderSize,tmp))return false;m=tmp;seq=candidate;return true;
}
}
