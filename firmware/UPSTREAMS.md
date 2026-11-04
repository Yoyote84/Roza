# Provenance des composants — Import cardputer-adv-firmware

## Dépôt source
- **Dépôt** : https://github.com/Lewis-Peter/cardputer-adv-firmware
- **SHA d'importation** : `715c97c0dad22fa44a80a6e1e00053264f252c24` (référence au commit synchronisé sur origin/main de Yoyote84/Roza)
- **Date d'importation** : 2026-11-04

## Composants réutilisés tels quels

### BSP et bibliothèques M5Stack
- **espp/m5stack-cardputer** version **1.3.11** (via idf_component.yml)
  - Licence : MIT
  - Fournit l'abstraction matérielle : ST7789V2/LVGL, TCA8418, BMI270, batterie, microSD, SX1262 et GNSS du Cap
  - Dépendances transitives gérées par ESP Component Manager
- **m5stack/M5Unified** @ 0.2.20
  - Licence : MIT
  - Abstraction matérielle unifiée pour M5Stack
- **m5stack/M5GFX** @ 0.2.27
  - Licence : MIT
  - Pilote d'écran graphique

### Bibliothèques tierces (versions exactes)
- **mikalhart/TinyGPSPlus** @ 1.1.0 — Licence MIT
  - Parsing des sentences NMEA GPS/Glonass/Galileo/Béidou/QZSS
  - Utilisée dans `src/gnss.cpp`
- **bblanchon/ArduinoJson** @ 7.4.3 — Licence MIT
  - Manipulation JSON pour config, météo, géolocalisation
  - Utilisée dans de nombreux fichiers dont `config.http_json.h`, `geoloc.cpp`, `weather.cpp`, etc.
- **jgromes/RadioLib** @ 7.7.1 — Licence BSD
  - Pilote SX1262 LoRa dans `src/lora.cpp`
- **fastled/FastLED** @ 3.10.3 — Licence MIT
  - Pilotage LED RVB sur GPIO21
- **earlephilhower/ESP8266Audio** (GitHub #1.9.9) — Licence MIT
  - Décodeur MP3/I2S dans `src/radio.cpp`
  - Requiert lien GitHub direct (non disponible sur PlatformIO Registry)
- **ewpa/LibSSH-ESP32** @ 5.9.0 — Licence MIT
  - SSH client pour les applications réseau

### Fichiers de configuration et de système
- **platformio.ini** — Configuration PlatformIO avec dépendances épinglées
  - Platform : espressif32@7.0.1
  - Board : esp32-s3-devkitc-1
  - Framework : arduino
- **HARDWARE.md** — Référence de matériel Cardputer ADV avec caveats
  - GPIO5 doit être HIGH au démarrage pour compatibilité carte SD
  - Bus SPI partagé microSD/LoRa (NSS différent : GPIO12 vs GPIO5)
  - GPS NMEA buffer : `setRxBufferSize(4096)` avant `begin()` obligatoire
- **data/cert/x509_crt_bundle.bin** — Bundle de 121 certificats racines Mozilla
  - Inscrit en dur pour vérification HTTPS authentique
- **docs/** — Documentation technique (20 fichiers dont dependency-pins.md, feature-inventory.md, etc.)

## Adaptations effectuées pour ROZA

### Identité ROZA préservée
- Nom du projet : **РОЗА** (ROZA)
- Attribution : Lewis-Peter pour le firmware technique, ROZA pour l'identité visuelle et les contraintes non négociables
- Style visuel : 1940-1950 (à personnaliser dans les ressources data/ui/)

### Contraintes non négociables ROZA
1. **Wi-Fi / Bluetooth** : Désactivés par défaut au démarrage. Activables à la demande par l'utilisateur via l'interface.
2. **LoRa** : Mode réception par défaut.émission seulement sur action explicive de l'utilisateur.
3. **GPS** : Capable de fonctionner hors ligne. Les données de position sont disponibles sans connexion Internet.
4. **microSD** : Aucune formatage automatique. La carte doit être reconnue et montée par l'utilisateur.
5. **Interface** : Français (UI progressif). Toutes les nouvelles applications doivent respecter la langue française.
6. **Aucune clé API** dans Git. Les utilisateurs créent leur propre `src/secrets.h` locale.

### Fichiers NE JAMAIS importés (interdits)
- `src/secrets.h` — Doit être créé par l'utilisateur localement avec ses propres clés API
- `src/config_local.h` — Configuration locale utilisateur, non versionnée
- Dépôt `.git` du projet source
- Tout fichier contenant des clés API réelles ou des identifiants privés

### Prochainement à adapter (G2-G5)
- G2 : Batterie et stockage microSD (vérification formats, pas de formatage auto)
- G3 : Interface ROZA et navigation clavier
- G4 : GNSS AT6668 et SX1262 (réception d'abord, émission validée)
- G5 : Applications hors ligne : carnet, notes, journal, agenda, outils