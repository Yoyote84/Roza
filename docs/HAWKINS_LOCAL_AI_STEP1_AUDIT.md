# Hawkins — audit étape 1/5 : IA locale embarquée

Date : 21 novembre 2026  
Branche examinée : `main`  
Commit de référence : `0858a2589efee9cc7a84857a8a689e32d7b0ede9`

## Décision de l'étape 1

**Base technique retenue :** le moteur d'inférence du projet open source [therezor/cardputer-ai](https://github.com/therezor/cardputer-ai), adapté au firmware ROZA existant.

**Architecture cible :** l'IA s'exécute directement sur le Cardputer ADV. Aucun téléphone, ordinateur externe, serveur, Wi-Fi ou accès Internet ne sera nécessaire pendant une partie. Le jeu reste hors ligne et garde la maîtrise de ses règles.

**Important :** le moteur REZOR est une preuve concrète qu'un petit modèle génératif peut tourner sur ce matériel. Le modèle livré avec ce projet n'est pas encore un narrateur français Hawkins prêt à l'emploi. Il sert de base de recherche et doit être adapté avant son intégration.

Aucun code de jeu ni pilote n'est modifié à cette étape. Cette livraison est un audit et une décision d'architecture.

## 1. Matériel réellement disponible

La fiche officielle M5Stack confirme le matériel du Cardputer ADV :

- SoC ESP32-S3FN8, double cœur Xtensa LX7 à 240 MHz ;
- 8 Mo de Flash ;
- 512 Ko de SRAM interne ;
- aucune PSRAM sur le module Stamp-S3A ;
- lecteur microSD pour le stockage externe.

