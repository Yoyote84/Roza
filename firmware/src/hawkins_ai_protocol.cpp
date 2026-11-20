#include "hawkins_ai_protocol.h"

#include <ArduinoJson.h>
#include <string.h>

namespace hawkins {
namespace {
constexpr char kPrefix[] = "HKA/1 ";
constexpr size_t kPrefixLength = sizeof(kPrefix) - 1U;

bool copyBoundedText(const char* source, char* destination, size_t capacity) {
  if (!source || !destination || capacity == 0U) return false;
  const size_t length = strnlen(source, capacity);
  if (length == 0U || length >= capacity) return false;
  for (size_t i = 0U; i < length; ++i) {
    const unsigned char c = static_cast<unsigned char>(source[i]);
    if (c < 0x20U && c != '\n' && c != '\t') return false;
  }
  memcpy(destination, source, length);
  destination[length] = '\0';
  return true;
}
}  // namespace

bool buildHawkinsAiRequestLine(uint32_t requestId, const char* contextJson,
                               char* output, size_t capacity) {
  if (!output || capacity < kPrefixLength + 3U || requestId == 0U ||
      !contextJson || contextJson[0] == '\0') return false;
  output[0] = '\0';

  JsonDocument context;
  const DeserializationError contextError = deserializeJson(context, contextJson);
  if (contextError || !context.is<JsonObject>()) return false;

  JsonDocument envelope;
  envelope["type"] = "narrate";
  envelope["id"] = requestId;
  envelope["context"].set(context.as<JsonObjectConst>());

  const size_t jsonLength = measureJson(envelope);
  if (jsonLength == 0U ||
      kPrefixLength + jsonLength + 2U > capacity ||
      kPrefixLength + jsonLength + 1U > kHawkinsAiMaxLine) return false;

  memcpy(output, kPrefix, kPrefixLength);
  const size_t written = serializeJson(envelope, output + kPrefixLength,
                                       capacity - kPrefixLength);
  if (written != jsonLength) {
    output[0] = '\0';
    return false;
  }
  output[kPrefixLength + written] = '\n';
  output[kPrefixLength + written + 1U] = '\0';
  return true;
}

bool parseHawkinsAiMessage(const char* line, size_t length,
                           HawkinsAiMessage& output) {
  output = HawkinsAiMessage{};
  if (!line || length < kPrefixLength + 2U ||
      length > kHawkinsAiMaxLine ||
      memcmp(line, kPrefix, kPrefixLength) != 0) return false;

  JsonDocument document;
  const DeserializationError error =
      deserializeJson(document, line + kPrefixLength, length - kPrefixLength);
  if (error || !document.is<JsonObject>()) return false;

  const char* type = document["type"].as<const char*>();
  if (!type) return false;
  if (strcmp(type, "hello") == 0) {
    const uint8_t version = document["version"] | 0U;
    if (version != 1U) return false;
    output.kind = HawkinsAiMessageKind::Hello;
    return true;
  }

  if (!document["id"].is<uint32_t>()) return false;
  const uint32_t id = document["id"].as<uint32_t>();
  if (id == 0U) return false;

  if (strcmp(type, "narration") == 0) {
    const char* text = document["text"].as<const char*>();
    if (!copyBoundedText(text, output.text, sizeof(output.text)) ||
        !document["choices"].is<JsonArray>()) return false;
    JsonArray choices = document["choices"].as<JsonArray>();
    if (choices.size() > kHawkinsAiChoiceCount) return false;
    for (JsonVariantConst choice : choices) {
      if (output.choiceCount >= kHawkinsAiChoiceCount ||
          !copyBoundedText(choice.as<const char*>(),
                           output.choices[output.choiceCount],
                           sizeof(output.choices[0]))) return false;
      ++output.choiceCount;
    }
    output.kind = HawkinsAiMessageKind::Narration;
    output.requestId = id;
    return true;
  }

  if (strcmp(type, "error") == 0) {
    const char* message = document["message"].as<const char*>();
    if (!copyBoundedText(message, output.text,
                         kHawkinsAiErrorCapacity + 1U)) return false;
    output.kind = HawkinsAiMessageKind::Error;
    output.requestId = id;
    return true;
  }
  return false;
}
}  // namespace hawkins
