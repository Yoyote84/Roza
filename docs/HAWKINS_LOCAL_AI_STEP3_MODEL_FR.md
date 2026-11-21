# Hawkins — étape 3/5 : adaptation du modèle français

Date : 21 novembre 2026  
Branche : main

## Objectif

Préparer un petit modèle GPT-Neo qui peut produire de courtes réponses en français sur le Cardputer ADV. Cette étape fournit le modèle quantifié et son tokenizer dans un artefact GitHub Actions ; elle ne modifie pas encore le firmware flashable. L'intégration matérielle appartient à l'étape 4, et le seul firmware destiné à l'installation sera livré à l'étape 5.

## Base retenue

- Modèle : TheREZOR/TinyTalk, GPT-Neo, 8 couches, dimension 128, environ 8,3 millions de paramètres publiés.
- Licence du modèle : CC BY 4.0. Les attributions TinyTalk, TinyStories-Instruct-3M, SODA et GPT-2 doivent être conservées.
- Données françaises : OpenAssistant/oasst1 (messages français validés, licence Apache-2.0) et FreedomIntelligence/alpaca-gpt4-french (dialogues human→gpt courts, licence Apache-2.0).
- Corpus Hawkins : exemples originaux versionnés dans firmware/tools/hawkinsllm/seed_fr.jsonl.
- Convertisseur : therezor/cardputer-ai épinglé au commit 78c5128672b750977619dc0a6c3b8baed91168ed.

Le modèle TinyTalk d'origine est anglophone. Le simple fait de lui envoyer un prompt en français ne suffit donc pas. Le fine-tuning tente d'adapter son style de dialogue au français, mais le résultat reste expérimental et doit être lu dans l'artefact avant d'être présenté comme utilisable.

## Pipeline créé

1. prepare_fr_dataset.py combine des messages OpenAssistant évalués et un corpus d'instructions/dialogues français court issu de FreedomIntelligence/alpaca-gpt4-french. Il filtre les messages OpenAssistant supprimés, synthétiques, non validés ou signalés ; les exemples trop longs, les réponses de refus génériques et les réponses de type code/listes sont exclus. Les URLs, adresses e-mail et numéros de téléphone sont retirés.
2. Les exemples originaux Hawkins sont sur-échantillonnés modérément pour donner une tonalité de narration au petit modèle.
3. finetune_fr.py affine TheREZOR/TinyTalk avec la séquence User: ... / Bot: ..., masque la perte sur le prompt utilisateur et limite les séquences d'entraînement à 80 tokens.
4. Le convertisseur amont produit model_neo_q4.bin (CRDP v3, GPT-Neo, Q4_0) et tok_neo.bin (CTK2 byte-level BPE). La réduction du vocabulaire conserve les 256 tokens octet, ce qui permet de représenter les caractères accentués UTF-8.
5. verify_export.py vérifie les en-têtes, les dimensions, la correspondance des vocabulaires, les tables BPE, les longueurs de fichiers et le budget Flash estimé.
6. Le host harness utilise les fichiers llm.cpp et llm.h présents dans ROZA, et non un autre moteur de référence, pour tenter une génération française d'essai.

## Critères de validation de cette étape

L'étape ne sera considérée comme validée qu'après un workflow vert avec :
- un corpus français suffisant et une validation distincte ;
- une perte d'entraînement et de validation finies ;
- un export CRDP v3 / CTK2 cohérent avec le parseur ROZA ;
- une estimation Flash inférieure à la partition application 0x7E0000, avec 256 Kio de marge ;
- une génération non vide via le moteur ROZA sur ordinateur ;
- un artefact contenant le modèle quantifié, le tokenizer, les métriques, les sorties d'essai et le manifeste de provenance.

Cela ne prouve pas encore l'exécution physique sur l'ESP32-S3. Le firmware ne doit pas être flashé à ce stade. L'étape 4 intégrera le modèle au jeu, puis l'étape 5 fabriquera et testera le firmware final sur le Cardputer ADV.

## Fichiers de cette étape

- firmware/tools/hawkinsllm/seed_fr.jsonl
- firmware/tools/hawkinsllm/prepare_fr_dataset.py
- firmware/tools/hawkinsllm/finetune_fr.py
- firmware/tools/hawkinsllm/verify_export.py
- .github/workflows/hawkins-fr-model.yml

## Sources et attribution

- TheREZOR/TinyTalk : https://huggingface.co/TheREZOR/TinyTalk
- OpenAssistant/oasst1 : https://huggingface.co/datasets/OpenAssistant/oasst1
- FreedomIntelligence/alpaca-gpt4-french : https://huggingface.co/datasets/FreedomIntelligence/alpaca-gpt4-french
- Cardputer AI / moteur et convertisseur : https://github.com/therezor/cardputer-ai
- Cardputer ADV officiel : https://docs.m5stack.com/en/core/Cardputer-Adv
