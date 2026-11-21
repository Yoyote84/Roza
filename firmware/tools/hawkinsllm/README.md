# Hawkins — modèle français embarqué (étape 3/5)

Ce dossier fabrique un petit modèle de dialogue en français pour le moteur local porté de REZOR. Il ne télécharge aucun poids vers le Cardputer pendant le jeu : les fichiers créés ici sont des artefacts de développement destinés à être intégrés dans la Flash à l'étape 4.

## Base choisie

- Modèle de départ : \`TheREZOR/TinyTalk\` (GPT-Neo, 8,3 M paramètres publiés, 8 couches, dimension 128).
- Licence du modèle de départ : CC BY 4.0. Conserver les attributions indiquées dans sa fiche et son NOTICE.
- Données de conversation : \`OpenAssistant/oasst1\`, filtrées sur \`lang=fr\`, licence Apache-2.0.
- Corpus Hawkins additionnel : dialogues originaux écrits pour ce projet, stockés dans \`seed_fr.jsonl\`.
- Export : Q4_0 GPT-Neo CRDP v3 + tokenizer CTK2 byte-level BPE, format attendu par \`firmware/src/hawkins_llm/llm.cpp\`.

Le tokenizer GPT-2 est byte-level : il peut représenter les caractères UTF-8 français. Le convertisseur amont réduit le vocabulaire et conserve les 256 tokens octet de base. Le script de vérification impose la limite CTK2 de 32 768 tokens et vérifie que le vocabulaire du modèle correspond à celui du tokenizer.

## Limites honnêtes

TinyTalk reste un modèle extrêmement petit, conçu pour du small talk anglais, pas pour le raisonnement complexe. Le fine-tuning en français est une adaptation expérimentale ; un passage de CI ou une génération sur ordinateur ne garantit pas une conversation française parfaite sur ESP32-S3. La qualité devra être évaluée à partir des sorties d'essai sauvegardées dans l'artefact.

Le jeu ROZA reste l'autorité : le modèle ne doit jamais modifier les PV, l'inventaire, les missions, les compétences ou les sauvegardes. Il propose uniquement une narration courte et des formulations de choix.

## Reproduire

Depuis la racine du dépôt, avec Python 3.11 :

1. Installer PyTorch CPU, Transformers, Datasets et les dépendances du convertisseur.
2. Exécuter \`prepare_fr_dataset.py\` pour produire train/validation/corpus.
3. Exécuter \`finetune_fr.py\` à partir de \`TheREZOR/TinyTalk\`.
4. Cloner \`therezor/cardputer-ai\` à la révision auditée dans le workflow et lancer \`tools/convert_tinystories_instruct.py\` avec le checkpoint et le corpus français.
5. Lancer \`verify_export.py\`, compiler le firmware de base puis exécuter le host harness avec le vrai \`llm.cpp\` de ROZA.

Le workflow GitHub Actions réalise ces opérations et publie les deux fichiers binaires, les métriques, l'échantillon généré et le manifeste comme artefact temporaire. Aucun firmware flashable final n'est livré à cette étape.

## Attribution

- TinyTalk : TheREZOR, basé sur TinyStories-Instruct-3M (Ronen Eldan et Yuanzhi Li), dialogues SODA (Kim et al.), tokenizer GPT-2/OpenAI. Voir la fiche du modèle et son NOTICE : https://huggingface.co/TheREZOR/TinyTalk
- OpenAssistant OASST1 : OpenAssistant contributors, licence Apache-2.0 : https://huggingface.co/datasets/OpenAssistant/oasst1
- Moteur et convertisseur Cardputer AI : https://github.com/therezor/cardputer-ai
