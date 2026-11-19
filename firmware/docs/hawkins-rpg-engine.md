# Moteur de monde persistant Hawkins — première étape

Ce module constitue le premier socle logiciel du JDR grandeur nature prévu pour le
Cardputer ADV. Il est volontairement indépendant de l'interface et des pilotes
matériels afin de pouvoir être testé sur ordinateur avant son intégration au jeu.

## Fichiers

- `src/hawkins_world.h` : structures d'état et API publique.
- `src/hawkins_world.cpp` : simulation journalière déterministe et génération d'événements.
- `tools/hawkinstest/` : tests natifs, exécutables sans carte ESP32.

## Fonctionnement actuel

1. `initializeWorld()` crée un monde à partir d'une graine et d'une date Unix fiable.
2. `advanceWorld()` calcule les jours écoulés depuis le début de la campagne.
3. Chaque jour fait évoluer la tension, le niveau d'anomalie et l'activité des factions.
4. Certains jours produisent un événement : signal, anomalie électrique, trace de créature,
   mouvement de faction, appel de détresse ou fluctuation de brèche.
5. Une graine identique et les mêmes dates produisent la même évolution, ce qui facilite
   les sauvegardes, les tests et la synchronisation future entre joueurs.

Le monde ne possède aucun état de fin de campagne. La limite de 4096 jours par appel
est uniquement une protection contre un rattrapage trop long au démarrage ; les appels
successifs rattrapent le reste sans perdre de jours.

## Limites à ce stade

- Ce moteur n'est pas encore relié à un écran ou à une entrée du menu ROZA.
- L'état du monde n'est pas encore sérialisé sur la microSD ou dans NVS : ne pas considérer
  une partie comme sauvegardée entre deux redémarrages.
- La date est fournie par l'appelant ; l'intégration GNSS, RTC ou date manuelle reste à faire.
- Les secteurs 0 à 31 sont des identifiants abstraits, pas des coordonnées GPS.
- Les événements sont des données de simulation ; aucune émission LoRa n'est effectuée.
- Le générateur pseudo-aléatoire est destiné au jeu, jamais à la cryptographie.

## Tests

Depuis `firmware/` :

```sh
bash tools/hawkinstest/build.sh
SAN=1 bash tools/hawkinstest/build.sh
```

La CI ROZA exécute également cette suite dans le workflow PlatformIO.

## Étapes suivantes

1. Ajouter une sauvegarde versionnée et résistante aux coupures, sans écriture automatique
   sur la microSD tant que son système de fichiers et son état n'ont pas été validés.
2. Créer un journal d'événements et un mécanisme de récupération après redémarrage.
3. Relier l'horloge au service de temps existant et aux sources GNSS/manuel.
4. Intégrer une application Hawkins dans l'interface ROZA.
5. Ajouter les personnages, les combats au tour par tour, puis les communications LoRa.
