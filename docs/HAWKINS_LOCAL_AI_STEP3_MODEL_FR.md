# Hawkins — étape 3/5 : modèle français embarqué

Date : 21 novembre 2026  
Branche : main

## Décision de modèle

TinyTalk de TheREZOR a été la première base essayée. L'export était lisible et très petit, mais la réponse générée par le véritable moteur ROZA était incohérente en français. Cette base n'est donc pas retenue.

Le candidat actuel est **MaxLSB/LeCarnet-3M**, épinglé à la révision :

\`0bf134cfceb1b00c7c56a0ebbc1560ec634c3502\`

Métadonnées Hugging Face vérifiées :
- architecture LLaMA (\`LlamaForCausalLM\`) ;
- 3 742 144 paramètres ;
- dimension cachée 64, MLP 256, 8 couches, 16 têtes et 16 têtes KV ;
- vocabulaire initial de 50 262 tokens ;
- BOS et EOS ont tous deux l'id 1 ;
- tokenizer GPT-2 byte-level BPE ;
- licence modèle MIT ;
- corpus de pré-entraînement MaxLSB/LeCarnet, licence ODC-BY.

Le contexte embarqué est ramené à 32 tokens pour le premier profil Cardputer. Ce modèle est déjà francophone, mais il a été entraîné principalement sur de courts récits. Le fine-tuning le transforme en candidat conversationnel, sans garantir un raisonnement profond.

## Données de fine-tuning

Le pipeline combine :
- OpenAssistant/OASST1 : messages français évalués, licence Apache-2.0 ;
- FreedomIntelligence/alpaca-gpt4-french : courtes paires de dialogue français, licence Apache-2.0 ;
- 40 exemples de narration Hawkins originaux dans \`firmware/tools/hawkinsllm/seed_fr.jsonl\`.

Les réponses longues, les exemples code/liste, les URLs, adresses e-mail et numéros de téléphone sont filtrés. Les données d'entraînement et de validation sont séparées de façon déterministe.

## Format embarqué et changements du moteur

Le convertisseur \`convert_lecarnet.py\` produit :
- un modèle CRDP version 3, architecture LLaMA, quantification Q4_0 row-planar ;
- un tokenizer CTK2 byte-level BPE, réduit à partir des tokens présents dans le corpus et de leurs dépendances BPE.
- une table couvrant les 256 valeurs d'octet : les 210 tokens de base réellement présents sont conservés, et les 46 valeurs absentes pointent vers l'embedding <UNK> appris. Aucun token ni poids artificiel n'est créé ; les valeurs rares sont rendues par le caractère de remplacement UTF-8.

Le moteur `firmware/src/hawkins_llm/llm.cpp` sélectionne désormais le layout CRDP v3 selon l'architecture déclarée : LLaMA utilise son agencement RMSNorm/RoPE/SwiGLU, tandis que GPT-Neo garde son layout et son cache distincts. Les six tokens de contrôle du tokenizer LeCarnet sont portés par `tokenizer_config.json` (`added_tokens_decoder`), séparément de `vocab.json` et `merges.txt`. Le convertisseur remappe le BOS/EOS d'origine (id 1) dans le vocabulaire réduit CTK2 et le conserve comme token spécial sans octets de texte ; le moteur insère ce nouvel ID et l'arrête à la génération. Les poids du modèle restent en Flash ; le cache KV et les buffers de calcul utilisent la SRAM interne.

## Validation nécessaire

Le workflow \`.github/workflows/hawkins-lecarnet-model.yml\` :
1. prépare les données françaises ;
2. fine-tune LeCarnet-3M ;
3. compile le firmware ROZA de base ;
4. convertit les poids et le tokenizer ;
5. vérifie le format et le budget Flash, en gardant 256 Kio de marge ;
6. compile le vrai \`llm.cpp\` dans le host harness REZOR ;
7. génère une réponse à une scène Hawkins et publie le modèle, les métriques, les diagnostics et le manifeste.

**L'étape 3 n'est terminée que si la réponse du moteur hôte est lisible et cohérente en français.** Une CI verte seule ne suffit pas. La génération sur le Cardputer physique, la mesure du heap libre, la vitesse et la stabilité sont réservées à l'étape 5 avec le firmware final.

## Fichiers principaux

- \`firmware/tools/hawkinsllm/seed_fr.jsonl\`
- \`firmware/tools/hawkinsllm/prepare_fr_dataset.py\`
- \`firmware/tools/hawkinsllm/finetune_fr.py\`
- \`firmware/tools/hawkinsllm/convert_lecarnet.py\`
- \`firmware/tools/hawkinsllm/verify_lecarnet_export.py\`
- \`.github/workflows/hawkins-lecarnet-model.yml\`

## Références et licences

- LeCarnet-3M (MIT) : https://huggingface.co/MaxLSB/LeCarnet-3M
- Corpus LeCarnet (ODC-BY) : https://huggingface.co/datasets/MaxLSB/LeCarnet
- OpenAssistant/OASST1 (Apache-2.0) : https://huggingface.co/datasets/OpenAssistant/oasst1
- FreedomIntelligence/alpaca-gpt4-french (Apache-2.0) : https://huggingface.co/datasets/FreedomIntelligence/alpaca-gpt4-french
- Moteur de référence : https://github.com/therezor/cardputer-ai
