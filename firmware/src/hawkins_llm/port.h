#pragma once

#include "llm.h"

// Table de fonctions du moteur embarqué REZOR.
// Les poids et le tokenizer seront fournis par l'étape 3 ; cette table expose
// le vrai moteur à Hawkins sans charger de modèle ni réserver sa mémoire.
struct HawkinsLlmApi {
  decltype(&llm_init_embedded) initEmbedded;
  decltype(&llm_tokenizer_from_memory) tokenizerFromMemory;
  decltype(&llm_build_sampler) buildSampler;
  decltype(&llm_forward) forward;
  decltype(&llm_forward_at) forwardAt;
  decltype(&llm_kv_slide) slideContext;
  decltype(&llm_sample) sample;
  decltype(&llm_encode) encode;
  decltype(&llm_decode) decode;
};

const HawkinsLlmApi& hawkinsLlmGetApi();
