#pragma once

#include "llm.h"

// Table de fonctions du moteur embarqué REZOR.
// Les poids et le tokenizer finaux sont livrés à la fin de l'étape 3.
// L'étape 4 les embarque et utilise cette table pour initialiser le moteur
// local ; obtenir l'API ne charge pas le modèle ni ne réserve sa mémoire.
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
