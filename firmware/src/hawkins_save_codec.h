#pragma once
#include "hawkins_world.h"
#include <stddef.h>
#include <stdint.h>
namespace hawkins {
constexpr uint16_t kWorldSaveFormatVersion = 1;
constexpr size_t kWorldSaveHeaderSize = 16;
constexpr size_t kWorldSavePayloadSize = 42;
constexpr size_t kWorldSaveEncodedSize = kWorldSaveHeaderSize + kWorldSavePayloadSize;
bool encodeWorldSave(const WorldState&, uint32_t, uint8_t*, size_t);
bool decodeWorldSave(const uint8_t*, size_t, WorldState&, uint32_t&);
bool isValidWorldState(const WorldState&);
}  // namespace hawkins
