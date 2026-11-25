# Provenance des composants — firmware ROZA

## Source principale et historique d'import

- Dépôt amont initial : https://github.com/Lewis-Peter/cardputer-adv-firmware
- Commit amont importé : `3671c86f7850bd0342f39b5befad5b0c02d33f5b`
- Date du commit amont indiquée à l'import : 8 octobre 2026
- Date d'import dans ROZA : 4 novembre 2026
- Répertoires importés à l'origine : `src/`, `lib/`, `data/`, `tools/`, `docs/`, `platformio.ini` et les licences associées.

Cette provenance explique l'origine du code, mais les versions listées dans la section « import initial » ne représentent pas nécessairement le firmware actuel.

## Chaîne de compilation active vérifiée sur main le 25 novembre 2026

- Outil : PlatformIO Core 6.2.0
- Plateforme : `espressif32@8.0.0`
- Framework : Arduino-ESP32 3.3.12
- Base ESP-IDF : 5.5.5
- Environnement : `cardputer-adv`
- Carte PlatformIO : `esp32-s3-devkitc-1`, Flash 8 Mo
- Table active : `firmware/partitions_8MB.csv`
- Commande : `cd firmware && pio run -e cardputer-adv`
- Workflow : `.github/workflows/roza_esp32s3.yml`

Le firmware actif est un projet PlatformIO + Arduino-ESP32. Les anciens fichiers ESP-IDF racine sont conservés à titre historique et ne doivent pas être utilisés pour compiler ce firmware.

## Versions épinglées dans firmware/platformio.ini

| Dépendance | Version active |
|---|---|
| M5Unified | 0.3.0 |
| M5GFX | 0.3.0 |
| NimBLE-Arduino | 2.5.2 |
| TinyGPSPlus | 1.1.0 déclarée dans PlatformIO |
| ArduinoJson | 7.4.3 |
| RadioLib | 7.7.1 |
| FastLED | 3.10.3 |
| ESP8266Audio | tag Git 2.4.3 |
| LibSSH-ESP32 | 5.9.0 |

Les versions actuellement épinglées sont définies par `firmware/platformio.ini` et décrites dans `firmware/docs/dependency-pins.md`. ESP8266Audio 2.4.3 est maintenant utilisé : son pilote ULP/DAC est conditionné par la capacité DAC du SoC. Les versions amont disponibles et les changements proposés sont suivis dans `docs/FIRMWARE_REFRESH_STEP1_AUDIT.md`.

## État de validation CI au 25 novembre 2026

Commit de référence : https://github.com/Yoyote84/Roza/commit/71cd1ff0216ec426f8d073153c838373181da5b9

Exécution CI : https://github.com/Yoyote84/Roza/actions/runs/45518114744

Le test de nettoyage des anciens composants du jeu retiré réussit. La compilation firmware échoue ensuite dans `firmware/src/wardrive.cpp` sur l'identifiant interne `STA_STARTED_BIT` qui n'est plus exposé par le core actuel. Les étapes de génération des images et de publication des artefacts sont donc ignorées. Aucun nouveau binaire final ni mesure de mémoire liée au commit de référence n'est disponible.

Un ancien succès CI ou une mesure mémoire sous Arduino 2.0.17 / ESP-IDF 4.4.7 ne valide pas le commit courant.

## Provenance, licences et règles de conservation

- Le dépôt amont initial est sous MIT, mais chaque sous-composant conserve ses propres avis et éventuelles licences distinctes.
- LibSSH-ESP32 embarque libssh, dont la licence et les obligations doivent être examinées avant remplacement ou redistribution.
- Toute alternative comme wolfSSH doit faire l'objet d'un contrôle de licence, de dépendances et de compatibilité avant intégration.
- Conserver tous les fichiers LICENSE, COPYING, en-têtes de copyright et attributions des bibliothèques tierces.
- Ne jamais intégrer une clé privée, un mot de passe, un token ou une clé API dans Git.
- Ne jamais créer de stub ou de faux pilote pour faire passer la compilation.

## Compatibilité matérielle à préserver

Cible : M5Stack Cardputer ADV (ESP32-S3FN8, 8 Mo Flash, sans PSRAM dans la configuration ROZA) et Cap LoRa-1262 (SX1262 + GNSS ATGM336H-6N / AT6668).

- Le clavier ADV est un TCA8418 ; il ne faut pas le confondre avec le clavier de l'ancien Cardputer.
- Le SPI microSD et le SPI SX1262 utilisent GPIO40/GPIO39/GPIO14. La SD utilise CS GPIO12 et le SX1262 CS GPIO5.
- GNSS : UART sur GPIO13/GPIO15 et expandeur I²C du Cap à l'adresse utilisée par le code 0x43.
- Le Wi-Fi et le Bluetooth restent arrêtés au démarrage.
- LoRa reste en réception par défaut ; aucune émission automatique.
- Ne pas monter, formater, effacer ou écrire sur la microSD de l'utilisateur avant identification du système de fichiers et sauvegarde.
- Toute validation physique doit être explicitement consignée ; un build CI ne prouve pas le fonctionnement matériel.

## Sources amont

- M5Stack Cardputer ADV : https://docs.m5stack.com/en/core/Cardputer-Adv
- M5Stack Cap LoRa-1262 : https://docs.m5stack.com/en/cap/Cap_LoRa-1262
- PlatformIO Core : https://github.com/platformio/platformio-core/releases
- Arduino-ESP32 : https://github.com/espressif/arduino-esp32/releases
- ESP-IDF : https://github.com/espressif/esp-idf/releases
- M5Unified : https://github.com/m5stack/M5Unified/releases
- M5GFX : https://github.com/m5stack/M5GFX/releases
- RadioLib : https://github.com/jgromes/RadioLib/releases
- FastLED : https://github.com/FastLED/FastLED/releases
- NimBLE-Arduino : https://github.com/h2zero/NimBLE-Arduino/releases
- ArduinoJson : https://github.com/bblanchon/ArduinoJson/releases
- TinyGPSPlus : https://github.com/mikalhart/TinyGPSPlus
- ESP8266Audio : https://github.com/earlephilhower/ESP8266Audio/releases
- SdFat : https://github.com/greiman/SdFat/releases
- LibSSH-ESP32 : https://github.com/ewpa/LibSSH-ESP32
- wolfSSH : https://github.com/wolfSSL/wolfssh/releases
