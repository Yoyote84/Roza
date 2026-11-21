# Hawkins — faisabilité de l'IA hors ligne sur Cardputer ADV

## Conclusion actualisée — 21 novembre 2026

Une IA générative très compacte est **possible** directement sur le Cardputer ADV. Le projet open source [therezor/cardputer-ai](https://github.com/therezor/cardputer-ai) fournit une démonstration existante sur la même classe de matériel : ESP32-S3FN8, 512 Ko de SRAM, 8 Mo de Flash et aucune PSRAM.

Ce constat remplace l'ancienne conclusion qui supposait qu'un narrateur génératif sur le Cardputer n'était pas réaliste. Il ne signifie pas qu'un modèle 4B peut tenir sur cette carte : seul un modèle extrêmement compact, spécialement conçu et quantifié pour le microcontrôleur, est envisageable.

## Matériel

La fiche M5Stack Cardputer ADV indique :

- ESP32-S3FN8, double cœur LX7 à 240 MHz ;
- Flash 8 Mo ;
- stockage externe microSD ;
- écran 240 x 135 et clavier 56 touches.

Le module Stamp-S3A ne possède pas de PSRAM. Les 512 Ko de SRAM interne sont la mémoire de travail du système.

Sources :
- https://docs.m5stack.com/en/core/Cardputer-Adv
- https://docs.m5stack.com/en/products_selector/m5stamp_compare

La microSD augmente la capacité de stockage, pas la RAM de travail. Elle reste utile pour les sauvegardes et ressources de ROZA, mais la première intégration IA lira son modèle depuis la Flash intégrée, comme le moteur Cardputer AI amont.

## Base technique retenue

Dépôt : https://github.com/therezor/cardputer-ai

Composants intéressants :
- moteur d'inférence GPT-Neo/TinyTalk en C++ ;
- noyau assembleur SIMD Q4 spécifique à ESP32-S3 ;
- poids quantifiés lus depuis la Flash mappée ;
- cache KV compact et tokenizer parcouru depuis la Flash ;
- démonstration hors ligne annoncée par l'auteur à environ 5 tokens/s pour TinyTalk 2.

Ces performances décrivent le projet amont, pas encore une version intégrée à ROZA.

Le modèle TinyTalk historique annoncé sur Hugging Face est d'environ 8,3 millions de paramètres, avec des poids Q4_0 autour de 2 Mo, et environ 80 tokens de contexte pratique sur l'appareil. Il est anglophone, limité aux conversations simples et n'est pas un narrateur JDR français prêt à l'emploi.

TinyTalk 2 est plus récent et meilleur en anglais, mais ses poids quantifiés sont annoncés autour de 5,7 Mo et son contexte embarqué est de 72 tokens. Il n'est pas retenu comme choix initial pour ROZA : l'espace Flash restant après ajout du firmware serait trop réduit.

Sources :
- https://github.com/therezor/cardputer-ai
- https://huggingface.co/TheREZOR/TinyTalk

## Adaptation nécessaire pour Hawkins

Le moteur REZOR est la base de portage. Le modèle anglais ne sera pas embarqué tel quel. Il faut préparer une adaptation française avec un corpus de dialogues et de narration courte dont les licences sont vérifiées.

Le modèle aura un rôle limité :
- produire une narration courte ;
- créer l'ambiance et des réactions de personnages ;
- suggérer jusqu'à trois choix.

Le moteur Hawkins reste la seule autorité pour les PV, stress, XP, compétences, objets, missions, progression, relations et sauvegardes. Le modèle ne pourra pas écrire dans l'état du jeu.

## Compatibilité avec ROZA

ROZA utilise actuellement PlatformIO + Arduino-ESP32 2.0.17 et ses bibliothèques M5Stack épinglées. Cardputer AI amont utilise PlatformIO avec ESP-IDF 5.5.4, sa propre configuration QIO et sa propre table de partitions.

Le portage direct du firmware amont n'est donc pas retenu. On doit porter le moteur d'inférence et les composants nécessaires dans la chaîne ROZA existante, sans remplacer l'UI, le clavier, le SD, LoRa, GNSS ni le système de sauvegarde. Toute API incompatible devra être adaptée et testée de manière ciblée.

La partition applicative ROZA actuelle est `factory` à `0x10000`, taille `0x7E0000` (environ 7,875 Mio). Les tailles finales du firmware, des poids et du tokenizer seront mesurées ensemble avant intégration finale ; aucune estimation ne remplace cette mesure.

## Architecture cible

- Cardputer ADV : moteur IA local, affichage, clavier et règles du jeu.
- Flash intégrée : code, poids Q4 et tokenizer.
- microSD : sauvegardes et ressources Hawkins/ROZA.
- Réseau : inutile pendant le jeu.
- Téléphone Android : non requis et non utilisé comme moteur IA.

## Étapes de réalisation

La feuille de route de référence en cinq étapes est documentée dans `docs/HAWKINS_LOCAL_AI_STEP1_AUDIT.md`. Le présent document consigne le changement de conclusion matérielle et les contraintes à respecter.
