#include "hawkins_narrative_store.h"

#include "globals.h"
#include "hawkins_narrative_memory_codec.h"
#include <Preferences.h>
#include <string.h>

namespace hawkins {
namespace {
constexpr char kNamespace[] = "hawkins";
constexpr char kSlot0[] = "nar0";
constexpr char kSlot1[] = "nar1";

struct Slot {
  bool present;
  bool valid;
  uint32_t sequence;
  NarrativeMemory memory;
};

bool sequenceNewer(uint32_t a, uint32_t b) {
  return static_cast<int32_t>(a - b) > 0;
}
bool readSlot(Preferences& store, const char* key, Slot& slot) {
  slot = Slot{};
  const size_t length = store.getBytesLength(key);
  if (length == 0U) return true;
  slot.present = true;
  if (length != kNarrativeSaveEncodedSize) return true;
  uint8_t bytes[kNarrativeSaveEncodedSize];
  if (store.getBytes(key, bytes, length) != length) return false;
  slot.valid = decodeNarrativeMemorySave(bytes, length, slot.memory,
                                         slot.sequence);
  return true;
}
const Slot& newestValid(const Slot& a, const Slot& b) {
  if (!a.valid) return b;
  if (!b.valid) return a;
  return sequenceNewer(a.sequence, b.sequence) ? a : b;
}
}  // namespace

NarrativeSaveLoadStatus narrativeSaveLoad(NarrativeMemory& memory,
                                          uint32_t* sequence) {
  PrefsLock lock;
  if (!prefs.begin(kNamespace, false)) return NarrativeSaveLoadStatus::StorageError;
  Slot a{}, b{};
  const bool ok = readSlot(prefs, kSlot0, a) && readSlot(prefs, kSlot1, b);
  prefs.end();
  if (!ok) return NarrativeSaveLoadStatus::StorageError;
  if (!a.valid && !b.valid) {
    return (a.present || b.present) ? NarrativeSaveLoadStatus::Corrupt
                                    : NarrativeSaveLoadStatus::Empty;
  }
  const Slot& best = newestValid(a, b);
  memory = best.memory;
  if (sequence) *sequence = best.sequence;
  return NarrativeSaveLoadStatus::Loaded;
}

bool narrativeSaveCommit(const NarrativeMemory& memory, uint32_t* sequence) {
  if (!isValidNarrativeMemory(memory)) return false;
  PrefsLock lock;
  if (!prefs.begin(kNamespace, false)) return false;
  Slot a{}, b{};
  if (!readSlot(prefs, kSlot0, a) || !readSlot(prefs, kSlot1, b)) {
    prefs.end();
    return false;
  }

  uint32_t next = 1U;
  const char* target = kSlot0;
  if (a.valid && b.valid) {
    const Slot& best = newestValid(a, b);
    next = best.sequence + 1U;
    target = (&best == &a) ? kSlot1 : kSlot0;
  } else if (a.valid) {
    next = a.sequence + 1U;
    target = kSlot1;
  } else if (b.valid) {
    next = b.sequence + 1U;
    target = kSlot0;
  } else if (a.present && !b.present) {
    target = kSlot1;
  }
  if (next == 0U) next = 1U;

  uint8_t encoded[kNarrativeSaveEncodedSize];
  if (!encodeNarrativeMemorySave(memory, next, encoded, sizeof(encoded))) {
    prefs.end();
    return false;
  }
  const size_t written = prefs.putBytes(target, encoded, sizeof(encoded));
  bool verified = false;
  if (written == sizeof(encoded) &&
      prefs.getBytesLength(target) == sizeof(encoded)) {
    uint8_t back[kNarrativeSaveEncodedSize];
    NarrativeMemory restored{};
    uint32_t restoredSequence = 0U;
    if (prefs.getBytes(target, back, sizeof(back)) == sizeof(back) &&
        memcmp(encoded, back, sizeof(encoded)) == 0 &&
        decodeNarrativeMemorySave(back, sizeof(back), restored,
                                  restoredSequence) &&
        restoredSequence == next) {
      verified = true;
    }
  }
  prefs.end();
  if (verified && sequence) *sequence = next;
  return verified;
}
}  // namespace hawkins
