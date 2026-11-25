# Dossier de reprise technique ROZA

> **Avis de fraîcheur — 25 novembre 2026 :** le présent dossier contient des observations prises sur un commit antérieur et doit être lu comme historique de travail. Pour l'état actuel du dépôt, les versions des dépendances, le catalogue réel des 40 entrées, la CI du commit courant et la préparation de la grande mise à jour VHS, consulter en priorité [l'audit de référence de l'étape 1/5](FIRMWARE_REFRESH_STEP1_AUDIT.md). Ne pas utiliser les anciens SHA, numéros de versions ou états CI de ce document comme état courant.

Audit documentaire antérieur sur `main` après le commit `c83499428c564ada2c969ce48c06bd0e7c498a34`. Ce document est conservé pour son historique de diagnostic ; il ne vaut ni compilation ni essai matériel.

## Références impératives

- Dépôt `Yoyote84/Roza`, branche autorisée `main` uniquement.
- Firmware actif : `firmware/`, PlatformIO + Arduino-ESP32, environnement `cardputer-adv`.
- Compilation : `cd firmware && pio run -e cardputer-adv`.
- Cible : ESP32-S3FN8, Flash 8 Mo, sans PSRAM.
- Respecter `AGENTS.md`, `docs/ROADMAP.md` et `firmware/docs/dependency-pins.md`.
- Aucun stub, faux pilote, suppression de fonction, clé API inventée ou changement de dépendance non étudié.
- Avant chaque modification, rechercher les fonctions, fichiers, symboles, services et pilotes déjà présents. Réutiliser l'existant ; ne consolider un doublon qu'après comparaison documentée.
- Wi-Fi/Bluetooth inactifs au démarrage. LoRa en réception par défaut, aucune émission automatique.
- Ne jamais formater, effacer ou écrire automatiquement sur la microSD.
- Séparer compilation CI, tests logiciels et essais physiques. Ne jamais déclarer un périphérique validé sur la seule présence de son initialisation.

## État historique constaté dans le code

Les sections ci-dessous décrivent l'état d'un commit précédent. Elles ne remplacent pas l'audit de référence courant.

### G1 — écran, clavier, IMU
- Écran/rétroéclairage : intégration via le BSP M5Unified/M5GFX, initialisé depuis `firmware/src/main.cpp`.
- Clavier ADV : pilote `firmware/src/keyboard_adv.cpp/.h`, TCA8418 à l'adresse I²C `0x34`, bus interne M5Unified.
- IMU : `firmware/src/imu.cpp` utilise `M5.Imu.update()`, `getAccel()` et `getGyro()`.
- Ces éléments prouvent une intégration logicielle, pas le fonctionnement physique.

### G2 — batterie, SD, SPI
- Batterie : fonctions dans `firmware/src/power_util.*`.
- SD : le pilote monte la carte via Arduino SD et expose des opérations de suppression. Le comportement exact et la sécurité du système de fichiers doivent être vérifiés sur le code courant.
- La carte de 128 Go ne doit pas être formatée, effacée ou écrite pour un simple test.

### G4 — GNSS et LoRa
- GNSS existant dans `firmware/src/gnss.cpp`, avec TinyGPSPlus et UART1.
- LoRa existant dans `firmware/src/lora.cpp`, avec RadioLib/SX1262.
- Vérifier les branchements et les paramètres réglementaires avant toute émission.

## Règle de validation

Une CI verte prouve la compilation logicielle pour le SHA testé. Elle ne prouve ni démarrage, ni écran, clavier, SD, audio, GNSS ou réception LoRa sur la carte physique.
