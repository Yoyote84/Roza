#include "runtime.h"

#include "llm.h"

#include <Arduino.h>
#include <esp_system.h>
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
constexpr int kPromptTokenLimit = 16;
constexpr int kMaxEncodedPromptTokens = 192;
constexpr size_t kDecodeScratchBytes = 128U;

Transformer transformer{};
Tokenizer tokenizer{};
Sampler sampler{};
bool initAttempted = false;
bool runtimeReady = false;

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

  memset(&transformer, 0, sizeof(transformer));
  memset(&tokenizer, 0, sizeof(tokenizer));
  memset(&sampler, 0, sizeof(sampler));

  if (!llm_init_embedded(&transformer, hawkins_model_start, modelSize,
                         kRuntimeContextLimit)) {
    llm_free(&transformer);
    Serial.printf("[hawkins-llm] en-tete ou allocation du modele invalide (%u octets)\n",
                  static_cast<unsigned>(modelSize));
    return false;
  }

  if (transformer.config.arch != ARCH_LLAMA ||
      transformer.config.vocab_size <= 0 ||
      transformer.config.seq_len < kRuntimeContextLimit) {
    llm_free(&transformer);
    Serial.println("[hawkins-llm] architecture, vocabulaire ou contexte incompatible");
    return false;
  }

  if (!llm_tokenizer_from_memory(&tokenizer, hawkins_tokenizer_start,
                                 tokenizerSize,
                                 transformer.config.vocab_size)) {
    llm_free(&transformer);
    Serial.println("[hawkins-llm] tokenizer CTK2 invalide");
    return false;
  }

  llm_build_sampler(&sampler, transformer.config.vocab_size,
                    0.80f, 0.92f,
                    static_cast<uint64_t>(esp_random()));
  runtimeReady = true;
  Serial.printf("[hawkins-llm] modele embarque pret: CRDP v3 LLaMA, vocab=%d, contexte=%d, poids=%u, tokenizer=%u\n",
                transformer.config.vocab_size, transformer.config.seq_len,
                static_cast<unsigned>(modelSize),
                static_cast<unsigned>(tokenizerSize));
  return true;
}

bool hawkinsLlmRuntimeReady() {
  return initAttempted && runtimeReady;
}

bool hawkinsLlmRuntimeGenerate(const char* prompt, char* output,
                              size_t outputCapacity,
                              unsigned maxNewTokens) {
  if (!output || outputCapacity == 0U) return false;
  output[0] = '\0';
  if (!hawkinsLlmRuntimeReady() || !prompt || prompt[0] == '\0') return false;

  int tokens[kMaxEncodedPromptTokens] = {};
  int tokenCount = 0;
  llm_encode(&tokenizer, prompt, 1, 0, tokens, &tokenCount);
  if (tokenCount < 2 || tokenCount > kMaxEncodedPromptTokens) return false;

  const int contextLimit =
      transformer.config.seq_len < kRuntimeContextLimit
          ? transformer.config.seq_len : kRuntimeContextLimit;
  if (contextLimit < 2) return false;

  const unsigned generationLimit =
      maxNewTokens < static_cast<unsigned>(contextLimit - 1)
          ? maxNewTokens : static_cast<unsigned>(contextLimit - 1);
  const int promptLimit = contextLimit - static_cast<int>(generationLimit);
  if (promptLimit < 2) return false;

  // Conserver le BOS et la fin du prompt, notamment le marqueur "Bot:".
  if (tokenCount > promptLimit) {
    const int keepTail = promptLimit - 1;
    memmove(tokens + 1, tokens + tokenCount - keepTail,
            static_cast<size_t>(keepTail) * sizeof(tokens[0]));
    tokens[0] = tokenizer.eos_id;
    tokenCount = promptLimit;
  }

  float* logits = nullptr;
  for (int position = 0; position < tokenCount; ++position) {
    logits = llm_forward(&transformer, tokens[position], position);
    if (!logits) return false;
  }

  int previous = tokens[tokenCount - 1];
  int position = tokenCount;
  char piece[kDecodeScratchBytes];
  for (unsigned generated = 0U;
       generated < generationLimit && position < contextLimit;
       ++generated) {
    const int next = llm_sample(&sampler, logits);
    if (next < 0 || next >= tokenizer.vocab_size) break;
    if (next == tokenizer.eos_id) break;

    piece[0] = '\0';
    const char* decoded = llm_decode(&tokenizer, previous, next,
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
    logits = llm_forward(&transformer, next, position - 1);
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
