#include "hawkins_ai_protocol.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

using hawkins::HawkinsAiMessage;
using hawkins::HawkinsAiMessageKind;

static void testRequestBuildsBoundedEnvelope() {
  char line[1024];
  assert(hawkins::buildHawkinsAiRequestLine(
      42U, "{\"player\":{\"name\":\"ELEVEN\"},\"day\":3}", line, sizeof(line)));
  assert(strncmp(line, "HKA/1 ", 6U) == 0);
  assert(strstr(line, "\"type\":\"narrate\"") != nullptr);
  assert(strstr(line, "\"id\":42") != nullptr);
  assert(strstr(line, "\"player\":{\"name\":\"ELEVEN\"}") != nullptr);
  assert(line[strlen(line) - 1U] == '\n');
}

static void testRequestRejectsInvalidInputAndOverflow() {
  char small[24];
  assert(!hawkins::buildHawkinsAiRequestLine(0U, "{}", small, sizeof(small)));
  assert(!hawkins::buildHawkinsAiRequestLine(1U, "[]", small, sizeof(small)));
  assert(!hawkins::buildHawkinsAiRequestLine(1U, "{", small, sizeof(small)));
  char large[256];
  assert(!hawkins::buildHawkinsAiRequestLine(
      1U, "{\"very_long\":\"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz\"}",
      large, 24U));
}

static void testHelloHeartbeat() {
  const char line[] = "HKA/1 {\"type\":\"hello\",\"version\":1}";
  HawkinsAiMessage message{};
  assert(hawkins::parseHawkinsAiMessage(line, sizeof(line) - 1U, message));
  assert(message.kind == HawkinsAiMessageKind::Hello);
  assert(message.requestId == 0U && message.text[0] == '\0');
}

static void testNarrationAndChoices() {
  const char line[] =
      "HKA/1 {\"type\":\"narration\",\"id\":7,\"text\":\"La lumiere vacille.\","
      "\"choices\":[\"Suivre le bruit\",\"Appeler Lia\",\"Se cacher\"]}";
  HawkinsAiMessage message{};
  assert(hawkins::parseHawkinsAiMessage(line, sizeof(line) - 1U, message));
  assert(message.kind == HawkinsAiMessageKind::Narration);
  assert(message.requestId == 7U);
  assert(strcmp(message.text, "La lumiere vacille.") == 0);
  assert(message.choiceCount == 3U);
  assert(strcmp(message.choices[1], "Appeler Lia") == 0);
}

static void testInvalidAndUnknownMessages() {
  HawkinsAiMessage message{};
  const char noPrefix[] = "{\"type\":\"hello\",\"version\":1}";
  assert(!hawkins::parseHawkinsAiMessage(noPrefix, sizeof(noPrefix) - 1U, message));
  const char wrongVersion[] = "HKA/1 {\"type\":\"hello\",\"version\":2}";
  assert(!hawkins::parseHawkinsAiMessage(wrongVersion, sizeof(wrongVersion) - 1U, message));
  const char noId[] = "HKA/1 {\"type\":\"narration\",\"text\":\"Salut\",\"choices\":[]}";
  assert(!hawkins::parseHawkinsAiMessage(noId, sizeof(noId) - 1U, message));
  const char unknown[] = "HKA/1 {\"type\":\"inventory_change\",\"id\":1}";
  assert(!hawkins::parseHawkinsAiMessage(unknown, sizeof(unknown) - 1U, message));
  const char malformed[] = "HKA/1 {\"type\":";
  assert(!hawkins::parseHawkinsAiMessage(malformed, sizeof(malformed) - 1U, message));
}

static void testRejectsEmptyAndOversizedNarration() {
  HawkinsAiMessage message{};
  const char empty[] =
      "HKA/1 {\"type\":\"narration\",\"id\":3,\"text\":\"\",\"choices\":[]}";
  assert(!hawkins::parseHawkinsAiMessage(empty, sizeof(empty) - 1U, message));

  char line[700];
  size_t offset = static_cast<size_t>(
      snprintf(line, sizeof(line),
               "HKA/1 {\"type\":\"narration\",\"id\":3,\"text\":\""));
  for (unsigned i = 0U; i < 301U; ++i) line[offset++] = 'x';
  line[offset++] = '"';
  const char suffix[] = ",\"choices\":[]}";
  memcpy(line + offset, suffix, sizeof(suffix) - 1U);
  offset += sizeof(suffix) - 1U;
  assert(!hawkins::parseHawkinsAiMessage(line, offset, message));
}

static void testRejectsControlCharactersAndExcessChoices() {
  const char control[] =
      "HKA/1 {\"type\":\"narration\",\"id\":2,\"text\":\"ok\\u0001no\",\"choices\":[]}";
  HawkinsAiMessage message{};
  assert(!hawkins::parseHawkinsAiMessage(control, sizeof(control) - 1U, message));
  const char tooMany[] =
      "HKA/1 {\"type\":\"narration\",\"id\":2,\"text\":\"ok\","
      "\"choices\":[\"1\",\"2\",\"3\",\"4\"]}";
  assert(!hawkins::parseHawkinsAiMessage(tooMany, sizeof(tooMany) - 1U, message));
}

int main() {
  testRequestBuildsBoundedEnvelope();
  testRequestRejectsInvalidInputAndOverflow();
  testHelloHeartbeat();
  testNarrationAndChoices();
  testInvalidAndUnknownMessages();
  testRejectsEmptyAndOversizedNarration();
  testRejectsControlCharactersAndExcessChoices();
  puts("HAWKINS AI USB: toutes les validations du protocole ont reussi.");
  return 0;
}
