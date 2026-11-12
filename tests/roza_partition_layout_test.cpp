#include "roza_partition_layout.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void set_entry(roza_partition_layout_entry_t &entry, uint8_t type, uint8_t subtype,
                      uint32_t offset, uint32_t size, uint32_t flags, const char *label)
{
    entry = {};
    entry.type = type;
    entry.subtype = subtype;
    entry.offset = offset;
    entry.size = size;
    entry.flags = flags;
    const size_t length = strlen(label);
    memcpy(entry.label, label, length < sizeof(entry.label) ? length : sizeof(entry.label));
}

static void make_valid(roza_partition_layout_entry_t entries[ROZA_PARTITION_EXPECTED_COUNT])
{
    set_entry(entries[0], 0x01, 0x40, 0x009000, 0x005000, 0x00, "launcher_nvs_old");
    set_entry(entries[1], 0x01, 0x00, 0x00E000, 0x002000, 0x00, "otadata");
    set_entry(entries[2], 0x01, 0x01, 0x011000, 0x001000, 0x00, "phy_init");
    set_entry(entries[3], 0x01, 0x02, 0x013000, 0x006000, 0x00, "nvs");
    set_entry(entries[4], 0x01, 0x04, 0x019000, 0x001000, 0x01, "nvs_keys");
    set_entry(entries[5], 0x00, 0x10, 0x020000, 0x380000, 0x00, "ota_0");
    set_entry(entries[6], 0x00, 0x11, 0x3A0000, 0x380000, 0x00, "ota_1");
    set_entry(entries[7], 0x01, 0x83, 0x720000, 0x040000, 0x00, "nucleo_nap");
    set_entry(entries[8], 0x01, 0x82, 0x760000, 0x0A0000, 0x00, "cfg");
}

int main()
{
    roza_partition_layout_entry_t entries[ROZA_PARTITION_EXPECTED_COUNT];
    make_valid(entries);
    assert(roza_partition_layout_validate(entries, ROZA_PARTITION_EXPECTED_COUNT) == ROZA_PARTITION_LAYOUT_OK);

    // Entry order is not significant; labels, types, flags and geometry are authoritative.
    roza_partition_layout_entry_t shuffled[ROZA_PARTITION_EXPECTED_COUNT];
    for (size_t i = 0; i < ROZA_PARTITION_EXPECTED_COUNT; ++i)
        shuffled[i] = entries[ROZA_PARTITION_EXPECTED_COUNT - 1 - i];
    assert(roza_partition_layout_validate(shuffled, ROZA_PARTITION_EXPECTED_COUNT) == ROZA_PARTITION_LAYOUT_OK);

    make_valid(entries);
    entries[5].offset += 0x1000;
    assert(roza_partition_layout_validate(entries, ROZA_PARTITION_EXPECTED_COUNT) == ROZA_PARTITION_LAYOUT_MISMATCH);

    make_valid(entries);
    entries[6].offset = 0x30000;
    assert(roza_partition_layout_validate(entries, ROZA_PARTITION_EXPECTED_COUNT) == ROZA_PARTITION_LAYOUT_OVERLAP);

    make_valid(entries);
    memset(entries[8].label, 0, sizeof(entries[8].label));
    memcpy(entries[8].label, "ota_0", 5);
    assert(roza_partition_layout_validate(entries, ROZA_PARTITION_EXPECTED_COUNT) == ROZA_PARTITION_LAYOUT_DUPLICATE_LABEL);

    make_valid(entries);
    memset(entries[8].label, 0, sizeof(entries[8].label));
    memcpy(entries[8].label, "unexpected", 10);
    assert(roza_partition_layout_validate(entries, ROZA_PARTITION_EXPECTED_COUNT) == ROZA_PARTITION_LAYOUT_UNKNOWN_LABEL);

    make_valid(entries);
    entries[8].offset = 0x7F0000;
    entries[8].size = 0x20000;
    assert(roza_partition_layout_validate(entries, ROZA_PARTITION_EXPECTED_COUNT) == ROZA_PARTITION_LAYOUT_OUT_OF_FLASH);

    make_valid(entries);
    entries[4].flags = 0;
    assert(roza_partition_layout_validate(entries, ROZA_PARTITION_EXPECTED_COUNT) == ROZA_PARTITION_LAYOUT_MISMATCH);

    make_valid(entries);
    assert(roza_partition_layout_validate(entries, ROZA_PARTITION_EXPECTED_COUNT - 1) == ROZA_PARTITION_LAYOUT_INVALID_COUNT);
    assert(roza_partition_layout_validate(nullptr, ROZA_PARTITION_EXPECTED_COUNT) == ROZA_PARTITION_LAYOUT_INVALID_ARGUMENT);

    puts("ROZA partition layout: PASS");
    puts(" - exact ESP32-S3FN8 8 MiB layout accepted in any entry order");
    puts(" - mismatched geometry, overlaps, duplicates, unknown labels, out-of-flash ranges and wrong flags rejected");
    return 0;
}
