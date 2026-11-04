# Dossier de reprise technique ROZA

Audit documentaire actualisé sur `main` après le commit `c83499428c564ada2c969ce48c06bd0e7c498a34`. Ce document prépare les prochains lots ; il ne vaut ni compilation ni essai matériel.

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

## État constaté dans le code

### Chaîne de build
- `firmware/platformio.ini` épingle plateforme, framework et bibliothèques.
- Le workflow `.github/workflows/roza_esp32s3.yml` compile `cardputer-adv`, vérifie ELF/firmware/bootloader/partitions et publie les artefacts.
- Aucun run Actions n'était associé au commit de base `3bfaa0b` au moment de l'audit. Le dernier succès retrouvé était le run #45 sur `8e380dca2e85fe4cb1b5404572ee4bc7e4fbc6ac` : ce succès ne valide pas les commits suivants.

### G1 — écran, clavier, IMU
- Écran/rétroéclairage : intégration via le BSP M5Unified/M5GFX, initialisé depuis `firmware/src/main.cpp`. Aucun motif démontré de réécrire ce pilote.
- Clavier ADV : pilote existant `firmware/src/keyboard_adv.cpp/.h`, TCA8418 à l'adresse I²C `0x34`, bus interne M5Unified, 100 kHz. Le code gère FIFO, mapping, Fn, modificateurs et répétition.
- IMU : `firmware/src/imu.cpp` utilise `M5.Imu.update()`, `getAccel()` et `getGyro()`. Le flux série et l'UI partagent un cache pour éviter deux lectures concurrentes.
- Les deux vues IMU vérifient maintenant `M5.Imu.isEnabled()` avant d'afficher les mesures ; si le BMI270 n'est pas activé/détecté, elles affichent un état indisponible au lieu de présenter les valeurs initiales du cache (0, 0, 1) comme des mesures réelles. Correction publiée dans `c834994`.
- Ces éléments prouvent une intégration logicielle, pas le fonctionnement physique. Vérifier détection, touches, modificateurs, orientation, luminosité, veille/réveil et stabilité des mesures sur la vraie carte.

### G2 — batterie, SD, SPI
- Batterie : fonctions existantes dans `firmware/src/power_util.*` et écrans/logs associés. Une estimation reste une estimation tant qu'elle n'est pas calibrée sur plusieurs cycles.
- SD : `firmware/src/sd_files.cpp` appelle `SPI.begin(40, 39, 14, 12)`, puis `SD.begin(12, SPI, 20000000)`. Ce montage n'est pas en lecture seule.
- Vérification de la bibliothèque réellement épinglée : Arduino-ESP32 2.0.17 expose `SD.begin(..., bool format_if_empty=false)`, mais aucun paramètre de montage lecture seule. `format_if_empty=false` évite le formatage automatique à l'échec du montage ; cela ne protège pas contre les écritures ultérieures. L'API expose par ailleurs des fonctions d'écriture (`File::write`, `SD.remove`, etc.) ; l'absence d'un paramètre RO dans `SD.begin` signifie que le code actuel ne garantit pas un montage protégé contre les écritures. La configuration de compilation FatFs n'a pas été vérifiée indépendamment. Il serait donc trompeur de qualifier le montage actuel de lecture seule.
- `sdInit()` est appelé au démarrage. `formatSD()` n'est pas appelé au démarrage : l'interface Paramètres exige deux confirmations successives (`formatStep` 0 puis 1) avant l'effacement récursif. La fonction reste destructive et doit rester explicitement protégée.
- Le même fichier expose `formatSD()`, qui supprime récursivement fichiers et dossiers de la racine. C'est destructif même si aucun formatage bas niveau n'est effectué. Ne jamais l'appeler pendant une détection ou un test.
- Décision de sécurité : ne pas prétendre qu'un montage lecture seule existe et ne pas remplacer arbitrairement la bibliothèque. Avant d'activer le critère G2, concevoir un audit brut non destructif des secteurs (MBR/partition/boot sector) avant le montage FAT, ou une solution de montage réellement protégée, puis vérifier le partage SPI avec le SX1262. Ne pas effectuer de test d'écriture.

### G4 — GNSS et LoRa
- GNSS existant dans `firmware/src/gnss.cpp` : TinyGPSPlus, UART1 RX GPIO15/TX GPIO13, expandeur I²C `0x43` via `M5.In_I2C`.
- LoRa existant dans `firmware/src/lora.cpp` avec RadioLib/SX1262. Vérifier le câblage du Cap LoRa-1262, commutateur RF, antenne, fréquences, puissance et réglementation France/UE avant toute émission.

## Ordre des travaux

### Lot 1 — terminer G1
1. Auditer les points d'entrée dans `main.cpp`, les pilotes et le BSP ; relever pins, bus, adresses, versions et retours d'erreur.
2. Rechercher les conflits I²C, les états d'initialisation incorrects et les erreurs réelles en cas de périphérique absent.
3. Corriger seulement les défauts démontrés, sans créer de deuxième pilote ni diagnostic redondant.
4. Compiler avec la commande de référence ; vérifier le run CI associé au SHA exact.
5. Préparer puis consigner les essais physiques : affichage/orientation/luminosité/veille-réveil ; matrice clavier et toutes les touches Fn/modificateurs ; présence IMU, accélération/gyroscope et stabilité.
6. Ne clôturer G1 qu'après CI verte et essais physiques consignés.

### Lot 2 — G2
1. Cartographier les callsites batterie/SD ; ne pas créer de deuxième service.
2. Documenter conversion ADC, calibration, limites et états inconnus.
3. Auditer la SD sans écriture : présence, capacité, format, compatibilité.
4. Étudier une vraie stratégie lecture seule avec la bibliothèque épinglée ; si non supportée, laisser la SD inactive plutôt que simuler une protection.
5. Définir l'arbitrage SPI SD/SX1262 avant activation simultanée.
6. Écriture de test uniquement après identification du format, sauvegarde et validation explicite. Aucun formatage automatique.

### Lots 3 à 5 — G3/G4/G5
- Cartographier le cycle de vie des applications et services avant toute évolution d'interface ; conserver toutes les applications.
- Mesurer heap libre, plus grand bloc, minimum historique et Flash avant/après les fonctions lourdes.
- Stabiliser GNSS/SX1262 après audit bus/alimentation ; réception LoRa par défaut.
- Maintenir le mode hors ligne ; API facultatives et erreurs réseau non bloquantes.
- Prioriser robustesse, sauvegarde/récupération et fonctions locales sans sacrifier l'existant.

## Rapport obligatoire à chaque lot

Indiquer branche, SHA complet, fichiers modifiés, recherche de doublons, commandes et résultats, compilation locale, lien CI du SHA exact, tests logiciels, essais physiques (ou « non testés sur matériel »), risques et prochaine action. Une CI verte prouve uniquement la compilation dans l'environnement CI.