Source : [documentation officielle Cardputer ADV](https://docs.m5stack.com/en/core/Cardputer-Adv).

La microSD **n'ajoute pas de RAM**. Elle peut stocker des sauvegardes, des cartes, des ressources et des données du jeu, mais elle ne transforme pas les 512 Ko de SRAM en une mémoire de plusieurs gigaoctets. L'IA devra avoir un format et un moteur conçus pour la mémoire réellement disponible.

## 2. Projet existant réutilisable : Cardputer AI de REZOR

Dépôt examiné : [therezor/cardputer-ai](https://github.com/therezor/cardputer-ai).

Le projet prouve déjà les points suivants sur Cardputer / Cardputer ADV :

- chatbot embarqué, sans téléphone et sans réseau ;
- poids quantifiés lus depuis la Flash mappée, intégrés à l'image firmware ;
- moteur GPT-Neo/TinyTalk avec calcul matriciel Q4 et assembleur SIMD propre à ESP32-S3 ;
- cache de contexte compact et tokenizer parcouru depuis la Flash ;
- débit annoncé par l'auteur pour la version TinyTalk 2 : environ 5 tokens/s sur l'appareil.

Ces chiffres sont ceux annoncés par le projet amont, **pas encore des mesures sur le firmware ROZA**.

Sources : [README et code Cardputer AI](https://github.com/therezor/cardputer-ai), [notes de développement](https://github.com/therezor/cardputer-ai/blob/main/docs/DEVELOPING.md).

### Ce qu'on réutilisera

- le moteur d'inférence `llm.cpp / llm.h`, après port et vérification ;
- le noyau assembleur ESP32-S3 `dot_q4_pie.S` ;
- le format de poids quantifiés et les outils de conversion compatibles ;
- les stratégies de cache KV compact, de tokenizer en Flash et de génération incrémentale ;
- les tests/harness de référence qui peuvent être adaptés au dépôt ROZA.

### Ce qu'on ne recopiera pas tel quel

- l'application complète et son interface de chat autonome ;
- son écran, sa boucle clavier et ses pilotes, puisque ROZA possède déjà les siens ;
- son moteur de conversation générale comme logique de jeu ;
- son modèle anglais non adapté à Hawkins.

On intégrera le moteur sous forme de module au firmware ROZA, sans créer un second gestionnaire d'écran, un deuxième pilote clavier ou un deuxième système de sauvegarde.

## 3. Modèle : choix prudent

Le projet amont a plusieurs générations de TinyTalk. Il ne faut pas les confondre.

### TinyTalk 2 (version récente du firmware Cardputer AI)

Le README amont annonce un modèle 8M, environ 5,7 Mo de poids Q4 intégrés, un contexte de 72 tokens sur cette variante et environ 5 tokens/s mesurés par l'auteur. Le modèle est entraîné pour de petites conversations en anglais ; ses notes indiquent qu'il ne convient pas au raisonnement, aux longues conversations ou aux instructions complexes.

**Décision : ne pas intégrer directement TinyTalk 2 dans ROZA.** Les seuls poids de 5,7 Mo, ajoutés au firmware ROZA, rendent le budget Flash trop serré et ses données d'entraînement récentes incluent des jeux de données non commerciaux décrits dans le NOTICE amont.

### TinyTalk historique (carte de modèle TheREZOR/TinyTalk)

La carte du modèle historique indique environ 8,3 millions de paramètres au total, une quantification Q4_0 embarquée d'environ 2 Mo dans Cardputer AI, et un contexte pratique d'environ 80 tokens sur l'appareil. Son comportement reste limité : anglais simple, petite conversation, peu de mémoire, pas de connaissances fiables et pas de suivi d'instructions complexe.

Sa licence affichée est CC BY 4.0 ; les attributions du modèle de base, de SODA et du tokenizer doivent être conservées.

Source : [carte TheREZOR/TinyTalk](https://huggingface.co/TheREZOR/TinyTalk).

**Candidat provisoire pour l'adaptation :** cette variante historique plus compacte, plutôt que TinyTalk 2. Elle ne sera pas embarquée telle quelle : il faut d'abord vérifier le format exact, la taille convertie et les possibilités de fine-tuning français.

### Adaptation française obligatoire

Le modèle historique est explicitement présenté comme anglophone et limité au small talk. Le firmware final ne devra donc pas présenter le modèle anglais d'origine comme une IA française.

Le travail de l'étape 3 devra :

- préparer un corpus français original de narration et de dialogues courts Hawkins ;
- choisir des textes et jeux de données dont les licences permettent l'usage et la redistribution envisagés ;
- adapter le modèle et son vocabulaire/tokenizer si nécessaire ;
- produire le fichier quantifié attendu par le moteur ;
- mesurer la taille, la mémoire, la qualité de génération et le contexte utile ;
- tester des prompts français représentatifs du jeu.

Le corpus doit apprendre au modèle à décrire une scène courte, respecter un ton et proposer des possibilités. Il ne doit pas lui confier la vérité de l'état du jeu.

## 4. Compatibilité avec la chaîne ROZA actuelle

État constaté dans le dépôt au commit de référence :

- ROZA compile avec PlatformIO, environnement `cardputer-adv` ;
- framework actuel : Arduino-ESP32 2.0.17 (plateforme épinglée dans `firmware/platformio.ini`) ;
- bibliothèques et BSP existants : M5Unified/M5GFX, pilote clavier ADV, SD, LoRa, GNSS, sauvegardes et moteur Hawkins ;
- partition active : `firmware/partitions_8MB.csv`, partition application `factory` à `0x10000`, taille `0x7E0000` (environ 7,875 Mio).

Le projet Cardputer AI amont est une base ESP-IDF/PlatformIO différente : pioarduino avec ESP-IDF 5.5.4, configuration QIO/80 MHz et sa propre table de partitions. **Ce n'est donc pas un firmware qu'on peut fusionner ou flasher tel quel par-dessus ROZA.**

### Stratégie de port retenue

1. Conserver la chaîne PlatformIO + Arduino et les versions épinglées de ROZA au départ.
2. Isoler uniquement le moteur LLM, son assembleur, ses formats de modèle et ses outils nécessaires.
3. Vérifier les appels ESP-IDF/FreeRTOS et les instructions Xtensa PIE sous le framework ROZA actuel.
4. Faire un test de compilation isolé avant de connecter l'IA au gameplay.
5. Ne migrer ni tout le projet vers ESP-IDF 5.5.4 ni les bibliothèques M5Stack sans nécessité démontrée.

Si un appel précis du moteur amont exige une API absente du framework ROZA actuel, on cherchera d'abord une adaptation ciblée. Toute migration globale devra faire l'objet d'une décision distincte et d'un audit de régression.

## 5. Budget Flash et rôle de la microSD

La partition applicative ROZA est presque toute la Flash disponible, environ 7,875 Mio. Le candidat historique TinyTalk est annoncé autour de 2 Mo en Q4_0 ; le tokenizer, le code du moteur et le firmware Hawkins doivent également tenir dans cette même partition.

Le modèle récent TinyTalk 2, annoncé autour de 5,7 Mo, n'est pas notre choix initial : il laisserait trop peu de marge une fois le firmware ROZA ajouté.

**Aucun chiffre de taille final n'est garanti à ce stade.** À l'étape 2 puis à l'étape 3, nous mesurerons les tailles exactes des binaires ROZA, du modèle converti et du tokenizer. Le critère est que l'image complète soit inférieure à la partition, avec une marge documentée.

La microSD restera destinée aux sauvegardes et ressources du jeu. La première version du moteur lira le modèle depuis la Flash intégrée, comme le projet amont, pour éviter de dépendre d'un lecteur SD lent pour chaque opération matricielle.

## 6. Répartition des responsabilités

### IA embarquée

- rédiger une narration courte en français ;
- donner une ambiance et des réactions de personnages ;
- proposer jusqu'à trois choix de joueur ;
- tenir compte d'un résumé très compact de la scène.

### Moteur ROZA — autorité absolue du jeu

- personnage, PV, stress, compétences et XP ;
- objets et inventaire ;
- missions, drapeaux narratifs et progression ;
- monde, lieux visités et événements ;
- relations PNJ et sauvegardes NVS / microSD.

Le modèle ne modifiera jamais directement les statistiques, les objets, les missions ou les sauvegardes. ROZA validera les actions et leurs conséquences. Si le modèle échoue ou manque de mémoire, le jeu doit rester utilisable avec ses mécanismes locaux de secours.

## 7. Plan de réalisation en cinq étapes

| Étape | Travail | Résultat attendu |
|---|---|---|
| **1/5 — Audit et décision** | Étude du matériel, du moteur REZOR, des licences, du stockage, de la Flash et de la chaîne ROZA. | Architecture choisie et contraintes écrites. |
| **2/5 — Port du moteur** | Porter le moteur LLM et le noyau ESP32-S3 dans la chaîne ROZA ; mesurer RAM, Flash et débit sur un test isolé. | Moteur embarqué compilé et démontré sans régression du firmware. |
| **3/5 — Modèle français** | Construire/adapter le petit modèle français, le tokenizer et les poids quantifiés ; tests de narration et de qualité. | Modèle français adapté au format du moteur et au budget matériel. |
| **4/5 — Intégration Hawkins** | Connecter le narrateur au monde, au personnage, aux PNJ, aux missions et aux choix ; préserver les règles ROZA. | Le jeu utilise le narrateur embarqué sans téléphone ni réseau. |
| **5/5 — Firmware final et essais** | Compilation complète, tests automatisés, vérifications de mémoire/Flash, essais sur Cardputer ADV, corrections et génération du paquet Factory final. | Firmware Hawkins autonome final, BIN Factory, contrôles SHA-256 et instructions d'installation. |

Le paquet final est livré dans l'étape 5 : il n'y aura pas une sixième phase de développement séparée. Le test physique final doit être réalisé sur le vrai Cardputer ADV ; une compilation CI ne sera jamais présentée comme un essai matériel réussi.

## 8. Critères de fin

Le projet ne sera considéré terminé que si :

- le Cardputer démarre ROZA avec le moteur IA embarqué ;
- une génération française courte fonctionne hors ligne sur l'appareil ;
- l'écran et le clavier ROZA existants restent opérationnels ;
- des choix peuvent être sélectionnés et traités par les règles du jeu ;
- les sauvegardes, missions, inventaire, SD, LoRa et GNSS existants ne sont pas régressés ;
- le modèle et les poids sont intégrés à l'image firmware ; aucun téléphone n'est requis ;
- les mesures Flash/RAM et la version exacte du modèle sont documentées ;
- les workflows de compilation et tests passent sur le SHA final ;
- le BIN Factory final est fourni avec son empreinte et ses instructions.

## Sources vérifiées

- M5Stack, [Cardputer ADV — caractéristiques officielles](https://docs.m5stack.com/en/core/Cardputer-Adv)
- REZOR, [Cardputer AI — moteur, performances et limites](https://github.com/therezor/cardputer-ai)
- REZOR, [notes de développement et format du moteur](https://github.com/therezor/cardputer-ai/blob/main/docs/DEVELOPING.md)
- Hugging Face, [TheREZOR/TinyTalk — modèle, limites et licence](https://huggingface.co/TheREZOR/TinyTalk)
