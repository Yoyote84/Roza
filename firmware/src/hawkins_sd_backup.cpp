#include "hawkins_sd_backup.h"

#include "globals.h"
#include "sd_files.h"
#include "hawkins_nvs.h"
#include "hawkins_adventure_store.h"
#include "hawkins_save_codec.h"
#include "hawkins_adventure.h"

#include <string.h>

namespace hawkins {
namespace {

constexpr char kDirectory[] = "/HAWKINS";
constexpr char kSlot0Path[] = "/HAWKINS/SLOT0.HWS";
constexpr char kSlot1Path[] = "/HAWKINS/SLOT1.HWS";
constexpr char kTempPath[] = "/HAWKINS/WRITING.TMP";
constexpr uint32_t kArchiveMagic = 0x44535748U;  // "HWSD"
constexpr uint16_t kArchiveVersion = 1U;
constexpr uint16_t kHeaderSize = 20U;
constexpr size_t kCrcOffset = 16U;
constexpr uint8_t kRecordCount = 5U;
// Archive actuelle ~764 octets avec la mémoire de 128 secteurs ; 800 octets
// laissent une petite marge tout en gardant les buffers bornés.
constexpr size_t kArchiveCapacity = 800U;

struct ArchiveView {
  uint32_t sequence;
  uint16_t length[kRecordCount];
  const uint8_t* records[kRecordCount];
};

bool autoEnabled = false;
bool initialized = false;
uint32_t currentSequence = 0U;
const char* statusText = "COPIE SD NON CONFIGUREE";

void write16(uint8_t* p, uint16_t value) {
  p[0] = static_cast<uint8_t>(value);
  p[1] = static_cast<uint8_t>(value >> 8U);
}
void write32(uint8_t* p, uint32_t value) {
  for (unsigned i = 0; i < 4U; ++i) p[i] = static_cast<uint8_t>(value >> (8U * i));
}
uint16_t read16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0]) |
         static_cast<uint16_t>(static_cast<uint16_t>(p[1]) << 8U);
}
uint32_t read32(const uint8_t* p) {
  uint32_t value = 0U;
  for (unsigned i = 0; i < 4U; ++i) value |= static_cast<uint32_t>(p[i]) << (8U * i);
  return value;
}
uint32_t crc32(const uint8_t* data, size_t length) {
  uint32_t crc = 0xFFFFFFFFU;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (unsigned bit = 0; bit < 8U; ++bit) {
      const uint32_t mask = 0U - (crc & 1U);
      crc = (crc >> 1U) ^ (0xEDB88320U & mask);
    }
  }
  return ~crc;
}
bool sequenceNewer(uint32_t a, uint32_t b) {
  return static_cast<int32_t>(a - b) > 0;
}

bool validateRecords(const ArchiveView& view) {
  if (view.length[0] != kWorldSaveEncodedSize ||
      (view.length[1] != kExplorationSaveEncodedSize &&
       view.length[1] != kLegacyExplorationSaveEncodedSize) ||
      view.length[2] != kCharacterSaveEncodedSize ||
      view.length[3] != kMissionSaveEncodedSize ||
      view.length[4] != kAdventureSaveEncodedSize) return false;
  WorldState world{};
  ExplorationState exploration{};
  CharacterState character{};
  MissionState missions{};
  AdventureState adventure{};
  uint32_t sequence = 0U;
  return decodeWorldSave(view.records[0], view.length[0], world, sequence) &&
         decodeExplorationSave(view.records[1], view.length[1], exploration, sequence) &&
         decodeCharacterSave(view.records[2], view.length[2], character, sequence) &&
         decodeMissionSave(view.records[3], view.length[3], missions, sequence) &&
         decodeAdventureSave(view.records[4], view.length[4], adventure, sequence);
}

