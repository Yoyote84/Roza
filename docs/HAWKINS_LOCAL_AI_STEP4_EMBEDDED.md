# Hawkins — étape 4/5 : intégration de l'IA embarquée

**État : étape 4 intégrée — prête pour l'étape 5.**

## Objectif

Intégrer le modèle français LeCarnet-3M, livré à l'étape 3, directement dans l'application Hawkins du firmware ROZA. À la fin de cette étape, le code est conçu pour produire la narration sur le Cardputer ADV sans compagnon Android, câble USB vers le téléphone, Wi-Fi ou Internet.

Le Cardputer reste l'autorité sur les règles du jeu et les sauvegardes.

## Changements apportés dans le firmware

### 1. Modèle lu depuis la Flash

- `firmware/src/hawkins_llm/model_data.S` déclare les symboles de début et de fin des poids et du tokenizer.
- `firmware/src/hawkins_llm/runtime.cpp` récupère les tailles à partir de ces symboles et transmet les deux fichiers au moteur.
- Les binaires attendus dans `firmware/src/hawkins_llm/assets/` sont :
  - `model_lecarnet_q4.bin`
  - `tokenizer_lecarnet_ctk2.bin`
  - `model_manifest.json`
  - `SHA256SUMS.txt`
- Le workflow de l'étape 3 doit publier les deux binaires dans ce dossier après leur export. Les fichiers sont intégrés dans l'image applicative ; la microSD n'est pas requise pour les charger.

### 2. Runtime local

`runtime.cpp` initialise le transformeur LLaMA CRDP v3, le tokenizer CTK2 et le sampler. Le profil de départ utilise un contexte de 32 tokens et une génération de 12 tokens maximum.

Le moteur nettoie les allocations temporaires si l'initialisation échoue. Les échecs de chargement ou une génération vide désactivent seulement le narrateur : le jeu reste utilisable.

### 3. Raccordement à Hawkins

`hawkins_app.cpp` demande au runtime une narration dans une tâche FreeRTOS séparée, afin de ne pas bloquer directement la boucle d'affichage. Le contexte transmis contient un instantané compact du joueur, du monde, de l'événement, de l'objectif actif et de l'action choisie.

Le texte généré est affiché sur la page du narrateur et ajouté à la mémoire narrative. Les choix visibles de cette première intégration restent contrôlés par ROZA : Observer, Répondre, Se cacher. Leur sélection ne donne pas au modèle le droit d'altérer directement les données du joueur.

### 4. Suppression de la dépendance Android/USB

Le chemin HKA/1 qui attendait le compagnon Android a été retiré du lecteur série du jeu. Le code Android reste dans le dépôt comme prototype historique, mais il n'est plus requis par l'architecture actuelle.

## Responsabilités conservées par le moteur déterministe

Le modèle ne modifie jamais lui-même :
- les PV, le stress, les caractéristiques, l'expérience ou les niveaux ;
- l'inventaire et les objets ;
- les missions et les récompenses ;
- les compétences ;
- les drapeaux de progression et les sauvegardes NVS / microSD.

ROZA valide les choix et leurs conséquences.

## Correctifs techniques importants

Le chargement CRDP v3 LLaMA utilise maintenant les pas de lignes Q4 row-planar pour toutes les matrices du format v3. Le calcul de ces pas ne dépend plus uniquement de l'architecture GPT-Neo : c'est indispensable pour lire le modèle LeCarnet LLaMA correctement.

Une fonction `llm_free` libère les allocations de calcul si l'initialisation est interrompue. Le runtime valide l'architecture, le vocabulaire, la taille du contexte et le tokenizer avant d'autoriser la génération.

## État de livraison

- [x] Runtime local et API d'intégration ajoutés.
- [x] Symboles d'embarquement du modèle et du tokenizer ajoutés.
- [x] Narrateur Hawkins raccordé au runtime local.
- [x] Génération déplacée vers une tâche FreeRTOS.
- [x] Chemin compagnon Android/HKA retiré du flux du jeu.
- [x] Documentation mise à jour.
- [x] Publication des poids, du tokenizer, du manifeste et des SHA-256 dans `firmware/src/hawkins_llm/assets/`.
- [x] Chemins `.incbin` alignés sur les fichiers publiés ; runtime local raccordé à Hawkins.
- [ ] Compilation complète de l'image finale : réservée à l'étape 5.
- [ ] Mesures physiques de mémoire, débit, qualité et stabilité sur le Cardputer : réservées après l'étape 5.

La publication des binaires est terminée. La source d'embarquement `.S` référence les fichiers présents dans le dépôt. L'intégration logicielle de l'étape 4 est réalisée ; la compilation de l'image complète et sa validation sont volontairement reportées à l'étape 5.

## Frontière avec l'étape 5

Cette étape réalise le raccordement logiciel. Elle ne déclare pas que la qualité des réponses, la vitesse ou la stabilité sont validées.

L'étape 5 reste la livraison d'un seul firmware complet, la compilation finale, les contrôles Flash/RAM, le paquet Factory et l'installation sur le vrai Cardputer ADV. Les évaluations de réponses de l'IA auront lieu sur cette version complète, après les essais de l'utilisateur.
