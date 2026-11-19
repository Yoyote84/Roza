#include "hawkins_nvs.h"

#include "globals.h"
#include "hawkins_save_codec.h"
#include <Preferences.h>
#include <string.h>

namespace hawkins {
namespace {

const char* kNamespace = "hawkins";
const char* kWorldSlot0 = "slot0";
const char* kWorldSlot1 = "slot1";
const char* kExploreSlot0 = "expl0";
const char* kExploreSlot1 = "expl1";
const char* kCharacterSlot0 = "char0";
const char* kCharacterSlot1 = "char1";

template <typename State>
struct Slot {
  bool present;
  bool valid;
  uint32_t sequence;
  State state;
};

bool sequenceNewer(uint32_t a, uint32_t b) {
  return static_cast<int32_t>(a - b) > 0;
}

bool openStore() {
  return prefs.begin(kNamespace, false);
}

bool readWorldSlot(Preferences& store, const char* key, Slot<WorldState>& slot) {
  slot = Slot<WorldState>{};
  const size_t length = store.getBytesLength(key);
  if (length == 0U) return true;
  slot.present = true;
  if (length != kWorldSaveEncodedSize) return true;
  uint8_t bytes[kWorldSaveEncodedSize];
  if (store.getBytes(key, bytes, length) != length) return false;
  slot.valid = decodeWorldSave(bytes, length, slot.state, slot.sequence);
  return true;
}

bool readExploreSlot(Preferences& store, const char* key,
                     Slot<ExplorationState>& slot) {
  slot = Slot<ExplorationState>{};
  const size_t length = store.getBytesLength(key);
  if (length == 0U) return true;
  slot.present = true;
  if (length != kExplorationSaveEncodedSize) return true;
  uint8_t bytes[kExplorationSaveEncodedSize];
  if (store.getBytes(key, bytes, length) != length) return false;
  slot.valid = decodeExplorationSave(bytes, length, slot.state, slot.sequence);
  return true;
}

bool readCharacterSlot(Preferences& store, const char* key,
                       Slot<CharacterState>& slot) {
  slot = Slot<CharacterState>{};
  const size_t length = store.getBytesLength(key);
  if (length == 0U) return true;
  slot.present = true;
  if (length != kCharacterSaveEncodedSize) return true;
  uint8_t bytes[kCharacterSaveEncodedSize];
  if (store.getBytes(key, bytes, length) != length) return false;
  slot.valid = decodeCharacterSave(bytes, length, slot.state, slot.sequence);
  return true;
}

template <typename State>
const Slot<State>& newestValid(const Slot<State>& a, const Slot<State>& b) {
  if (!a.valid) return b;
  if (!b.valid) return a;
  return sequenceNewer(a.sequence, b.sequence) ? a : b;
}

}  // namespace

WorldSaveLoadStatus worldSaveLoad(WorldState& world, uint32_t* sequence) {
  PrefsLock lock;
  if (!openStore()) return WorldSaveLoadStatus::StorageError;
  Slot<WorldState> a{}, b{};
  const bool ok = readWorldSlot(prefs, kWorldSlot0, a) &&
                  readWorldSlot(prefs, kWorldSlot1, b);
  prefs.end();
  if (!ok) return WorldSaveLoadStatus::StorageError;
  if (!a.valid && !b.valid) {
    return (a.present || b.present) ? WorldSaveLoadStatus::Corrupt
                                   : WorldSaveLoadStatus::Empty;
  }
  const Slot<WorldState>& best = newestValid(a, b);
  world = best.state;
  if (sequence) *sequence = best.sequence;
  return WorldSaveLoadStatus::Loaded;
}

bool worldSaveCommit(const WorldState& world, uint32_t* sequence) {
  if (!isValidWorldState(world)) return false;
  PrefsLock lock;
  if (!openStore()) return false;
  Slot<WorldState> a{}, b{};
  if (!readWorldSlot(prefs, kWorldSlot0, a) ||
      !readWorldSlot(prefs, kWorldSlot1, b)) {
    prefs.end();
    return false;
  }
  uint32_t next = 1U;
  const char* target = kWorldSlot0;
  if (a.valid && b.valid) {
    const Slot<WorldState>& best = newestValid(a, b);
    next = best.sequence + 1U;
    target = (&best == &a) ? kWorldSlot1 : kWorldSlot0;
  } else if (a.valid) {
    next = a.sequence + 1U;
    target = kWorldSlot1;
  } else if (b.valid) {
    next = b.sequence + 1U;
    target = kWorldSlot0;
  } else if (a.present && !b.present) {
    target = kWorldSlot1;
  }
  if (next == 0U) next = 1U;
  uint8_t encoded[kWorldSaveEncodedSize];
  if (!encodeWorldSave(world, next, encoded, sizeof(encoded))) {
    prefs.end();
    return false;
  }
  const size_t written = prefs.putBytes(target, encoded, sizeof(encoded));
  bool verified = false;
  if (written == sizeof(encoded) &&
      prefs.getBytesLength(target) == sizeof(encoded)) {
    uint8_t back[kWorldSaveEncodedSize];
    WorldState restored{};
    uint32_t restoredSequence = 0U;
    if (prefs.getBytes(target, back, sizeof(back)) == sizeof(back) &&
        memcmp(encoded, back, sizeof(encoded)) == 0 &&
        decodeWorldSave(back, sizeof(back), restored, restoredSequence) &&
        restoredSequence == next) {
      verified = true;
    }
  }
  prefs.end();
  if (verified && sequence) *sequence = next;
  return verified;
}

ExplorationSaveLoadStatus explorationSaveLoad(ExplorationState& state,
                                               uint32_t* sequence) {
  PrefsLock lock;
  if (!openStore()) return ExplorationSaveLoadStatus::StorageError;
  Slot<ExplorationState> a{}, b{};
  const bool ok = readExploreSlot(prefs, kExploreSlot0, a) &&
                  readExploreSlot(prefs, kExploreSlot1, b);
  prefs.end();
  if (!ok) return ExplorationSaveLoadStatus::StorageError;
  if (!a.valid && !b.valid) {
    return (a.present || b.present) ? ExplorationSaveLoadStatus::Corrupt
                                   : ExplorationSaveLoadStatus::Empty;
  }
  const Slot<ExplorationState>& best = newestValid(a, b);
  state = best.state;
  if (sequence) *sequence = best.sequence;
  return ExplorationSaveLoadStatus::Loaded;
}

bool explorationSaveCommit(const ExplorationState& state, uint32_t* sequence) {
  if (!isValidExplorationState(state)) return false;
  PrefsLock lock;
  if (!openStore()) return false;
  Slot<ExplorationState> a{}, b{};
  if (!readExploreSlot(prefs, kExploreSlot0, a) ||
      !readExploreSlot(prefs, kExploreSlot1, b)) {
    prefs.end();
    return false;
  }
  uint32_t next = 1U;
  const char* target = kExploreSlot0;
  if (a.valid && b.valid) {
    const Slot<ExplorationState>& best = newestValid(a, b);
    next = best.sequence + 1U;
    target = (&best == &a) ? kExploreSlot1 : kExploreSlot0;
  } else if (a.valid) {
    next = a.sequence + 1U;
    target = kExploreSlot1;
  } else if (b.valid) {
    next = b.sequence + 1U;
    target = kExploreSlot0;
  } else if (a.present && !b.present) {
    target = kExploreSlot1;
  }
  if (next == 0U) next = 1U;
  uint8_t encoded[kExplorationSaveEncodedSize];
  if (!encodeExplorationSave(state, next, encoded, sizeof(encoded))) {
    prefs.end();
    return false;
  }
  const size_t written = prefs.putBytes(target, encoded, sizeof(encoded));
  bool verified = false;
  if (written == sizeof(encoded) &&
      prefs.getBytesLength(target) == sizeof(encoded)) {
    uint8_t back[kExplorationSaveEncodedSize];
    ExplorationState restored{};
    uint32_t restoredSequence = 0U;
    if (prefs.getBytes(target, back, sizeof(back)) == sizeof(back) &&
        memcmp(encoded, back, sizeof(encoded)) == 0 &&
        decodeExplorationSave(back, sizeof(back), restored, restoredSequence) &&
        restoredSequence == next) {
      verified = true;
    }
  }
  prefs.end();
  if (verified && sequence) *sequence = next;
  return verified;
}

CharacterSaveLoadStatus characterSaveLoad(CharacterState& character,
                                           uint32_t* sequence) {
  PrefsLock lock;
  if (!openStore()) return CharacterSaveLoadStatus::StorageError;
  Slot<CharacterState> a{}, b{};
  const bool ok = readCharacterSlot(prefs, kCharacterSlot0, a) &&
                  readCharacterSlot(prefs, kCharacterSlot1, b);
  prefs.end();
  if (!ok) return CharacterSaveLoadStatus::StorageError;
  if (!a.valid && !b.valid) {
    return (a.present || b.present) ? CharacterSaveLoadStatus::Corrupt
                                   : CharacterSaveLoadStatus::Empty;
  }
  const Slot<CharacterState>& best = newestValid(a, b);
  character = best.state;
  if (sequence) *sequence = best.sequence;
  return CharacterSaveLoadStatus::Loaded;
}

bool characterSaveCommit(const CharacterState& character, uint32_t* sequence) {
  if (!isValidCharacter(character)) return false;
  PrefsLock lock;
  if (!openStore()) return false;
  Slot<CharacterState> a{}, b{};
  if (!readCharacterSlot(prefs, kCharacterSlot0, a) ||
      !readCharacterSlot(prefs, kCharacterSlot1, b)) {
    prefs.end();
    return false;
  }
  uint32_t next = 1U;
  const char* target = kCharacterSlot0;
  if (a.valid && b.valid) {
    const Slot<CharacterState>& best = newestValid(a, b);
    next = best.sequence + 1U;
    target = (&best == &a) ? kCharacterSlot1 : kCharacterSlot0;
  } else if (a.valid) {
    next = a.sequence + 1U;
    target = kCharacterSlot1;
  } else if (b.valid) {
    next = b.sequence + 1U;
    target = kCharacterSlot0;
  } else if (a.present && !b.present) {
    target = kCharacterSlot1;
  }
  if (next == 0U) next = 1U;
  uint8_t encoded[kCharacterSaveEncodedSize];
  if (!encodeCharacterSave(character, next, encoded, sizeof(encoded))) {
    prefs.end();
    return false;
  }
  const size_t written = prefs.putBytes(target, encoded, sizeof(encoded));
  bool verified = false;
  if (written == sizeof(encoded) &&
      prefs.getBytesLength(target) == sizeof(encoded)) {
    uint8_t back[kCharacterSaveEncodedSize];
    CharacterState restored{};
    uint32_t restoredSequence = 0U;
    if (prefs.getBytes(target, back, sizeof(back)) == sizeof(back) &&
        memcmp(encoded, back, sizeof(encoded)) == 0 &&
        decodeCharacterSave(back, sizeof(back), restored, restoredSequence) &&
        restoredSequence == next) {
      verified = true;
    }
  }
  prefs.end();
  if (verified && sequence) *sequence = next;
  return verified;
}

}  // namespace hawkins
