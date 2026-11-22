# Hawkins — étape 3/5 : modèle français final

**État : étape 3 terminée — passage à l'étape 4.**

## Livrable de l'étape 3

Le modèle à intégrer au firmware Hawkins est maintenant entraîné et exporté. Il ne s'agit plus d'un simple candidat ou de scripts de préparation : les deux fichiers binaires de livraison sont disponibles dans le paquet **HAWKINS-ETAPE3-MODELE-FR-FINAL.zip**.

- Poids : `model_lecarnet_q4.bin`
- Tokenizer : `tokenizer_lecarnet_ctk2.bin`
- Format des poids : CRDP v3, architecture LLaMA, Q4_0 row-planar
- Format du tokenizer : CTK2, GPT-2 byte-level BPE
- Révision source : `MaxLSB/LeCarnet-3M@0bf134cfceb1b00c7c56a0ebbc1560ec634c3502`
- Contexte du premier profil : 32 tokens
- Vocabulaire exporté : 14 104 tokens

## Entraînement réalisé

Le pipeline a préparé un corpus Hawkins en français à partir de données autorisées et des dialogues originaux du projet, puis a fine-tuné LeCarnet-3M sur trois époques.

Métriques enregistrées par le pipeline :
- 4 867 exemples d'entraînement et 510 exemples de validation ;
- 3 époques, batch de 16, séquence de 32 tokens ;
- meilleure perte de validation enregistrée : 5,8709 ;
- durée d'entraînement : 215,5 secondes sur le runner CPU.

La perte de validation a diminué au fil des trois époques. Ces métriques décrivent l'entraînement ; elles ne remplacent pas une évaluation de la qualité des réponses.

## Tailles et budget de référence

| Fichier / mesure | Valeur |
|---|---:|
| Poids Q4_0 CRDP v3 | 1 050 048 octets |
| Tokenizer CTK2 | 230 722 octets |
| Firmware de base utilisé pour l'estimation | 3 769 088 octets |
| Total estimé avant intégration | 5 049 858 octets |
| Partition applicative | 8 257 536 octets |
| Marge de sécurité réservée | 262 144 octets |
| Espace restant après cette marge | 2 945 534 octets |
| Heap dynamique estimé pour le moteur | 127 872 octets |

Le manifeste d'export du pipeline indique `status: valid`. Il vérifie la structure CRDP/CTK2, les dimensions et le budget sur le firmware de base. Le budget est **préliminaire** : l'étape 4 doit embarquer les fichiers et l'étape 5 doit mesurer le binaire final et le heap réel sur la carte.

## Intégrité des fichiers livrés

`model_lecarnet_q4.bin` — SHA-256 :

`99f5b02e550d2de461003aa312dbbaf0e6e4bd2744f082472e8103bd0f91fc45`

`tokenizer_lecarnet_ctk2.bin` — SHA-256 :

`e707eaa72c56acb1f73ef2a7f6d89327875c22d9301be5e58ebca8eac3168f5b`

Le paquet inclut le manifeste d'export, les métriques d'entraînement, les informations de provenance et un fichier `SHA256SUMS.txt`.

## Décision de fin d'étape

**L'étape 3 est terminée pour le passage à l'étape 4** : le modèle entraîné, les poids quantifiés et le tokenizer final à intégrer sont disponibles.

La qualité des réponses, le comportement conversationnel, la mémoire réelle, la vitesse et la stabilité ne sont pas déclarés validés ici. Conformément à l'ordre décidé pour le projet, ces contrôles seront repris après l'étape 5, sur le firmware complet.

## Travail de l'étape 4

1. Ajouter les deux binaires au mécanisme d'embarquement de l'image ROZA, sans dépendre de la microSD ni du réseau pour charger le modèle.
2. Initialiser le moteur avec les poids et le tokenizer embarqués, avec une gestion d'échec qui laisse le jeu utilisable.
3. Relier la narration au contexte Hawkins déjà calculé par le firmware.
4. Laisser le moteur déterministe ROZA garder l'autorité sur les PV, l'inventaire, les compétences, les missions, les choix acceptés et les sauvegardes.
5. Supprimer la dépendance du narrateur au téléphone compagnon USB pour le fonctionnement normal du jeu hors ligne.
6. Conserver le modèle et le tokenizer dans le paquet de livraison de l'étape 3 afin qu'ils soient identifiables et reproductibles.

## Provenance et licences

- LeCarnet-3M : MIT.
- Corpus de pré-entraînement LeCarnet : ODC-BY.
- OpenAssistant/OASST1 : Apache-2.0.
- FreedomIntelligence/alpaca-gpt4-french : Apache-2.0.
- Les 40 dialogues Hawkins originaux sont fournis par le projet.

Les attributions doivent être conservées lors de la redistribution du modèle et du corpus.
