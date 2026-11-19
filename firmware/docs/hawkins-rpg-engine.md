# Moteur RPG Hawkins — étapes 1 et 2

Le jeu est développé dans le firmware ROZA existant, sans remplacer ses pilotes matériels.
La base reste PlatformIO + Arduino-ESP32 sur Cardputer ADV ESP32-S3FN8, Flash 8 Mo, sans PSRAM.

## Étape 1 — monde persistant déterministe

- hawkins_world.h/.cpp : état du monde, évolution journalière, statistiques et événements.
- Une graine identique et les mêmes dates donnent la même évolution.
- Aucun état de fin de campagne n'existe dans le moteur.
- Le rattrapage est plafonné à 4096 jours par appel, sans plafonner la durée totale du monde.

## Étape 2 — sauvegarde, horloge et première interface

- hawkins_save_codec.h/.cpp : format explicite little-endian, versionné, 58 octets, CRC-32.
- hawkins_nvs.h/.cpp : deux emplacements NVS alternés ; la dernière sauvegarde valide est préservée pendant l'écriture de la suivante.
- hawkins_runtime.h/.cpp : restauration du monde, synchronisation avec l'horloge ROZA, rattrapage et sauvegarde.
- hawkins_app.h/.cpp : première page Hawkins dans un groupe RPG séparé du menu ROZA existant.
- Tableau de bord : jour du monde, nombre d'événements, tension, anomalies, activité des factions et dernier événement.

### Horloge

Le runtime utilise l'horloge système ROZA : base manuelle conservée en NVS, NTP lorsque le Wi-Fi est activé volontairement, ou UTC GNSS lorsqu'un Cap LoRa-1262 fournit une date GPS valide. Le jeu n'active pas le réseau.

Si l'heure n'est pas valide, la sauvegarde reste consultable mais le monde ne progresse pas. Sans RTC ou synchronisation GNSS/NTP, le temps écoulé appareil éteint ne peut pas être connu de façon fiable ; le jeu ne l'invente pas.

### Sauvegarde

Les instantanés vont dans la NVS interne, sous le namespace hawkins et les deux clés slot0/slot1. Le verrou NVS existant PrefsLock est réutilisé afin d'éviter les accès concurrents avec les services ROZA.

Aucun formatage, effacement ou écriture n'est effectué sur la microSD. Les fonctions existantes de partage SPI entre SD et SX1262 ne sont pas touchées.

### Contrôles

La suite native hawkinstest vérifie le monde déterministe, la progression sur dix ans, les sauvegardes encodées/décodées, la détection de corruption CRC et les états invalides. Elle est exécutée par la CI PlatformIO.

### Limites actuelles

- L'écran Hawkins est un tableau de bord, pas encore l'interface complète du personnage.
- Les personnages, classes, inventaire, combats D20 et journal détaillé des événements restent à développer.
- La date restaurée depuis manual_epoch est une base locale, pas un RTC : seule une source GNSS/NTP fraîche permet de rattraper précisément le temps réellement écoulé pendant une longue extinction.
- Les essais physiques sur Cardputer ADV restent nécessaires ; la CI ne valide pas le matériel.