bool parseArchive(const uint8_t* bytes, size_t length, ArchiveView& view) {
  if (!bytes || length < kHeaderSize + kRecordCount * 2U || length > kArchiveCapacity ||
      read32(bytes) != kArchiveMagic || read16(bytes + 4U) != kArchiveVersion ||
      read16(bytes + 6U) != kHeaderSize || read32(bytes + 8U) == 0U ||
      read32(bytes + 12U) != length - kHeaderSize) return false;
  uint8_t check[kArchiveCapacity];
  memcpy(check, bytes, length);
  const uint32_t storedCrc = read32(check + kCrcOffset);
  write32(check + kCrcOffset, 0U);
  if (crc32(check, length) != storedCrc) return false;

  view = ArchiveView{};
  view.sequence = read32(bytes + 8U);
  size_t offset = kHeaderSize;
  for (uint8_t i = 0U; i < kRecordCount; ++i) {
    if (offset + 2U > length) return false;
    view.length[i] = read16(bytes + offset);
    offset += 2U;
    if (offset + view.length[i] > length) return false;
    view.records[i] = bytes + offset;
    offset += view.length[i];
  }
  return offset == length && validateRecords(view);
}

bool readSlotFile(const char* path, uint8_t* bytes, size_t& length,
                  ArchiveView& view, bool& exists) {
  exists = SD.exists(path);
  length = 0U;
  if (!exists) return true;
  File file = SD.open(path, FILE_READ);
  if (!file) return false;
  const size_t fileSize = static_cast<size_t>(file.size());
  if (fileSize > kArchiveCapacity || fileSize < kHeaderSize) {
    file.close();
    return false;
  }
  const size_t read = file.read(bytes, fileSize);
  file.close();
  if (read != fileSize) return false;
  length = fileSize;
  return parseArchive(bytes, length, view);
}

bool appendRecord(uint8_t* output, size_t& offset,
                  const uint8_t* record, uint16_t length) {
  if (offset + 2U + length > kArchiveCapacity) return false;
  write16(output + offset, length);
  offset += 2U;
  if (length > 0U) memcpy(output + offset, record, length);
  offset += length;
  return true;
}

bool makeArchive(uint8_t* output, size_t& length, uint32_t sequence) {
  uint8_t worldBytes[kWorldSaveEncodedSize];
  uint8_t explorationBytes[kExplorationSaveEncodedSize];
  uint8_t characterBytes[kCharacterSaveEncodedSize];
  uint8_t missionBytes[kMissionSaveEncodedSize];
  uint8_t adventureBytes[kAdventureSaveEncodedSize];
  WorldState world{};
  ExplorationState exploration{};
  CharacterState character{};
  MissionState missions{};
  AdventureState adventure{};
  uint32_t worldSequence = 0U, explorationSequence = 0U;
  uint32_t characterSequence = 0U, missionSequence = 0U, adventureSequence = 0U;

  const WorldSaveLoadStatus worldStatus = worldSaveLoad(world, &worldSequence);
  const ExplorationSaveLoadStatus explorationStatus =
      explorationSaveLoad(exploration, &explorationSequence);
  const CharacterSaveLoadStatus characterStatus =
      characterSaveLoad(character, &characterSequence);
  const MissionSaveLoadStatus missionStatus = missionSaveLoad(missions, &missionSequence);
  const AdventureSaveLoadStatus adventureStatus =
      adventureSaveLoad(adventure, &adventureSequence);

  // Une copie n'est jamais créée à partir de données manquantes ou invalides.
  // Un profil joueur est obligatoire pour garantir une sauvegarde complète.
  if (worldStatus != WorldSaveLoadStatus::Loaded ||
      explorationStatus != ExplorationSaveLoadStatus::Loaded ||
      characterStatus != CharacterSaveLoadStatus::Loaded ||
      missionStatus != MissionSaveLoadStatus::Loaded ||
      adventureStatus != AdventureSaveLoadStatus::Loaded) return false;

  if (!encodeWorldSave(world, worldSequence, worldBytes, sizeof(worldBytes)) ||
      !encodeExplorationSave(exploration, explorationSequence,
                             explorationBytes, sizeof(explorationBytes)) ||
      !encodeCharacterSave(character, characterSequence,
                           characterBytes, sizeof(characterBytes)) ||
      !encodeMissionSave(missions, missionSequence, missionBytes, sizeof(missionBytes)) ||
      !encodeAdventureSave(adventure, adventureSequence,
                           adventureBytes, sizeof(adventureBytes))) return false;

  memset(output, 0, kHeaderSize);
  write32(output, kArchiveMagic);
  write16(output + 4U, kArchiveVersion);
  write16(output + 6U, kHeaderSize);
  write32(output + 8U, sequence);
  size_t offset = kHeaderSize;
  if (!appendRecord(output, offset, worldBytes, sizeof(worldBytes)) ||
      !appendRecord(output, offset, explorationBytes, sizeof(explorationBytes)) ||
      !appendRecord(output, offset, characterBytes, sizeof(characterBytes)) ||
      !appendRecord(output, offset, missionBytes, sizeof(missionBytes)) ||
      !appendRecord(output, offset, adventureBytes, sizeof(adventureBytes))) return false;
  length = offset;
  write32(output + 12U, static_cast<uint32_t>(length - kHeaderSize));
  write32(output + kCrcOffset, 0U);
  write32(output + kCrcOffset, crc32(output, length));
  return true;
}

