#include "llm.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

int main() {
  testRejectsShortModelBeforeReadingPointer();
  testRejectsWrongMagic();
  testRejectsUnsupportedModelVersion();
  puts("HAWKINS LLM : moteur compilé, garde-fous d'en-tête validés.");
  return 0;
}
