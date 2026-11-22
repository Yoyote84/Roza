#pragma once

#include <stddef.h>

namespace hawkins {

// Charge une seule fois les poids CRDP et le tokenizer CTK2 depuis la Flash
// mappée. Aucun accès SD, USB, Wi-Fi ou réseau n'est nécessaire.
bool hawkinsLlmRuntimeInit();
bool hawkinsLlmRuntimeReady();

// Génère une narration courte en français. Appel à effectuer depuis une
// tâche FreeRTOS dédiée : l'inférence est bloquante pendant le calcul.
bool hawkinsLlmRuntimeGenerate(const char* prompt, char* output,
                              size_t outputCapacity,
                              unsigned maxNewTokens = 16U);

}  // namespace hawkins
