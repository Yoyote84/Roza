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

## Vérifications encore nécessaires

- Compiler réellement avec PlatformIO sur CI et examiner les erreurs jusqu'à obtenir une compilation propre.
- Vérifier la taille du binaire et les marges de Flash/RAM ; ne pas confondre compilation réussie et validation matérielle.
- Auditer tous les chemins de démarrage et toutes les applications pour confirmer que Wi-Fi/Bluetooth/LoRa ne s'activent pas automatiquement.
- Identifier le format réel de la microSD 128 Go avant d'activer son montage/écriture sur l'appareil.
- Vérifier les obligations réglementaires françaises/européennes avant de définir les paramètres d'émission LoRa.