bool writeVerifiedSlot(const char* targetPath, const uint8_t* bytes, size_t length) {
  if (SD.exists(kTempPath) && !SD.remove(kTempPath)) return false;
  File output = SD.open(kTempPath, FILE_WRITE);
  if (!output) return false;
  const size_t written = output.write(bytes, length);
  output.flush();
  output.close();
  if (written != length) {
    SD.remove(kTempPath);
    return false;
  }

  uint8_t verify[kArchiveCapacity];
  ArchiveView view{};
  size_t verifyLength = 0U;
  bool exists = false;
  if (!readSlotFile(kTempPath, verify, verifyLength, view, exists) ||
      !exists || verifyLength != length || memcmp(verify, bytes, length) != 0) {
    SD.remove(kTempPath);
    return false;
  }
  // targetPath est toujours le plus ancien emplacement ; l'autre reste intact.
  if (SD.exists(targetPath) && !SD.remove(targetPath)) {
    SD.remove(kTempPath);
    return false;
  }
  if (!SD.rename(kTempPath, targetPath)) {
    SD.remove(kTempPath);
    return false;
  }
  return true;
}

bool readBestArchive(uint8_t* bytes, size_t& length, ArchiveView& bestView) {
  if (!sdMounted) return false;
  uint8_t aBytes[kArchiveCapacity], bBytes[kArchiveCapacity];
  size_t aLength = 0U, bLength = 0U;
  ArchiveView aView{}, bView{};
  bool aExists = false, bExists = false;
  const bool aOk = readSlotFile(kSlot0Path, aBytes, aLength, aView, aExists);
  const bool bOk = readSlotFile(kSlot1Path, bBytes, bLength, bView, bExists);
  if (aOk && aExists && bOk && bExists) {
    const bool chooseA = sequenceNewer(aView.sequence, bView.sequence);
    const uint8_t* source = chooseA ? aBytes : bBytes;
    length = chooseA ? aLength : bLength;
    memcpy(bytes, source, length);
    return parseArchive(bytes, length, bestView);
  }
  if (aOk && aExists) {
    length = aLength;
    memcpy(bytes, aBytes, length);
    return parseArchive(bytes, length, bestView);
  }
  if (bOk && bExists) {
    length = bLength;
    memcpy(bytes, bBytes, length);
    return parseArchive(bytes, length, bestView);
  }
  return false;
}

}  // namespace

void sdBackupInit() {
  autoEnabled = loadBool("hawkins", "sdauto", false);
  initialized = true;
  currentSequence = 0U;
  if (sdMounted) {
    uint8_t archive[kArchiveCapacity];
    size_t length = 0U;
    ArchiveView view{};
    if (readBestArchive(archive, length, view)) currentSequence = view.sequence;
  }
  statusText = autoEnabled ? "COPIE AUTOMATIQUE ACTIVE" : "COPIE SD DESACTIVEE";
}

bool sdBackupEnabled() {
  if (!initialized) sdBackupInit();
  return autoEnabled;
}

