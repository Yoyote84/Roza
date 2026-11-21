#include "port.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

static void testRejectsShortModelBeforeReadingPointer() {
  Transformer transformer{};
  assert(!llm_init_embedded(&transformer, nullptr, 0U, 8));
}

static void testRejectsWrongMagic() {
  Transformer transformer{};
  uint8_t model[64] = {};
  memcpy(model, "NOPE", 4U);
  assert(!llm_init_embedded(&transformer, model, sizeof(model), 8));
}

static void testRejectsUnsupportedModelVersion() {
  Transformer transformer{};
  uint8_t model[64] = {};
  memcpy(model, "CRDP", 4U);
  const uint32_t unsupportedVersion = 2U;
  memcpy(model + 4U, &unsupportedVersion, sizeof(unsupportedVersion));
  assert(!llm_init_embedded(&transformer, model, sizeof(model), 8));
}

static void testAllEngineEntrypointsAreLinked() {
  const HawkinsLlmApi& api = hawkinsLlmGetApi();
  assert(api.initEmbedded != nullptr);
  assert(api.tokenizerFromMemory != nullptr);
  assert(api.buildSampler != nullptr);
  assert(api.forward != nullptr);
  assert(api.forwardAt != nullptr);
  assert(api.slideContext != nullptr);
  assert(api.sample != nullptr);
  assert(api.encode != nullptr);
  assert(api.decode != nullptr);
}

int main() {
  testRejectsShortModelBeforeReadingPointer();
  testRejectsWrongMagic();
  testRejectsUnsupportedModelVersion();
  testAllEngineEntrypointsAreLinked();
  testTokenizerRejectsInvalidInputsAndMetadata();
  puts("HAWKINS LLM : moteur compilé, garde-fous d'en-tête validés.");
  return 0;
}
