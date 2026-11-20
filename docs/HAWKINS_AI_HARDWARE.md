# Hawkins — audit technique de l'IA hors ligne

État : **étape 4/5 en cours**. Ce document sépare les prototypes réellement disponibles des composants qui restent à intégrer.

## Cible matérielle et contraintes ROZA

La cible active est le Cardputer ADV : ESP32-S3FN8, 8 Mo de Flash et 512 Ko de SRAM interne, sans PSRAM. ROZA utilise actuellement PlatformIO + Arduino-ESP32 (versions épinglées dans `firmware/platformio.ini`). Le firmware conserve les applications, pilotes, GNSS, LoRa, sauvegardes et interface existants.

La carte microSD est un stockage externe bloc/FAT. Ce n'est pas une mémoire directement adressable par le MMU de l'ESP32-S3 comme la Flash interne. Un runtime qui attend des pointeurs vers des poids mémoire-mappés ne peut donc pas être branché tel quel sur un fichier SD : il faut une couche d'accès/cache et des mesures réelles de débit, ou une autre architecture de stockage explicitement validée. Pas de chargement fictif du modèle entier en RAM.

## Piste vérifiée : Cardputer AI / TinyTalk 2

Dépôt de référence : https://github.com/therezor/cardputer-ai

Ce projet prouve qu'un petit modèle génératif réel fonctionne sur le même Cardputer ADV sans PSRAM. L'auteur publie les valeurs suivantes : 8 M de paramètres, poids quantifiés d'environ 5,7 Mo, environ 5 tokens/s sur l'appareil et une fenêtre de contexte de 72 tokens pour le modèle 8M. Le runtime utilise des poids lus directement depuis la Flash mappée et un cache KV int4.

**Limites incompatibles avec le besoin ROZA actuel :**
- TinyTalk 2 est entraîné pour des conversations simples en anglais et des histoires courtes ; ce n'est pas un narrateur français spécialisé dans Hawkins.
- Le contexte annoncé est très court (72 tokens sur le modèle 8M).
- Le modèle est intégré dans l'image firmware et non chargé depuis la microSD.
- Le code moteur est MIT, mais les poids TinyTalk 2 ont une licence distincte CC BY-NC-SA 4.0, avec données DailyDialog et SciQ non commerciales. Le code MIT ne rend donc pas les poids MIT.

Sources :
- https://github.com/therezor/cardputer-ai
- https://github.com/therezor/cardputer-ai/blob/main/LICENSE
- https://github.com/therezor/cardputer-ai/blob/main/NOTICE.md
- https://huggingface.co/TheREZOR/TinyTalk-2

Décision : ne pas importer ni redistribuer les poids TinyTalk 2 comme s'ils étaient libres pour tout usage ; ne pas livrer des dialogues anglais dans le jeu français.

## Piste francophone étudiée : Nomade-15M-fr

Fiche du modèle : https://huggingface.co/RDTvlokip/Nomade-15M-fr

La fiche décrit un modèle génératif de 15 M de paramètres entraîné en français, sous Apache 2.0, avec tokenizer BPE 32k, architecture Transformer récurrente/à profondeur bouclée et fenêtre annoncée de 768 tokens. Son fichier actuel `best_model.pt` est d'environ 60,1 Mo en précision complète.

**Limites :** modèle de base non instruction-tuné, code d'inférence personnalisé, aucun port ESP32/Cardputer documenté dans sa fiche ; les résultats sont préliminaires (une seule graine, sans contrôle de variance). Il faudrait quantifier, réduire/pruner le vocabulaire, adapter son architecture et vérifier le budget RAM/Flash avant d'envisager un essai.

Décision : c'est une base francophone intéressante pour un prototype, mais pas un modèle directement installable sur la carte.

## Travail ROZA effectué pour préparer le contexte

Le moteur Hawkins reste propriétaire de l'état du jeu :
- la mémoire narrative est persistante en NVS et dans l'archive SD v2 ;
- `formatNarrativeMemorySummary` produit un résumé compact, sans inventer de faits ;
- `formatHawkinsAiContext` construit un contexte borné depuis le personnage, ses caractéristiques, l'inventaire, les compétences, les missions, le monde simulé, l'exploration GNSS et les événements de mémoire narrative ;
- le contexte indique au modèle de produire un récit en français et de ne jamais modifier les états du jeu.

Le générateur IA, lorsqu'il sera intégré, ne pourra produire que du texte ou des propositions typées. Toute action (récompense, objet, soin, XP, progression de mission, déplacement) devra être validée par les fonctions existantes du moteur. Aucune mutation directe par le modèle.

## Portes restantes de l'étape 4

1. Choisir/adapter un modèle francophone dont les poids et la licence peuvent accompagner ROZA.
2. Convertir/pruner/quantifier le modèle et son tokenizer ; conserver un manifeste de version et SHA-256.
3. Implémenter l'accès au fichier modèle sur microSD avec buffers bornés, sans charger les poids complets en SRAM et sans écrire/formatter automatiquement la carte.
4. Porter le moteur sur la chaîne Arduino-ESP32 épinglée, sans basculer le firmware complet vers ESP-IDF ni remplacer les pilotes existants.
5. Mesurer sur le Cardputer : heap libre/max bloc, temps de préremplissage, tokens/s, consommation et stabilité.
6. Tester une batterie de prompts Hawkins en français, hors Wi-Fi, puis vérifier que seules les actions validées par le moteur peuvent affecter la partie.

**État honnête :** le contexte réel du jeu est préparé et testé en natif ; aucun moteur génératif ni poids de modèle n'est encore intégré à ROZA. L'étape 4 n'est donc pas terminée. La découverte de Cardputer AI prouve la faisabilité d'un LLM minuscule sur cette puce, mais ne résout pas à elle seule les exigences françaises et microSD.