bool sdBackupSaveNow() {
  if (!sdMounted) {
    statusText = "CARTE SD ABSENTE";
    return false;
  }
  if (!SD.exists(kDirectory) && !SD.mkdir(kDirectory)) {
    statusText = "DOSSIER HAWKINS IMPOSSIBLE";
    return false;
  }

  uint8_t aBytes[kArchiveCapacity], bBytes[kArchiveCapacity];
  size_t aLength = 0U, bLength = 0U;
  ArchiveView aView{}, bView{};
  bool aExists = false, bExists = false;
  const bool aOk = readSlotFile(kSlot0Path, aBytes, aLength, aView, aExists);
  const bool bOk = readSlotFile(kSlot1Path, bBytes, bLength, bView, bExists);
  if ((aExists && !aOk) && (bExists && !bOk)) {
    statusText = "DEUX SAUVEGARDES SD INVALIDES";
    return false;
  }

  uint32_t next = 1U;
  const char* target = kSlot0Path;
  if (aOk && aExists && bOk && bExists) {
    const bool aNewer = sequenceNewer(aView.sequence, bView.sequence);
    next = (aNewer ? aView.sequence : bView.sequence) + 1U;
    target = aNewer ? kSlot1Path : kSlot0Path;
  } else if (aOk && aExists) {
    next = aView.sequence + 1U;
    target = kSlot1Path;
  } else if (bOk && bExists) {
    next = bView.sequence + 1U;
    target = kSlot0Path;
  } else if (aExists && !aOk) {
    target = kSlot0Path;
  } else if (bExists && !bOk) {
    target = kSlot1Path;
  }
  if (next == 0U) next = 1U;

  uint8_t archive[kArchiveCapacity];
  size_t archiveLength = 0U;
  if (!makeArchive(archive, archiveLength, next) ||
      !writeVerifiedSlot(target, archive, archiveLength)) {
    statusText = "ECHEC COPIE SD - NVS CONSERVEE";
    return false;
  }
  currentSequence = next;
  statusText = "SAUVEGARDE SD VERIFIEE";
  return true;
}

bool sdBackupSetEnabled(bool enabled) {
  if (!initialized) sdBackupInit();
  if (enabled) {
    if (!sdMounted || !sdBackupSaveNow()) return false;
  }
  saveBool("hawkins", "sdauto", enabled);
  autoEnabled = enabled;
  statusText = enabled ? "COPIE AUTOMATIQUE ACTIVE" : "COPIE AUTOMATIQUE DESACTIVEE";
  return true;
}

bool sdBackupAutoSave() {
  if (!sdBackupEnabled()) return true;
  return sdBackupSaveNow();
}

bool sdBackupRestore() {
  if (!sdMounted) {
    statusText = "CARTE SD ABSENTE";
    return false;
  }
  uint8_t archive[kArchiveCapacity];
  size_t length = 0U;
  ArchiveView view{};
  if (!readBestArchive(archive, length, view)) {
    statusText = "AUCUNE COPIE SD VALIDE";
    return false;
  }

  WorldState world{};
  ExplorationState exploration{};
  CharacterState character{};
  MissionState missions{};
  AdventureState adventure{};
  uint32_t sequence = 0U;
  if (!decodeWorldSave(view.records[0], view.length[0], world, sequence) ||
      !decodeExplorationSave(view.records[1], view.length[1], exploration, sequence) ||
      !decodeCharacterSave(view.records[2], view.length[2], character, sequence) ||
      !decodeMissionSave(view.records[3], view.length[3], missions, sequence) ||
      !decodeAdventureSave(view.records[4], view.length[4], adventure, sequence)) {
    statusText = "FORMAT DE COPIE INVALIDE";
    return false;
  }

  // Tous les cinq blocs sont décodés et leurs CRC contrôlés avant la première écriture.
  const bool restored = worldSaveCommit(world) &&
      explorationSaveCommit(exploration) &&
      characterSaveCommit(character) && missionSaveCommit(missions) &&
      adventureSaveCommit(adventure);
  if (!restored) {
    statusText = "RESTAURATION PARTIELLE - VERIFIER NVS";
    return false;
  }
  currentSequence = view.sequence;
  statusText = "RESTAURATION SD TERMINEE";
  return true;
}

uint32_t sdBackupSequence() {
  return currentSequence;
}

const char* sdBackupStatusText() {
  return statusText;
}

}  // namespace hawkins
