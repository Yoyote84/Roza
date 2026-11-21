# Hawkins — production du modèle français (étape 3/5)

**État : étape 3 terminée.** Les poids quantifiés et le tokenizer finaux à intégrer à l'étape 4 ont été produits et livrés dans `HAWKINS-ETAPE3-MODELE-FR-FINAL.zip`.

## Modèle livré

- Base : `MaxLSB/LeCarnet-3M`
- Révision : `0bf134cfceb1b00c7c56a0ebbc1560ec634c3502`
- Architecture : LLaMA, 3 742 144 paramètres
- Fine-tuning : 3 époques sur les dialogues français et Hawkins du projet
- Export : CRDP v3 LLaMA Q4_0 row-planar + CTK2 byte-level BPE
- Contexte : 32 tokens ; vocabulaire exporté : 14 104 tokens
- Poids : 1 050 048 octets
- Tokenizer : 230 722 octets

## Fichiers livrables

- `model_lecarnet_q4.bin` — poids quantifiés à embarquer
- `tokenizer_lecarnet_ctk2.bin` — vocabulaire BPE et merges à embarquer
- `lecarnet-export-manifest.json` — structure, tailles et estimation Flash
- `metrics.json` — métriques de fine-tuning
- `dataset_manifest.json` — provenance du corpus
- `SHA256SUMS.txt` — empreintes des deux binaires

SHA-256 des poids : `99f5b02e550d2de461003aa312dbbaf0e6e4bd2744f082472e8103bd0f91fc45`

SHA-256 du tokenizer : `e707eaa72c56acb1f73ef2a7f6d89327875c22d9301be5e58ebca8eac3168f5b`

## Budget mémoire et Flash

Sur le firmware de base utilisé par l'export, le total estimé firmware + poids + tokenizer est de 5 049 858 octets dans une partition applicative de 8 257 536 octets. Après réservation de 256 Kio, le manifeste laisse 2 945 534 octets.

Cette estimation précède l'intégration à l'image finale. L'étape 4 devra embarquer les deux fichiers ; l'étape 5 recalculera la taille réelle du binaire et mesurera le heap sur la vraie carte.

## Reproduction

Le workflow `.github/workflows/hawkins-lecarnet-model.yml` peut reconstruire le jeu de données, refaire le fine-tuning, exporter le modèle et vérifier la structure du paquet. Il conserve maintenant les artefacts 90 jours.

Les vérifications qualitatives des réponses du modèle ne font pas partie de ce workflow. Elles sont reportées après l'étape 5, selon le plan du projet.

## Responsabilités du jeu

Le modèle ne décide jamais des PV, de l'inventaire, des compétences, des missions, de la progression ni des sauvegardes. Il fournit uniquement une narration et des propositions ; ROZA valide tous les choix et conséquences.

## Licences et provenance

- LeCarnet-3M : MIT.
- Corpus LeCarnet : ODC-BY.
- OpenAssistant/OASST1 et Alpaca GPT-4 French : Apache-2.0.
- Les dialogues Hawkins originaux sont fournis par le projet.

Conserver les attributions lors de la redistribution.
