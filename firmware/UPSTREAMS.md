# Provenance des composants — fondation firmware ROZA

## Source principale

- Dépôt amont : https://github.com/Lewis-Peter/cardputer-adv-firmware
- Commit amont exact importé : `3671c86f7850bd0342f39b5befad5b0c02d33f5b`
- Date du commit amont : 2026-10-08
- Date d'import dans ROZA : 2026-11-04
- Répertoire importé : `src/`, `lib/`, `data/`, `tools/`, `docs/`, `platformio.ini`, ainsi que les fichiers de licence et de documentation associés.
- Le SHA d'importation ROZA n'est pas le SHA du dépôt amont. Ne pas les intervertir.

## Système de compilation réel

- Outil : PlatformIO
- Environnement : `cardputer-adv`
- Framework : Arduino-ESP32
- Plateforme : `espressif32@7.0.1`
- Paquet framework épinglé : `framework-arduinoespressif32@3.20017.241212`
- Carte PlatformIO : `esp32-s3-devkitc-1`, configurée pour 8 Mo de Flash
- Table de partitions ROZA : `partitions_8MB.csv` (partition applicative unique ; pas de double slot OTA)
- Commande de compilation : `pio run -e cardputer-adv`

**Important :** ce projet actif n'est pas un projet ESP-IDF natif. Ne pas utiliser `idf.py build` pour compiler ce firmware. Le workflow GitHub Actions doit appeler PlatformIO.

- **Validation CI GitHub Actions #39** : exécution réussie le `2026-11-04` sur le commit `6b57da60850bbcdb7e0c260a1a5071b854783093`.
  - Le workflow installe PlatformIO `6.1.6` et exécute `platformio run -e cardputer-adv`.
  - Il vérifie la présence de `firmware.bin`, `firmware.elf`, `bootloader.bin` et `partitions.bin`, puis publie ces artefacts.
  - RAM : `115612 / 327680 octets (35,3 %)` ; Flash : `3680085 / 8257536 octets (44,6 %)`.
  - Taille du fichier `firmware.bin` : `3680448 octets`.
  - Avertissements non bloquants dans LibSSH-ESP32 : perte du qualificatif `const` sur certains pointeurs et appels à des fonctions dépréciées.
  - **Le workflow ne lance pas de tests unitaires, de tests de syntaxe séparés ni de tests sur carte.** Il valide la compilation croisée et la génération des binaires uniquement.
  - **Essais physiques sur Cardputer ADV et Cap LoRa-1262 : non effectués.** La compilation CI ne prouve ni le démarrage sur carte, ni la réception RF, ni le fonctionnement des périphériques.

- Lien de l'exécution : https://github.com/Yoyote84/Roza/actions/runs/41769762257

## Dépendances déclarées dans platformio.ini

Les versions doivent rester épinglées et les mises à jour doivent être vérifiées par compilation et tests :

- M5Stack M5Unified 0.2.20
- M5Stack M5GFX 0.2.27
- TinyGPSPlus 1.1.0
- ArduinoJson 7.4.3
- RadioLib 7.7.1
- FastLED 3.10.3
- ESP8266Audio 1.9.9 (référence Git)
- LibSSH-ESP32 5.9.0

Les licences des dépendances et les avis de copyright contenus dans les fichiers tiers doivent être conservés. Ne pas déduire la licence de chaque fichier uniquement de la licence MIT du dépôt principal.

## Licence

Le fichier `firmware/LICENSE` contient la licence MIT du dépôt amont et son avis de copyright. Le dépôt amont précise que certains fichiers sous `lib/` et `src/sha512_crypt.*` sont des composants tiers soumis à leurs licences d'origine. Conserver leurs en-têtes, notices et fichiers de licence.

## Adaptations ROZA attendues

1. Interface et documentation progressivement francisées, sans supprimer les applications fonctionnelles importées.
2. Wi-Fi et Bluetooth arrêtés au démarrage ; activation uniquement sur demande explicite.
3. LoRa en réception par défaut ; émission uniquement après action volontaire clairement visible.
4. GPS utilisable hors ligne.
5. Aucune commande de formatage automatique de la microSD ; identifier le système de fichiers avant de décider du montage et des écritures.
6. Aucune clé API, aucun mot de passe ni identifiant privé dans Git.
7. Respect des limites matérielles : ESP32-S3FN8, 8 Mo Flash, pas de PSRAM.
8. Aucun test matériel ne doit être déclaré sans essai physique réel.

## Vérifications restantes

- Réaliser des essais physiques sur le Cardputer ADV réel : démarrage, clavier, écran, audio, GNSS, microSD et stabilité mémoire.
- Tester physiquement la réception LoRa avec le Cap SX1262, une antenne adaptée et un signal de test autorisé ; vérifier aussi la coexistence SPI avec la microSD.
- Effectuer un audit exhaustif de toutes les applications et de tous les chemins d'activation Wi-Fi/Bluetooth/LoRa. Les contrôles du démarrage principal ne remplacent pas cet audit complet.
- Identifier le système de fichiers réellement utilisé par la microSD 128 Go avant toute décision d'écriture ou de formatage. Aucun formatage automatique ne doit être ajouté.
- Vérifier les obligations réglementaires françaises et européennes avant de définir ou d'activer les paramètres d'émission LoRa.
- Examiner les avertissements de compilation LibSSH-ESP32 sans modifier le comportement ni les sources tierces sans justification.
