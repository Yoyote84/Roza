# Moteur IA embarqué — port ROZA

Ce dossier contient le moteur d'inférence C++ issu de [therezor/cardputer-ai](https://github.com/therezor/cardputer-ai), adapté à l'arborescence PlatformIO active de ROZA.

## Source et licence

- Dépôt amont : https://github.com/therezor/cardputer-ai
- Révision copiée : `78c5128672b750977619dc0a6c3b8baed91168ed`
- Fichiers importés : `llm.cpp`, `llm.h`, `dot_q4_pie.S`
- Licence du code moteur : MIT, voir `LICENSE`.
- Attribution et provenance du moteur : REZOR, 2026.

## Portage

Le moteur utilise les API fournies par l'environnement ROZA :
- `esp_heap_caps.h` pour l'allocation SRAM interne ;
- FreeRTOS pour le worker de calcul sur le second cœur ;
- les instructions PIE SIMD de l'ESP32-S3, via `dot_q4_pie.S`.

La version ROZA reste PlatformIO + Arduino-ESP32 2.0.17 / ESP-IDF 4.4.7. Aucune migration globale vers l'ESP-IDF amont 5.5.4 n'est effectuée.

## Modèle final de l'étape 3

Le modèle français LeCarnet-3M a été fine-tuné et exporté. Les deux fichiers définitifs destinés à l'intégration de l'étape 4 sont :

- `model_lecarnet_q4.bin` : poids CRDP v3 LLaMA Q4_0 row-planar ;
- `tokenizer_lecarnet_ctk2.bin` : tokenizer CTK2 GPT-2 byte-level BPE.

Ils sont livrés dans l'artefact `HAWKINS-ETAPE3-MODELE-FR-FINAL.zip`, avec manifeste, métriques, provenance et SHA-256. Les fichiers ne doivent pas être copiés sur la microSD pour l'inférence : l'étape 4 les embarquera dans l'image applicative et les lira depuis la Flash mappée.

## Liaison au jeu

`hawkins_app.cpp` expose déjà le contexte du monde, du personnage, des missions, des objets et de la mémoire narrative. L'étape 4 doit remplacer le chemin de narration compagnon USB par le moteur local embarqué.

Le modèle ne doit jamais écrire directement dans les sauvegardes ni modifier les règles. Le moteur déterministe ROZA garde l'autorité sur les PV, l'inventaire, les missions, les compétences et la progression.

## Limites de validation

L'export de l'étape 3 est présent et son manifeste structurel a passé les contrôles CRDP/CTK2 et le budget préliminaire. Cela ne constitue pas une validation qualitative des réponses et ne constitue pas un essai sur la carte. Les contrôles de qualité, de mémoire libre, de vitesse, de stabilité et le fonctionnement physique sont reportés après l'étape 5, sur le firmware complet.
