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

static void testNarration() {
  const char line[] =
      "HKA/1 {\"type\":\"narration\",\"id\":7,\"text\":\"La lumiere vacille.\",\"choices\":[\"Repondre\",\"Se cacher\"]}";
  HawkinsAiMessage message{};
  assert(hawkins::parseHawkinsAiMessage(line, sizeof(line) - 1U, message));
  assert(message.kind == HawkinsAiMessageKind::Narration);
  assert(message.requestId == 7U);
  assert(message.choiceCount == 2U);
  assert(strcmp(message.choices[0], "Repondre") == 0);
  assert(strcmp(message.choices[1], "Se cacher") == 0);
  assert(strcmp(message.text, "La lumiere vacille.") == 0);
}

static void testNarrationAcceptsFrenchUtf8() {
  const char line[] =
      "HKA/1 {\"type\":\"narration\",\"id\":1,\"text\":\"La lumiere vacille dans le couloir. Quelqu\u0027un frappe a la porte.\",\"choices\":[\"Repondre a la radio\",\"Se cacher\",\"Aller voir\"]}";
  HawkinsAiMessage message{};
  assert(hawkins::parseHawkinsAiMessage(line, sizeof(line) - 1U, message));
  assert(message.kind == HawkinsAiMessageKind::Narration);
  assert(message.requestId == 1U);
  assert(message.choiceCount == 3U);
  assert(strcmp(message.text, "La lumiere vacille dans le couloir. Quelqu'un frappe a la porte.") == 0);
}

static void testNarrationNormalizesControlCharacters() {
  const char line[] =
      "HKA/1 {\"type\":\"narration\",\"id\":9,\"text\":\"Ligne 1\\rLigne 2\",\"choices\":[\"A\\u0001B\"]}";
  HawkinsAiMessage message{};
  const char* errorCode = nullptr;
  assert(hawkins::parseHawkinsAiMessage(
      line, sizeof(line) - 1U, message, &errorCode));
  assert(strcmp(errorCode, "ok") == 0);
  assert(strcmp(message.text, "Ligne 1 Ligne 2") == 0);
  assert(strcmp(message.choices[0], "A B") == 0);
}

static void testNarrationExposesPreciseParseFailure() {
  const char line[] =
      "HKA/1 {\"type\":\"narration\",\"id\":9,\"text\":\"Bonjour\"}";
  HawkinsAiMessage message{};
  const char* errorCode = nullptr;
  assert(!hawkins::parseHawkinsAiMessage(
      line, sizeof(line) - 1U, message, &errorCode));
  assert(strcmp(errorCode, "choices_missing_or_not_array") == 0);
}

static void testInvalidAndUnknownMessages() {
  HawkinsAiMessage message{};
  const char noPrefix[] = "{\"type\":\"hello\",\"version\":1}";
  assert(!hawkins::parseHawkinsAiMessage(noPrefix, sizeof(noPrefix) - 1U, message));
  const char wrongVersion[] = "HKA/1 {\"type\":\"hello\",\"version\":2}";
  assert(!hawkins::parseHawkinsAiMessage(wrongVersion, sizeof(wrongVersion) - 1U, message));
  const char noId[] = "HKA/1 {\"type\":\"narration\",\"text\":\"Salut\"}";
  assert(!hawkins::parseHawkinsAiMessage(noId, sizeof(noId) - 1U, message));
  const char unknown[] = "HKA/1 {\"type\":\"inventory_change\",\"id\":1}";
  assert(!hawkins::parseHawkinsAiMessage(unknown, sizeof(unknown) - 1U, message));
  const char malformed[] = "HKA/1 {\"type\":";
  assert(!hawkins::parseHawkinsAiMessage(malformed, sizeof(malformed) - 1U, message));
}

static void testRejectsEmptyAndOversizedNarration() {
  HawkinsAiMessage message{};
  const char empty[] = "HKA/1 {\"type\":\"narration\",\"id\":3,\"text\":\"\"}";
  assert(!hawkins::parseHawkinsAiMessage(empty, sizeof(empty) - 1U, message));

  char line[700];
  size_t offset = static_cast<size_t>(
      snprintf(line, sizeof(line), "HKA/1 {\"type\":\"narration\",\"id\":3,\"text\":\""));
  for (unsigned i = 0U; i < 500U; ++i) line[offset++] = 'x';
  line[offset++] = '"';
  line[offset++] = '}';
  assert(!hawkins::parseHawkinsAiMessage(line, offset, message));
}

static void testRejectsControlCharacters() {
  const char line[] =
      "HKA/1 {\"type\":\"narration\",\"id\":2,\"text\":\"ok\\u0001no\"}";
  HawkinsAiMessage message{};
  assert(!hawkins::parseHawkinsAiMessage(line, sizeof(line) - 1U, message));
}

int main() {
  testRequestBuildsBoundedEnvelope();
  testRequestRejectsInvalidInputAndOverflow();
  testHelloHeartbeat();
  testNarration();
  testNarrationAcceptsFrenchUtf8();
  testNarrationNormalizesControlCharacters();
  testNarrationExposesPreciseParseFailure();
  testInvalidAndUnknownMessages();
  testRejectsEmptyAndOversizedNarration();
  testRejectsControlCharacters();
  puts("HAWKINS AI USB: toutes les validations du protocole ont reussi.");
  return 0;
}
