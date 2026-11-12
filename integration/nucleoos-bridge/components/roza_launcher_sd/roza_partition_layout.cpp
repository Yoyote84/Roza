#include "roza_partition_layout.h"

#include <string.h>

namespace {
struct ExpectedPartition {
    const char *label;
    uint8_t type;
    uint8_t subtype;
    uint32_t offset;
    uint32_t size;
    uint32_t flags;
};

constexpr ExpectedPartition kExpected[ROZA_PARTITION_EXPECTED_COUNT] = {
    {"launcher_nvs_old", 0x01, 0x40, 0x009000, 0x005000, 0x00},
    {"otadata",          0x01, 0x00, 0x00E000, 0x002000, 0x00},
    {"phy_init",         0x01, 0x01, 0x011000, 0x001000, 0x00},
    {"nvs",              0x01, 0x02, 0x013000, 0x006000, 0x00},
    {"nvs_keys",         0x01, 0x04, 0x019000, 0x001000, 0x01},
    {"ota_0",             0x00, 0x10, 0x020000, 0x380000, 0x00},
    {"ota_1",             0x00, 0x11, 0x3A0000, 0x380000, 0x00},
    {"nucleo_nap",       0x01, 0x83, 0x720000, 0x040000, 0x00},
    {"cfg",              0x01, 0x82, 0x760000, 0x0A0000, 0x00},
};

size_t label_length(const char label[ROZA_PARTITION_LABEL_SIZE])
{
    size_t length = 0;
    while (length < ROZA_PARTITION_LABEL_SIZE && label[length] != '\0') ++length;
    return length;
}

int expected_index(const char *label, size_t length)
{
    for (size_t i = 0; i < ROZA_PARTITION_EXPECTED_COUNT; ++i) {
        const size_t expected_length = strlen(kExpected[i].label);
        if (length == expected_length && memcmp(label, kExpected[i].label, length) == 0)
            return static_cast<int>(i);
    }
    return -1;
}
} // namespace

extern "C" roza_partition_layout_status_t roza_partition_layout_validate(
    const roza_partition_layout_entry_t *entries,
    size_t count)
{
    if (!entries) return ROZA_PARTITION_LAYOUT_INVALID_ARGUMENT;
    if (count != ROZA_PARTITION_EXPECTED_COUNT) return ROZA_PARTITION_LAYOUT_INVALID_COUNT;

    bool seen[ROZA_PARTITION_EXPECTED_COUNT] = {};
    uint64_t ends[ROZA_PARTITION_EXPECTED_COUNT] = {};

    for (size_t i = 0; i < count; ++i) {
        const size_t length = label_length(entries[i].label);
        if (length == 0) return ROZA_PARTITION_LAYOUT_INVALID_LABEL;

        const int index = expected_index(entries[i].label, length);
        if (index < 0) return ROZA_PARTITION_LAYOUT_UNKNOWN_LABEL;
        if (seen[index]) return ROZA_PARTITION_LAYOUT_DUPLICATE_LABEL;
        seen[index] = true;

        const uint64_t end = static_cast<uint64_t>(entries[i].offset) + entries[i].size;
        if (entries[i].size == 0 || end > ROZA_PARTITION_FLASH_BYTES)
            return ROZA_PARTITION_LAYOUT_OUT_OF_FLASH;
        ends[i] = end;

        for (size_t j = 0; j < i; ++j) {
            if (entries[i].offset < ends[j] && entries[j].offset < end)
                return ROZA_PARTITION_LAYOUT_OVERLAP;
        }

        const ExpectedPartition &expected = kExpected[index];
        if (entries[i].type != expected.type ||
            entries[i].subtype != expected.subtype ||
            entries[i].offset != expected.offset ||
            entries[i].size != expected.size ||
            entries[i].flags != expected.flags)
            return ROZA_PARTITION_LAYOUT_MISMATCH;
    }

    for (bool present : seen) {
        if (!present) return ROZA_PARTITION_LAYOUT_MISSING_LABEL;
    }
    return ROZA_PARTITION_LAYOUT_OK;
}

extern "C" const char *roza_partition_layout_status_message(roza_partition_layout_status_t status)
{
    switch (status) {
        case ROZA_PARTITION_LAYOUT_OK: return "table ROZA 8 Mio validee";
        case ROZA_PARTITION_LAYOUT_INVALID_ARGUMENT: return "table de partitions absente";
        case ROZA_PARTITION_LAYOUT_INVALID_COUNT: return "nombre de partitions ROZA inattendu";
        case ROZA_PARTITION_LAYOUT_INVALID_LABEL: return "label de partition invalide";
        case ROZA_PARTITION_LAYOUT_DUPLICATE_LABEL: return "label de partition duplique";
        case ROZA_PARTITION_LAYOUT_UNKNOWN_LABEL: return "partition inconnue dans la table ROZA";
        case ROZA_PARTITION_LAYOUT_OUT_OF_FLASH: return "partition hors des 8 Mio";
        case ROZA_PARTITION_LAYOUT_OVERLAP: return "chevauchement de partitions";
        case ROZA_PARTITION_LAYOUT_MISMATCH: return "geometrie de partition ROZA incompatible";
        case ROZA_PARTITION_LAYOUT_MISSING_LABEL: return "partition ROZA manquante";
        default: return "erreur de table de partitions";
    }
}
