#include "hawkins_adventure_store.h"

#include "globals.h"
#include <Preferences.h>
#include <string.h>

namespace hawkins {
namespace {
constexpr char kNamespace[] = "hawkins";
constexpr char kSlot0[] = "adv0";
constexpr char kSlot1[] = "adv1";

struct Slot {
  bool present;
  bool valid;
  uint32_t sequence;
  AdventureState state;
};

bool sequenceNewer(uint32_t a, uint32_t b) {
  return static_cast<int32_t>(a - b) > 0;
}

bool readSlot(Preferences& store, const char* key, Slot& slot) {
  slot = Slot{};
  const size_t length = store.getBytesLength(key);
  if (length == 0U) return true;
  slot.present = true;
  if (length != kAdventureSaveEncodedSize) return true;
  uint8_t bytes[kAdventureSaveEncodedSize];
  if (store.getBytes(key, bytes, length) != length) return false;
  slot.valid = decodeAdventureSave(bytes, length, slot.state, slot.sequence);
  return true;
}

const Slot& newestValid(const Slot& a, const Slot& b) {
  if (!a.valid) return b;
  if (!b.valid) return a;
  return sequenceNewer(a.sequence, b.sequence) ? a : b;
}
}  // namespace

AdventureSaveLoadStatus adventureSaveLoad(AdventureState& state, uint32_t* sequence) {
  PrefsLock lock;
  if (!prefs.begin(kNamespace, false)) return AdventureSaveLoadStatus::StorageError;
  Slot a{}, b{};
  const bool ok = readSlot(prefs, kSlot0, a) && readSlot(prefs, kSlot1, b);
  prefs.end();
  if (!ok) return AdventureSaveLoadStatus::StorageError;
  if (!a.valid && !b.valid) {
    return (a.present || b.present) ? AdventureSaveLoadStatus::Corrupt
                                    : AdventureSaveLoadStatus::Empty;
  }
  const Slot& best = newestValid(a, b);
  state = best.state;
  if (sequence) *sequence = best.sequence;
  return AdventureSaveLoadStatus::Loaded;
}

bool adventureSaveCommit(const AdventureState& state, uint32_t* sequence) {
  if (!isValidAdventureState(state)) return false;
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
  uint8_t encoded[kAdventureSaveEncodedSize];
  if (!encodeAdventureSave(state, next, encoded, sizeof(encoded))) {
    prefs.end();
    return false;
  }
  const size_t written = prefs.putBytes(target, encoded, sizeof(encoded));
  bool verified = false;
  if (written == sizeof(encoded) && prefs.getBytesLength(target) == sizeof(encoded)) {
    uint8_t back[kAdventureSaveEncodedSize];
    AdventureState restored{};
    uint32_t restoredSequence = 0U;
    if (prefs.getBytes(target, back, sizeof(back)) == sizeof(back) &&
        memcmp(encoded, back, sizeof(encoded)) == 0 &&
        decodeAdventureSave(back, sizeof(back), restored, restoredSequence) &&
        restoredSequence == next) verified = true;
  }
  prefs.end();
  if (verified && sequence) *sequence = next;
  return verified;
}

}  // namespace hawkins
