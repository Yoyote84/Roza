# ROZA — Étape 2/5 : matrice de migration et décision technique

Date : 24 novembre 2026
Base auditée : `main` — `907192a4c55a96464450e854cb52c0d9e291e1c6`

## Décision

Préparer une migration contrôlée vers PlatformIO `espressif32@8.0.0` / Arduino-ESP32 `3.3.12` / ESP-IDF `5.5.5`, avec M5Unified et M5GFX `0.3.0` testés ensemble. Cette cible est préférable à une adoption immédiate d'Arduino core 4.0/ESP-IDF 6.1 : la plateforme PlatformIO officielle 8.0.0 fournit le core 3.3.12 et IDF 5.5.5; elle est plus prudente pour une migration applicative.

Cette décision est une cible de test, pas une déclaration de compatibilité acquise. Aucun changement de version ne sera intégré avant compilation et tests.

## Matrice

| Élément | Version actuelle | Cible proposée | Risque / action |
|---|---|---|---|
| PlatformIO ESP32 | 7.0.1 | 8.0.0 | Vérifier outil de flash, partitionnement, CI, configuration USB |
| Arduino-ESP32 | 2.0.17 (IDF 4.4.7) | 3.3.12 (IDF 5.5.5) | Ruptures API/driver, migration majeure à compiler |
| M5Unified | 0.2.20 | 0.3.0 | API bool, audio, identification matériel, rotation |
| M5GFX | 0.2.27 | 0.3.0 | Mettre à niveau en tandem; rotation absolue modifiée |
| FastLED | 3.10.3 | Maintenir au premier passage | Isoler ce changement; tester LED et charge CPU |
| ESP8266Audio | 1.9.9 | Maintenir initialement | Tester la compilation et les lecteurs; changer séparément ensuite |
| ArduinoJson | 7.4.3 | Maintenir | Pas de changement sans besoin identifié |
| RadioLib | 7.7.1 | Maintenir | Tester SPI partagé SD/SX1262 sous core 3 |
| TinyGPSPlus | 1.1.0 | Maintenir | Test GNSS |
| LibSSH-ESP32 | 5.9.0 | Maintenir | Vérifier compilation et mémoire TLS/SSH |

## Points d'adaptation identifiés

- M5Unified/M5GFX 0.3.0 changent l'orientation absolue : sur Cardputer ADV, l'ancien `setRotation(1)` devient `setRotation(0)` pour le même paysage.
- Certaines fonctions de M5Unified passent de `void` à `bool`; vérifier les usages, les appels audio, les erreurs et la logique d'initialisation.
- Ne pas remplacer le pilote écran, clavier ADV/TCA8418, IMU, microSD, GNSS ou SX1262.
- Contrôler l'USB CDC natif, les options de flash/partition 8MB et le workflow PlatformIO.
- Core 3 change plusieurs APIs ESP32/I2S, Wi-Fi, Bluetooth, GPIO et timers. Faire l'inventaire de compilation avant les corrections.
- Cardputer ADV : ESP32-S3FN8, 8MB Flash, sans PSRAM. Mesurer heap libre, plus grand bloc, RAM statique et Flash; l'absence de PSRAM impose des tests de charge TLS, carte, audio et cartographie.
- SPI partagé microSD et LoRa : contrôler l'arbitrage, les transactions et les régressions de débit/fiabilité.
- Conserver Wi-Fi/Bluetooth désactivés au démarrage et LoRa en réception/émission contrôlée selon les règles existantes.

## Plan de validation de l'étape 3

1. Modifier uniquement la plateforme/framework et les dépendances nécessaires à leur compatibilité.
2. Recompiler avec l'environnement exact `cardputer-adv`, sans stubs ni pilotes factices.
3. Adapter les API et appels de rotation, puis examiner le diff.
4. Mesurer les usages RAM/Flash et comparer au baseline de l'audit étape 1.
5. Exécuter la CI et les tests disponibles.
6. En cas de réussite de compilation, considérer le firmware comme candidat de test seulement; la validation matérielle reste à l'étape 5.

## Hors périmètre de cette étape

- Aucun changement de code ni de versions dans `platformio.ini`.
- Aucun nouveau firmware flashé.
- Aucun test matériel déclaré.
- Arduino core 4.0 / ESP-IDF 6.1, pioarduino ou framework ESP-IDF natif restent hors cible initiale.
- Aucun changement d'interface ROZA.

## Références primaires

- PlatformIO espressif32 8.0.0 : https://github.com/platformio/platform-espressif32/releases
- Arduino-ESP32 : https://github.com/espressif/arduino-esp32/releases
- M5Unified 0.3.0 : https://github.com/m5stack/M5Unified/releases
- M5GFX 0.3.0 : https://github.com/m5stack/M5GFX/releases
- Audit de départ : `docs/FIRMWARE_REFRESH_STEP1_AUDIT.md`
- Pinning et historique des benchmarks : `firmware/docs/dependency-pins.md`

## Résultat

La cible de migration est définie. La prochaine étape (3/5) sera l'implémentation progressive et la compilation, après prise en compte de l'état de la PR #3 (ouverte et non mergeable au moment de cet audit). Cette étape 2 ne modifie pas le firmware.
