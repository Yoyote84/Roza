#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ROZA_PARTITION_FLASH_BYTES 0x800000u
#define ROZA_PARTITION_EXPECTED_COUNT 9u
#define ROZA_PARTITION_LABEL_SIZE 16u

typedef struct {
    uint8_t type;
    uint8_t subtype;
    uint32_t offset;
    uint32_t size;
    uint32_t flags;
    char label[ROZA_PARTITION_LABEL_SIZE];
} roza_partition_layout_entry_t;

typedef enum {
    ROZA_PARTITION_LAYOUT_OK = 0,
    ROZA_PARTITION_LAYOUT_INVALID_ARGUMENT,
    ROZA_PARTITION_LAYOUT_INVALID_COUNT,
    ROZA_PARTITION_LAYOUT_INVALID_LABEL,
    ROZA_PARTITION_LAYOUT_DUPLICATE_LABEL,
    ROZA_PARTITION_LAYOUT_UNKNOWN_LABEL,
    ROZA_PARTITION_LAYOUT_OUT_OF_FLASH,
    ROZA_PARTITION_LAYOUT_OVERLAP,
    ROZA_PARTITION_LAYOUT_MISMATCH,
    ROZA_PARTITION_LAYOUT_MISSING_LABEL
} roza_partition_layout_status_t;

roza_partition_layout_status_t roza_partition_layout_validate(
    const roza_partition_layout_entry_t *entries,
    size_t count);

const char *roza_partition_layout_status_message(roza_partition_layout_status_t status);

#ifdef __cplusplus
}
#endif
