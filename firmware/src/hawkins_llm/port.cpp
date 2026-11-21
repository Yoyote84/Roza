#include "port.h"

// Les pointeurs vers chaque entrée publique gardent le moteur réel dans le
// firmware lié. Les poids et le tokenizer exportés à l'étape 3 seront raccordés
// par l'application Hawkins à l'étape 4.
// Aucun modèle n'est chargé ici : aucune allocation du cache KV n'est faite.
const HawkinsLlmApi& hawkinsLlmGetApi() {
  static const HawkinsLlmApi api = {
      &llm_init_embedded,
      &llm_tokenizer_from_memory,
      &llm_build_sampler,
      &llm_forward,
      &llm_forward_at,
      &llm_kv_slide,
      &llm_sample,
      &llm_encode,
      &llm_decode,
  };
  return api;
}
