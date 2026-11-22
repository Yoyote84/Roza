#include "runtime.h"

#include "port.h"

#include <Arduino.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern "C" {
extern const uint8_t hawkins_model_start[];
extern const uint8_t hawkins_model_end[];
extern const uint8_t hawkins_tokenizer_start[];
extern const uint8_t hawkins_tokenizer_end[];
}

namespace hawkins {
namespace {

constexpr int kRuntimeContextLimit = 32;
constexpr int kMaxEncodedPromptTokens = 192;
constexpr size_t kMaxPromptBytes = kMaxEncodedPromptTokens - 1U;
constexpr size_t kDecodeScratchBytes = 128U;

Transformer transformer{};
Tokenizer tokenizer{};
Sampler sampler{};
bool initAttempted = false;
bool runtimeReady = false;

void logInternalHeap(const char* phase) {
  const uint32_t capabilities = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
  const size_t freeBytes = heap_caps_get_free_size(capabilities);
  const size_t largestBlock = heap_caps_get_largest_free_block(capabilities);
  Serial.printf("[hawkins-llm] HEAP %s : libre=%u octets, bloc_max=%u octets\n",
                phase, static_cast<unsigned>(freeBytes),
                static_cast<unsigned>(largestBlock));
}

size_t safeUtf8Length(const char* text, size_t length) {
  if (!text || length == 0U) return 0U;
  size_t start = length;
  while (start > 0U &&
         (static_cast<unsigned char>(text[start - 1U]) & 0xC0U) == 0x80U) {
    --start;
  }

  if (start == length) {
    const unsigned char last = static_cast<unsigned char>(text[length - 1U]);
    if ((last & 0xE0U) == 0xC0U) return length - 1U;
    if ((last & 0xF0U) == 0xE0U) return length - 1U;
    if ((last & 0xF8U) == 0xF0U) return length - 1U;
    return length;
  }
  if (start == 0U) return 0U;

  const unsigned char lead = static_cast<unsigned char>(text[start - 1U]);
  size_t expected = 1U;
  if ((lead & 0xE0U) == 0xC0U) expected = 2U;
  else if ((lead & 0xF0U) == 0xE0U) expected = 3U;
  else if ((lead & 0xF8U) == 0xF0U) expected = 4U;
  if (length - (start - 1U) < expected) return start - 1U;
  return length;
}

// Encode exactement le prompt fourni, sans couper son début ou sa fin.
// Le tokenizer CTK2 produit au plus un token par octet, plus le BOS.
bool encodeBoundedPrompt(const char* prompt, int* tokens, int& tokenCount) {
  tokenCount = 0;
  if (!prompt || !tokens) return false;
  const size_t promptLength = strnlen(prompt, kMaxPromptBytes + 1U);
  if (promptLength == 0U || promptLength > kMaxPromptBytes ||
      safeUtf8Length(prompt, promptLength) != promptLength) return false;

  char boundedPrompt[kMaxPromptBytes + 1U] = {};
  memcpy(boundedPrompt, prompt, promptLength);
  boundedPrompt[promptLength] = '\0';

  const HawkinsLlmApi& api = hawkinsLlmGetApi();
  if (!api.encode) return false;
  api.encode(&tokenizer, boundedPrompt, 1, 0, tokens, &tokenCount);
  return tokenCount >= 2 && tokenCount <= kMaxEncodedPromptTokens;
}

int runtimeContextLimit() {
  return transformer.config.seq_len < kRuntimeContextLimit
      ? transformer.config.seq_len : kRuntimeContextLimit;
}

unsigned runtimeGenerationLimit(unsigned requested) {
  const int contextLimit = runtimeContextLimit();
  if (contextLimit < 2) return 0U;
  const unsigned maximum = static_cast<unsigned>(contextLimit - 1);
  return requested < maximum ? requested : maximum;
}

}  // namespace

bool hawkinsLlmRuntimeInit() {
  if (initAttempted) return runtimeReady;
  initAttempted = true;

  const uintptr_t modelBegin = reinterpret_cast<uintptr_t>(hawkins_model_start);
  const uintptr_t modelEnd = reinterpret_cast<uintptr_t>(hawkins_model_end);
  const uintptr_t tokenizerBegin =
      reinterpret_cast<uintptr_t>(hawkins_tokenizer_start);
  const uintptr_t tokenizerEnd =
      reinterpret_cast<uintptr_t>(hawkins_tokenizer_end);

  if (modelEnd <= modelBegin || tokenizerEnd <= tokenizerBegin) {
    Serial.println("[hawkins-llm] fichiers embarques absents ou vides");
    return false;
  }

  const size_t modelSize = static_cast<size_t>(modelEnd - modelBegin);
  const size_t tokenizerSize =
      static_cast<size_t>(tokenizerEnd - tokenizerBegin);

  // Prévalider l'en-tête CRDP avant que llm_init_embedded() ne calcule les
  // strides. Un binaire absent ou mal formé ne doit jamais provoquer une
  // division par zéro ni des tailles d'allocation non bornées.
  if (modelSize < 64U || memcmp(hawkins_model_start, "CRDP", 4U) != 0) {
    Serial.println("[hawkins-llm] en-tete CRDP absent ou trop court");
    return false;
  }
  uint32_t formatVersion = 0U;
  int dimensions[7] = {};
  memcpy(&formatVersion, hawkins_model_start + 4U, sizeof(formatVersion));
  memcpy(dimensions, hawkins_model_start + 8U, sizeof(dimensions));
  const uint8_t sharedClassifier = hawkins_model_start[36U];
  const uint8_t quantType = hawkins_model_start[37U];
  const uint8_t modelArch = hawkins_model_start[38U];
  const int dim = dimensions[0];
  const int hidden = dimensions[1];
  const int layers = dimensions[2];
  const int heads = dimensions[3];
  const int kvHeads = dimensions[4];
  const int vocab = dimensions[5];
  const int context = dimensions[6];
  if (formatVersion != 3U || modelArch != ARCH_LLAMA ||
      quantType != 4U || sharedClassifier != 1U ||
      dim <= 0 || dim > 512 || hidden <= 0 || hidden > 1024 ||
      layers <= 0 || layers > 16 || heads <= 0 || heads > 16 ||
      kvHeads != heads || dim % heads != 0 || dim % 32 != 0 ||
      hidden % 32 != 0 || vocab <= 0 || vocab > 16000 ||
      context < kRuntimeContextLimit || context > 64) {
    Serial.println("[hawkins-llm] dimensions CRDP incompatibles avec le profil Cardputer");
    return false;
  }

  memset(&transformer, 0, sizeof(transformer));
  memset(&tokenizer, 0, sizeof(tokenizer));
  memset(&sampler, 0, sizeof(sampler));

  const HawkinsLlmApi& api = hawkinsLlmGetApi();
  if (!api.initEmbedded || !api.tokenizerFromMemory || !api.buildSampler ||
      !api.forward || !api.sample || !api.encode || !api.decode) {
    Serial.println("[hawkins-llm] table API du moteur incomplete");
    return false;
  }

  logInternalHeap("avant modele");
  if (!api.initEmbedded(&transformer, hawkins_model_start, modelSize,
                        kRuntimeContextLimit)) {
    llm_free(&transformer);
    Serial.printf("[hawkins-llm] en-tete ou allocation du modele invalide (%u octets)\n",
                  static_cast<unsigned>(modelSize));
    return false;
  }

  logInternalHeap("apres poids");
  if (transformer.config.arch != ARCH_LLAMA ||
      transformer.config.vocab_size <= 0 ||
      transformer.config.seq_len < kRuntimeContextLimit) {
    llm_free(&transformer);
    Serial.println("[hawkins-llm] architecture, vocabulaire ou contexte incompatible");
    return false;
  }

  if (!api.tokenizerFromMemory(&tokenizer, hawkins_tokenizer_start,
                               tokenizerSize,
                               transformer.config.vocab_size)) {
    llm_free(&transformer);
    Serial.println("[hawkins-llm] tokenizer CTK2 invalide");
    return false;
  }

  api.buildSampler(&sampler, transformer.config.vocab_size,
                   0.80f, 0.92f,
                    static_cast<uint64_t>(esp_random()));
  runtimeReady = true;
  logInternalHeap("IA prete");
  Serial.printf("[hawkins-llm] modele embarque pret: CRDP v3 LLaMA, vocab=%d, contexte=%d, poids=%u, tokenizer=%u\n",
                transformer.config.vocab_size, transformer.config.seq_len,
                static_cast<unsigned>(modelSize),
                static_cast<unsigned>(tokenizerSize));
  return true;
}

bool hawkinsLlmRuntimeReady() {
  return initAttempted && runtimeReady;
}

bool hawkinsLlmRuntimePromptFits(const char* prompt, unsigned maxNewTokens) {
  if (!hawkinsLlmRuntimeReady()) return false;
  int tokens[kMaxEncodedPromptTokens] = {};
  int tokenCount = 0;
  if (!encodeBoundedPrompt(prompt, tokens, tokenCount)) return false;
  const int contextLimit = runtimeContextLimit();
  const unsigned generationLimit = runtimeGenerationLimit(maxNewTokens);
  const int promptLimit = contextLimit - static_cast<int>(generationLimit);
  return promptLimit >= 2 && tokenCount <= promptLimit;
}

bool hawkinsLlmRuntimeGenerate(const char* prompt, char* output,
                              size_t outputCapacity,
                              unsigned maxNewTokens) {
  if (!output || outputCapacity == 0U) return false;
  output[0] = '\0';
  if (!hawkinsLlmRuntimeReady() || !prompt || prompt[0] == '\0') return false;

  const HawkinsLlmApi& api = hawkinsLlmGetApi();
  int tokens[kMaxEncodedPromptTokens] = {};
  int tokenCount = 0;
  if (!encodeBoundedPrompt(prompt, tokens, tokenCount)) return false;

  const int contextLimit = runtimeContextLimit();
  const unsigned generationLimit = runtimeGenerationLimit(maxNewTokens);
  const int promptLimit = contextLimit - static_cast<int>(generationLimit);
  if (contextLimit < 2 || promptLimit < 2 || tokenCount > promptLimit) {
    Serial.printf("[hawkins-llm] prompt refuse : %d tokens, maximum %d (generation %u)\n",
                  tokenCount, promptLimit, generationLimit);
    return false;
  }

  float* logits = nullptr;
  for (int position = 0; position < tokenCount; ++position) {
    logits = api.forward(&transformer, tokens[position], position);
    if (!logits) return false;
  }

  int previous = tokens[tokenCount - 1];
  int position = tokenCount;
  char piece[kDecodeScratchBytes];
  for (unsigned generated = 0U;
       generated < generationLimit && position < contextLimit;
       ++generated) {
    const int next = api.sample(&sampler, logits);
    if (next < 0 || next >= tokenizer.vocab_size) break;
    if (next == tokenizer.eos_id) break;

    piece[0] = '\0';
    const char* decoded = api.decode(&tokenizer, previous, next,
                                     piece, sizeof(piece));
    if (decoded && decoded[0] != '\0') {
      const size_t used = strlen(output);
      const size_t available = outputCapacity - used - 1U;
      const size_t pieceLength = strnlen(decoded, sizeof(piece));
      const size_t copyLength =
          pieceLength < available ? pieceLength : available;
      if (copyLength > 0U) {
        memcpy(output + used, decoded, copyLength);
        output[used + copyLength] = '\0';
      }
      if (copyLength < pieceLength || outputCapacity - strlen(output) <= 1U)
        break;
    }

    tokens[position] = next;
    ++position;
    previous = next;
    if (position >= contextLimit) break;
    logits = api.forward(&transformer, next, position - 1);
    if (!logits) break;
  }

  size_t outputLength = strlen(output);
  outputLength = safeUtf8Length(output, outputLength);
  output[outputLength] = '\0';
  while (outputLength > 0U &&
         (output[outputLength - 1U] == ' ' ||
          output[outputLength - 1U] == '\r' ||
          output[outputLength - 1U] == '\n' ||
          output[outputLength - 1U] == '\t')) {
    output[--outputLength] = '\0';
  }
  size_t first = 0U;
  while (output[first] == ' ' || output[first] == '\r' ||
         output[first] == '\n' || output[first] == '\t') ++first;
  if (first > 0U) {
    memmove(output, output + first, outputLength - first + 1U);
    outputLength -= first;
  }
  return outputLength > 0U;
}

}  // namespace hawkins
