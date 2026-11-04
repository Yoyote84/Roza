# РОЗА — base firmware propre

Ce dossier est le **seul projet firmware actif** du dépôt. Il remplace la première fondation artisanale, dont les pilotes maison ne compilaient pas.

## Choix techniques

- ESP-IDF natif, C++23.
- Cible ESP32-S3 / Cardputer ADV, 8 Mo Flash, sans PSRAM.
- BSP `espp/m5stack-cardputer` version **1.3.11** (ESP Component Registry, licence MIT).
- LVGL fourni par les dépendances du BSP.
- Pas de code de pilote écran ou clavier réécrit localement.
- Wi-Fi et Bluetooth non démarrés.
- Pas de montage/formatage automatique de la microSD.
- Pas d'initialisation radio LoRa ni d'émission au démarrage.

Le BSP existant fournit les pilotes Cardputer / Cardputer ADV et expose aussi les fonctions du Cap SX1262 + GNSS. Les fonctions LoRa/GNSS seront intégrées dans une étape dédiée après validation de la révision du Cap et de l'antenne.

## Compilation locale

Prérequis : ESP-IDF **5.5.1** installé pour ESP32-S3 et ESP Component Manager disponible.

```sh
cd /public/Roza/firmware
. "$IDF_PATH/export.sh"
idf.py set-target esp32s3
idf.py build
```

Le premier build télécharge les dépendances décrites dans `main/idf_component.yml`. Conserver `dependencies.lock` après sa génération pour figer les versions transitives.

## État honnête

Cette base initialise l'écran, LVGL, le clavier et tente l'IMU BMI270. Elle n'est pas encore un système d'exploitation complet. La compilation CI doit réussir avant toute déclaration de validation logicielle ; l'affichage, les touches et l'IMU doivent ensuite être testés sur le matériel réel.

La microSD 128 Go n'est pas montée ici : il faut d'abord identifier son système de fichiers. Ne jamais la formater automatiquement.
