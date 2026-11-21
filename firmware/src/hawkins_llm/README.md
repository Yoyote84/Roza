# Moteur IA embarqué — port ROZA

Ce dossier contient le moteur d'inférence C++ issu de [therezor/cardputer-ai](https://github.com/therezor/cardputer-ai), adapté à l'arborescence PlatformIO active de ROZA.

## Source et licence

- Dépôt amont : https://github.com/therezor/cardputer-ai
- Révision copiée : `78c5128672b750977619dc0a6c3b8baed91168ed`
- Fichiers importés : `llm.cpp`, `llm.h`, `dot_q4_pie.S`
- Licence du code moteur : MIT, voir `LICENSE`.
- Attribution et provenance du moteur : REZOR, 2026.
- Les poids du modèle et les données d'entraînement ne sont pas importés à l'étape 2. Leurs licences doivent être vérifiées séparément avant distribution.

## Portage

Le moteur utilise seulement des API déjà fournies par l'environnement ROZA :
- `esp_heap_caps.h` pour l'allocation SRAM interne ;
- FreeRTOS pour le worker de calcul sur le second cœur ;
- les instructions PIE SIMD du processeur ESP32-S3, via `dot_q4_pie.S`.

La version ROZA reste PlatformIO + Arduino-ESP32 2.0.17 / ESP-IDF 4.4.7. Aucune migration globale vers l'ESP-IDF amont 5.5.4 n'est effectuée.

## Format de modèle attendu

Le moteur sait lire :
- CRDP v1 (LLaMA) ;
- CRDP v3 (GPT-Neo/TinyTalk, Q4_0 en lignes planaires) ;
- tokenizer SentencePiece ou CTK2 GPT-2 BPE.

Aucun fichier de poids ou tokenizer n'est embarqué dans ce commit. La préparation du modèle français, la conversion et le budget final sont réservés à l'étape 3.

## Limites de cette étape

Cette étape porte et compile le moteur. Elle ne signifie pas encore que le modèle français a été entraîné, qu'une inférence a été exécutée sur l'appareil, ni que le narrateur est raccordé aux règles Hawkins. Ces validations arrivent dans les étapes suivantes.


## Liaison au jeu Hawkins

`hawkins_app.cpp` récupère la table `HawkinsLlmApi` à l'entrée de l'application. Cette référence garde les fonctions réelles du moteur dans le firmware final au lieu de les laisser éliminer par le ramasse-miettes du linker. Elle ne charge pas de poids et ne réserve pas le cache de contexte ; l'initialisation du modèle et la génération restent pour les étapes 3 et 4.

Le journal série indique explicitement que le moteur est lié, mais que le modèle n'est pas encore embarqué.


## Robustesse du port — étape 2

Les tests hôte couvrent également les pointeurs nuls, en-têtes trop courts, longueur de tokenizer invalide, et métadonnées CTK2 avec compteur de merges négatif ou hors blob. Ces tests visent les erreurs de format avant de charger un tokenizer en mémoire.
