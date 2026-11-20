#pragma once

#include <stddef.h>
#include <stdint.h>

namespace hawkins {

constexpr size_t kHawkinsAiMaxLine = 1792U;
constexpr size_t kHawkinsAiTextCapacity = 480U;
constexpr size_t kHawkinsAiErrorCapacity = 160U;

enum class HawkinsAiMessageKind : uint8_t { Invalid = 0, Hello, Narration, Error };

struct HawkinsAiMessage {
  HawkinsAiMessageKind kind;
  uint32_t requestId;
  char text[kHawkinsAiTextCapacity + 1U];
};

bool buildHawkinsAiRequestLine(uint32_t requestId, const char* contextJson,
                               char* output, size_t capacity);
bool parseHawkinsAiMessage(const char* line, size_t length,
                           HawkinsAiMessage& output);

}  // namespace hawkins
