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

static void testTokenizerRejectsInvalidInputsAndMetadata() {
  Tokenizer tk{};
  assert(!llm_tokenizer_from_memory(nullptr, nullptr, 0U, 1));
  assert(!llm_tokenizer_from_memory(&tk, nullptr, 0U, 1));
  uint8_t shortBlob[3] = {};
  assert(!llm_tokenizer_from_memory(&tk, shortBlob, sizeof(shortBlob), 1));
  uint32_t tooLarge = 999999U;
  uint8_t llamaHeader[4] = {};
  memcpy(llamaHeader, &tooLarge, sizeof(tooLarge));
  assert(!llm_tokenizer_from_memory(&tk, llamaHeader, sizeof(llamaHeader), 10));

  std::vector<uint8_t> ctk(20 + 512, 0U);
  const uint32_t ctkMagic = 0x324B5443U;
  memcpy(ctk.data(), &ctkMagic, 4U);
  int32_t v=256, mtl=32, eos=0, nm=-1;
  memcpy(ctk.data()+4,&v,4U); memcpy(ctk.data()+8,&mtl,4U);
  memcpy(ctk.data()+12,&eos,4U); memcpy(ctk.data()+16,&nm,4U);
  assert(!llm_tokenizer_from_memory(&tk,ctk.data(),ctk.size(),v));
  nm=1000;
  memcpy(ctk.data()+16,&nm,4U);
  assert(!llm_tokenizer_from_memory(&tk,ctk.data(),ctk.size(),v));
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
