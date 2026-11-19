#include "hawkins_nvs.h"
#include "globals.h"
#include "hawkins_save_codec.h"
#include <Preferences.h>
#include <string.h>
namespace hawkins { namespace {
const char* kNamespace="hawkins";const char* kSlot0="slot0";const char* kSlot1="slot1";
struct Slot{bool present;bool valid;uint32_t sequence;WorldState world;};
bool readSlot(Preferences& p,const char* key,Slot& s){
 s=Slot{};size_t n=p.getBytesLength(key);if(n==0)return true;s.present=true;
 if(n!=kWorldSaveEncodedSize)return true;uint8_t bytes[kWorldSaveEncodedSize];
 if(p.getBytes(key,bytes,n)!=n)return false;s.valid=decodeWorldSave(bytes,n,s.world,s.sequence);return true;
}
bool newer(uint32_t a,uint32_t b){return static_cast<int32_t>(a-b)>0;}
bool openStore(){return prefs.begin(kNamespace,false);}
}
WorldSaveLoadStatus worldSaveLoad(WorldState& world,uint32_t* sequence){
 PrefsLock lock;if(!openStore())return WorldSaveLoadStatus::StorageError;
 Slot a{},b{};bool ok=readSlot(prefs,kSlot0,a)&&readSlot(prefs,kSlot1,b);prefs.end();
 if(!ok)return WorldSaveLoadStatus::StorageError;
 if(!a.valid&&!b.valid)return (a.present||b.present)?WorldSaveLoadStatus::Corrupt:WorldSaveLoadStatus::Empty;
 const Slot& best=!a.valid?b:(!b.valid?a:(newer(a.sequence,b.sequence)?a:b));
 world=best.world;if(sequence)*sequence=best.sequence;return WorldSaveLoadStatus::Loaded;
}
bool worldSaveCommit(const WorldState& world,uint32_t* sequence){
 if(!isValidWorldState(world))return false;PrefsLock lock;if(!openStore())return false;
 Slot a{},b{};if(!readSlot(prefs,kSlot0,a)||!readSlot(prefs,kSlot1,b)){prefs.end();return false;}
 uint32_t next=1;const char* target=kSlot0;
 if(a.valid&&b.valid){if(newer(a.sequence,b.sequence)){next=a.sequence+1U;target=kSlot1;}else{next=b.sequence+1U;target=kSlot0;}}
 else if(a.valid){next=a.sequence+1U;target=kSlot1;}
 else if(b.valid){next=b.sequence+1U;target=kSlot0;}
 else if(a.present&&!b.present)target=kSlot1;
 if(next==0)next=1;uint8_t encoded[kWorldSaveEncodedSize];
 if(!encodeWorldSave(world,next,encoded,sizeof(encoded))){prefs.end();return false;}
 size_t written=prefs.putBytes(target,encoded,sizeof(encoded));bool verified=false;
 if(written==sizeof(encoded)&&prefs.getBytesLength(target)==sizeof(encoded)){
  uint8_t back[kWorldSaveEncodedSize];WorldState restored{};uint32_t restoredSeq=0;
  if(prefs.getBytes(target,back,sizeof(back))==sizeof(back)&&memcmp(encoded,back,sizeof(encoded))==0&&
     decodeWorldSave(back,sizeof(back),restored,restoredSeq)&&restoredSeq==next)verified=true;
 }
 prefs.end();if(verified&&sequence)*sequence=next;return verified;
}
}
