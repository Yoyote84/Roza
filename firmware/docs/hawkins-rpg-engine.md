# Moteur RPG Hawkins — étapes 1 à 3

Le jeu est intégré au firmware ROZA existant. Il conserve les pilotes matériels de ROZA et ne remplace ni les fonctions GNSS, ni les fonctions LoRa, ni l'interface principale.
La cible reste le Cardputer ADV ESP32-S3FN8, Flash 8 Mo, sans PSRAM.

## Étape 1 — monde persistant déterministe

- hawkins_world.h/.cpp : état du monde, évolution journalière, statistiques et événements.
- Une graine identique et les mêmes dates donnent la même évolution.
- Aucun état de fin de campagne n'existe dans le moteur.
- Le rattrapage temporel est plafonné à 4096 jours par appel, sans plafonner la durée totale du monde.

## Étape 2 — sauvegarde et première interface

- hawkins_save_codec.h/.cpp : format explicite little-endian, versionné, 58 octets, CRC-32.
- hawkins_nvs.h/.cpp : deux emplacements NVS alternés ; la dernière sauvegarde valide est préservée pendant l'écriture de la suivante.
- hawkins_runtime.h/.cpp : restauration du monde, progression et sauvegarde.
- hawkins_app.h/.cpp : page Hawkins dans un groupe RPG séparé du menu ROZA.
- Tableau de bord : jour du monde, nombre d'événements, tension, anomalies, activité des factions et dernier événement.

## Étape 3 — exploration physique entièrement hors ligne

- hawkins_exploration.h/.cpp : conversion WGS-84 vers une grille locale de cellules de 250 m, identifiants déterministes, visites et découvertes.
- Le pilote GNSS existant du Cap LoRa-1262 est réutilisé ; aucune API web, géolocalisation IP, tuile distante ou connexion Wi-Fi n'est sollicitée par Hawkins.
- Un point GPS doit être valide, compter au moins cinq satellites et rester stable pendant 10 secondes avant d'être pris en compte.
- Le premier point fixe l'origine. Les nouveaux secteurs font progresser le calendrier d'une journée ; une découverte narrative est garantie tous les trois nouveaux secteurs.
- 24 identifiants de secteurs récents sont conservés pour reconnaître les retours. Le compteur global continue au-delà de cette fenêtre.
- Seuls des identifiants hachés de cellules sont sauvegardés. Les coordonnées latitude/longitude brutes ne sont ni affichées ni écrites en NVS.
- Les données d'exploration sont dans la NVS interne, sous deux emplacements expl0/expl1 avec CRC-32, vérification après écriture et conservation du précédent emplacement valide.
- Le joueur peut ouvrir le tableau de bord du monde ou la page Terrain avec E. S force la sauvegarde locale.
- Le Wi-Fi n'est jamais activé par Hawkins. Le GPS reçoit les signaux satellites sans réseau mobile ou Internet. Le SX1262 n'est pas utilisé pour cette étape.

### Mode calendrier hors ligne

Un nouveau monde peut être créé sans horloge réglée : il reçoit un calendrier fictif local. Dans ce mode, le temps du jeu avance lors des nouvelles explorations et ne saute pas de jours lors d'une future synchronisation Wi-Fi. Les mondes déjà initialisés avec une horloge fiable gardent leur progression temporelle antérieure.

Le Cardputer n'ayant pas de RTC, aucune durée d'extinction ne peut être devinée en mode hors ligne. Le jeu reste néanmoins utilisable, sauvegardable et progressif sans NTP.

## Sauvegarde et stockage

Les données du monde et de l'exploration sont stockées dans la NVS interne. Le verrou existant PrefsLock est réutilisé pour éviter les accès concurrents au gestionnaire Preferences.

Aucun formatage, effacement ou écriture n'est effectué sur la microSD. Les fonctions existantes de partage SPI entre la SD et le SX1262 ne sont pas modifiées.

## Tests logiciels

La suite native hawkinstest vérifie :
- l'évolution déterministe et le rattrapage plafonné ;
- la progression de journées sans horloge ;
- la conversion de coordonnées en cellules et le rejet de coordonnées invalides ;
- les visites, retours et découvertes ;
- les sauvegardes encodées/décodées et la détection de corruption CRC.

La compilation native ne remplace pas les essais physiques. Le GPS peut ne pas obtenir de fix en intérieur ou dans un environnement masqué.

## Limites restantes

- Le tableau Hawkins et le relevé de terrain sont en place ; le personnage, l'inventaire, les dialogues, les quêtes et les combats seront ajoutés dans les étapes suivantes.
- La grille de 250 m est une grille de jeu, pas une carte routière ni un relevé topographique.
- Les essais physiques sur Cardputer ADV + Cap LoRa-1262 restent nécessaires.
