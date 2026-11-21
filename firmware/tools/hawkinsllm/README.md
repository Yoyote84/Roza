# Hawkins — modèle français embarqué (étape 3/5)

Ce dossier prépare un modèle français compact pour le moteur local de ROZA. Aucun modèle ni firmware intermédiaire n'est destiné à être flashé sur le Cardputer : le modèle sera intégré à l'application à l'étape 4 et le seul firmware installable sera livré à l'étape 5.

## Modèle candidat retenu

- Base : \`MaxLSB/LeCarnet-3M\`, révision \`0bf134cfceb1b00c7c56a0ebbc1560ec634c3502\`.
- Architecture : LLaMA, 3 742 144 paramètres, 8 couches, dimension 64, dimension MLP 256, 16 têtes.
- Licence du modèle : MIT.
- Jeu de pré-entraînement : \`MaxLSB/LeCarnet\`, licence ODC-BY.
- Fine-tuning : OpenAssistant/OASST1 (Apache-2.0), FreedomIntelligence/alpaca-gpt4-french (Apache-2.0), plus 40 dialogues Hawkins originaux.
- Export cible : CRDP v3 LLaMA Q4_0 row-planar + CTK2 byte-level BPE, contexte de 32 tokens.

TinyTalk (anglais) a été testé comme première base, mais sa génération française sur le vrai moteur ROZA était incohérente. Il est conservé uniquement comme essai de comparaison et n'est pas le modèle candidat final.

## Pipeline

1. \`prepare_fr_dataset.py\` prépare des dialogues courts, filtrés et distincts pour entraînement/validation.
2. \`finetune_fr.py\` adapte LeCarnet-3M aux réponses conversationnelles françaises et aux scènes Hawkins.
3. \`convert_lecarnet.py\` réduit le vocabulaire BPE à partir du corpus, réordonne les poids d'embedding, quantifie les matrices LLaMA en Q4_0 row-planar et écrit le format CRDP v3 + CTK2.
4. \`verify_lecarnet_export.py\` contrôle les dimensions, en-têtes, tables BPE, tailles et budget Flash avec 256 Kio de marge.
5. Le workflow compile le véritable \`firmware/src/hawkins_llm/llm.cpp\` dans le host harness REZOR et tente une réponse française courte.

La révision épinglée possède 210 tokens d'octet de base réellement entraînés, et non les 256 d'un byte-BPE complet. Le convertisseur conserve ces 210 IDs et mappe les 46 valeurs manquantes vers l'embedding <UNK> déjà présent dans le modèle. Aucun ID, poids ou token artificiel n'est créé. À l'encodage, ces octets rares deviennent le token inconnu ; au décodage, celui-ci est rendu par le caractère de remplacement UTF-8. Les octets utilisés par le français courant restent représentés par leurs vrais tokens. Le vocabulaire est compacté pour rester raisonnable sur la Flash. Les six tokens de contrôle LeCarnet sont déclarés dans `tokenizer_config.json` (`added_tokens_decoder`), pas dans les merges BPE ordinaires. L'exporteur récupère leurs IDs depuis ces métadonnées ; l'EOS/BOS d'id 1 est ajouté comme token spécial CTK2 sans octets de texte. Le wrapper CTK2 de ROZA l'insère quand le moteur demande un BOS et arrête la génération à cet EOS.

## Limites et critères de validation

LeCarnet-3M est un petit modèle francophone entraîné principalement sur des récits, pas un grand modèle de raisonnement. Le fine-tuning peut améliorer les dialogues courts, mais la qualité doit être jugée sur les réponses réellement produites par \`llm.cpp\`, pas uniquement sur la perte d'entraînement ou les exemples de Transformers.

Étape 3 n'est validée que si :
- entraînement et validation se terminent avec des pertes finies ;
- export CRDP v3 et CTK2 passe les contrôles ;
- taille modèle + tokenizer + firmware respecte la partition avec marge ;
- le host harness ouvre le modèle et produit un texte français compréhensible ;
- les métriques, sorties et manifeste de provenance sont conservés dans l'artefact.

Le test sur le véritable Cardputer ADV n'est pas fait à l'étape 3. La mémoire libre, la vitesse réelle, la stabilité et la qualité finale seront vérifiées avec le firmware complet à l'étape 5. Le modèle ne modifie jamais les PV, l'inventaire, les missions, les compétences ou les sauvegardes : ces règles restent dans le moteur déterministe de ROZA.

## Fichiers

- \`seed_fr.jsonl\`
- \`prepare_fr_dataset.py\`
- \`finetune_fr.py\`
- \`convert_lecarnet.py\`
- \`verify_lecarnet_export.py\`
- \`.github/workflows/hawkins-lecarnet-model.yml\`

## Sources et licences

- LeCarnet-3M : https://huggingface.co/MaxLSB/LeCarnet-3M
- Corpus LeCarnet : https://huggingface.co/datasets/MaxLSB/LeCarnet
- OpenAssistant/OASST1 : https://huggingface.co/datasets/OpenAssistant/oasst1
- Alpaca GPT-4 French : https://huggingface.co/datasets/FreedomIntelligence/alpaca-gpt4-french
- Moteur REZOR : https://github.com/therezor/cardputer-ai
- Cardputer ADV : https://docs.m5stack.com/en/core/Cardputer-Adv
