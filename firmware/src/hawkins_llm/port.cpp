#include "port.h"

// Les pointeurs vers chaque entrée publique gardent le moteur réel dans le
// firmware lié. Les poids et le tokenizer LeCarnet sont embarqués par
// model_data.S ; runtime.cpp les initialise à l'entrée du jeu Hawkins.
// Cette table centralise les appels au moteur REZOR, sans copie ni stub.
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
