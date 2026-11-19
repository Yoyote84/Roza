#include "hawkins_save_codec.h"
namespace hawkins {
namespace {
constexpr uint32_t kSaveMagic=0x534B5748U;
constexpr size_t kCrcOffset=12;
void w16(uint8_t* p,uint16_t v){p[0]=uint8_t(v);p[1]=uint8_t(v>>8);}
void w32(uint8_t* p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=uint8_t(v>>(8U*i));}
void w64(uint8_t* p,uint64_t v){for(unsigned i=0;i<8;++i)p[i]=uint8_t(v>>(8U*i));}
uint16_t r16(const uint8_t* p){return uint16_t(p[0])|uint16_t(uint16_t(p[1])<<8);}
uint32_t r32(const uint8_t* p){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=uint32_t(p[i])<<(8U*i);return v;}
uint64_t r64(const uint8_t* p){uint64_t v=0;for(unsigned i=0;i<8;++i)v|=uint64_t(p[i])<<(8U*i);return v;}
uint32_t crc32(const uint8_t* p,size_t n){
  uint32_t c=0xFFFFFFFFU;
  for(size_t i=0;i<n;++i){c^=p[i];for(unsigned b=0;b<8;++b){uint32_t m=0U-(c&1U);c=(c>>1)^(0xEDB88320U&m);}}
  return ~c;
}
void encodePayload(const WorldState& w,uint8_t* p){
  w32(p+0,w.schemaVersion);w32(p+4,w.seed);w64(p+8,w.startEpochSeconds);
  w32(p+16,w.simulatedDays);w32(p+20,w.randomState);w32(p+24,w.eventCount);
  p[28]=w.tension;p[29]=w.anomalyLevel;p[30]=w.factionActivity;
  w32(p+31,w.latestEvent.sequence);w32(p+35,w.latestEvent.day);
  p[39]=uint8_t(w.latestEvent.type);p[40]=w.latestEvent.intensity;p[41]=w.latestEvent.sector;
}
bool decodePayload(const uint8_t* p,WorldState& w){
  w=WorldState{};w.schemaVersion=r32(p+0);w.seed=r32(p+4);w.startEpochSeconds=r64(p+8);
  w.simulatedDays=r32(p+16);w.randomState=r32(p+20);w.eventCount=r32(p+24);
  w.tension=p[28];w.anomalyLevel=p[29];w.factionActivity=p[30];
  w.latestEvent.sequence=r32(p+31);w.latestEvent.day=r32(p+35);
  w.latestEvent.type=static_cast<EventType>(p[39]);w.latestEvent.intensity=p[40];w.latestEvent.sector=p[41];
  return isValidWorldState(w);
}
}
bool isValidWorldState(const WorldState& w){
  if(w.schemaVersion!=kWorldSchemaVersion||w.startEpochSeconds==0||w.randomState==0||
     w.tension>100||w.anomalyLevel>100||w.factionActivity>100)return false;
  uint8_t t=uint8_t(w.latestEvent.type);
  if(w.eventCount==0)return w.latestEvent.sequence==0&&w.latestEvent.type==EventType::None&&w.latestEvent.intensity==0;
  return w.latestEvent.sequence==w.eventCount&&w.latestEvent.day<=w.simulatedDays&&
    t>=uint8_t(EventType::StrangeSignal)&&t<=uint8_t(EventType::PortalFluctuation)&&
    w.latestEvent.intensity>=1&&w.latestEvent.intensity<=100&&w.latestEvent.sector<32;
}
bool encodeWorldSave(const WorldState& w,uint32_t seq,uint8_t* out,size_t cap){
  if(!out||cap<kWorldSaveEncodedSize||seq==0||!isValidWorldState(w))return false;
  for(size_t i=0;i<kWorldSaveEncodedSize;++i)out[i]=0;
  w32(out,kSaveMagic);w16(out+4,kWorldSaveFormatVersion);w16(out+6,uint16_t(kWorldSavePayloadSize));
  w32(out+8,seq);w32(out+kCrcOffset,0);encodePayload(w,out+kWorldSaveHeaderSize);
  w32(out+kCrcOffset,crc32(out,kWorldSaveEncodedSize));return true;
}
bool decodeWorldSave(const uint8_t* in,size_t len,WorldState& w,uint32_t& seq){
  if(!in||len!=kWorldSaveEncodedSize||r32(in)!=kSaveMagic||
     r16(in+4)!=kWorldSaveFormatVersion||r16(in+6)!=kWorldSavePayloadSize)return false;
  uint32_t stored=r32(in+kCrcOffset);uint8_t check[kWorldSaveEncodedSize];
  for(size_t i=0;i<kWorldSaveEncodedSize;++i)check[i]=in[i];
  w32(check+kCrcOffset,0);if(crc32(check,kWorldSaveEncodedSize)!=stored)return false;
  uint32_t candidate=r32(in+8);if(candidate==0)return false;
  WorldState temp{};if(!decodePayload(in+kWorldSaveHeaderSize,temp))return false;
  w=temp;seq=candidate;return true;
}
}  // namespace hawkins
