# ROZA — Matrice de migration préparatoire pour l'étape 2/5

**Mise à jour :** 25 novembre 2026  
**Branche de référence :** `main`  
**Commit audité :** `71cd1ff0216ec426f8d073153c838373181da5b9`  
**Dossier complet :** [Audit étape 1/5](FIRMWARE_REFRESH_STEP1_AUDIT.md)

> Cette matrice est un dossier de préparation. Elle ne signifie pas que les versions cibles sont déjà intégrées ou compatibles. Aucun code firmware ni dépendance n'est modifié ici.

## 1. Point de départ réel

| Élément | Version active observée dans main |
|---|---|
| PlatformIO Core en CI | 6.2.0 |
| Plateforme | espressif32 8.0.0 |
| Arduino-ESP32 | 3.3.12 |
| ESP-IDF sous Arduino | 5.5.5 |
| M5Unified / M5GFX | 0.3.0 / 0.3.0 |
| NimBLE-Arduino | 2.5.2 |
| RadioLib | 7.7.1 |
| FastLED | 3.10.3 |
| ArduinoJson | 7.4.3 |
| TinyGPSPlus | 1.1.0 déclaré par PlatformIO |
| ESP8266Audio | tag Git 2.4.3 |
| LibSSH-ESP32 | 5.9.0 |
| SD | Arduino SD / SPI partagé avec SX1262 |

La CI du commit de départ échoue dans Wardrive sur le symbole interne retiré `STA_STARTED_BIT`. La compilation échouée interdit d'obtenir une mesure Flash/RAM à jour. Corriger cette API privée fait partie du premier lot de l'étape 2.

## 2. Cible du framework

| Candidate | Date officielle | Avantage | Risque | Décision |
|---|---|---|---|---|
| Arduino 3.3.13 / IDF 5.5.6, pioarduino 55.03.313 | 17 novembre 2026 | saut incrémental depuis 3.3.12 | changement du framework et du packaging plateforme | première cible comparative pour isoler les régressions |
| Arduino 4.0.0 / IDF 6.1.1, pioarduino 61.04.00 | 19 novembre 2026 | core stable le plus récent vérifié | saut majeur, API natives, BLE, I²S, Wi-Fi, outils et bibliothèques | objectif moderne prioritaire, à adopter seulement après CI complète et non-régression |
| PlatformIO Espressif32 8.0.0 | base actuelle | point de comparaison reproductible | ne fournit pas le niveau Arduino 4.0.0 | conserver comme référence de comparaison |

Ne pas confondre pioarduino, qui est une plateforme communautaire, avec la plateforme officielle PlatformIO ou le dépôt Arduino-ESP32 d'Espressif. Ne pas transformer le projet actif en ESP-IDF natif : le workflow et les usages ROZA restent PlatformIO + Arduino.

## 3. Matrice des bibliothèques

| Composant | Actuel | Candidate stable au 25/11/2026 | Risque | Action proposée |
|---|---:|---:|---|---|
| M5Unified | 0.3.0 | 0.3.0 | moyen | conserver ; vérifier rotation, audio, détection carte et API bool |
| M5GFX | 0.3.0 | 0.3.0 | moyen | conserver ; mesurer rendu/DMA et rotation Cardputer ADV |
| NimBLE-Arduino | 2.5.2 | 2.5.2 | moyen | conserver ; retester EXT_ADV, PHY Coded, HID et heap |
| RadioLib | 7.7.1 | 8.0.0 | élevé | adapter les appels SX1262 à la nouvelle configuration et aux IRQ ; préserver les fonctions |
| FastLED | 3.10.3 | 3.10.10 | moyen | tester en isolation ; vérifier GPIO21, RMT, audio et tailles mémoire |
| ArduinoJson | 7.4.3 | 7.4.3 | faible | conserver ; renforcer les tests des flux et des tableaux temporaires |
| TinyGPSPlus | 1.1.0 | provenance à clarifier | moyen | garder le parseur et ses champs personnalisés ; ne pas changer sans parité NMEA démontrée |
| ESP8266Audio | 1.9.9 | 2.4.3 | élevé | Intégré dans `main` ; la CI doit confirmer le filtrage ULP/DAC sur ESP32-S3 et la non-régression MP3/streaming |
| LibSSH-ESP32 | 5.9.0 | pas de release GitHub identifiée | élevé | valider le core moderne ; comparer wolfSSH sans perte client/SCP ni licence non vérifiée |
| SdFat | absent | 2.3.1 | élevé | prototype sur carte de test FAT/exFAT ; ne pas remplacer avant lecture non destructive et arbitrage SPI |

## 4. État des lots d'intégration

- **Audio — 2.4.3 intégré dans `main`** : suppression du contournement `SafeMP3`, conservation des allocations préallouées et du flux HTTP sur mesure. Validation CI en attente.
- Framework 3.3.13/5.5.6, RadioLib 8.0.0 et FastLED 3.10.10 restent à intégrer après rétablissement d'un build stable.

## 5. Lots d'intégration proposés

### Lot A — rendre la base actuelle compilable
1. Remplacer l'usage de `STA_STARTED_BIT` dans Wardrive par les API publiques de gestion Wi-Fi adaptées au core réel.
2. Contrôler les appels d'attente Wi-Fi et leur timeout, sans dépendance aux symboles internes.
3. Traiter les avertissements de troncature et référence JSON potentiellement pendante.
4. Obtenir CI verte sur le commit exact, avec tests logiciels et images de build.
5. Enregistrer une première mesure RAM/Flash de la base actuelle pour disposer d'un vrai point de comparaison.

### Lot B — saut Arduino 3.3.13 / IDF 5.5.6
1. Créer un environnement de test isolé dans le même dépôt et sous les règles de branche main, ou rétablir la version exacte après comparaison ; ne pas laisser deux bases ambiguës dans la livraison.
2. Compiler tout le firmware et l'ensemble des tests.
3. Mesurer le delta RAM, Flash, heap contigu et temps de rendu.
4. Rejouer les tests Wi-Fi, NimBLE, USB CDC, I²S, écran, clavier, IMU, SD et SX1262.
5. Adopter ce palier seulement s'il apporte un gain réel et reste stable.

### Lot C — saut Arduino 4.0.0 / IDF 6.1.1
1. Vérifier le guide de migration Arduino et IDF de toutes les API effectivement utilisées.
2. Adapter les dépendances et les couches de compatibilité nécessaires, sans stub.
3. Recompiler depuis un environnement propre et vérifier les versions résolues.
4. Comparer le résultat au palier 3.3.13 et à la base 8.0.0.
5. Conserver la version stable la plus récente qui passe l'ensemble des tests et le budget du Cardputer ADV.

### Lot D — dépendances indépendantes
- RadioLib 8.0.0 : configuration SX1262, réglages entiers, IRQ et codes retours.
- ESP8266Audio 2.4.3 : intégré ; vérifier décodeurs, ID3, I²S, buffers, streaming et coexistence micro/haut-parleur.
- FastLED 3.10.10 : RMT, SK6812 GPIO21, timing et Flash/RAM.
- LibSSH / wolfSSH : parité client/SCP, licence, mémoire TLS, compatibilité du core.
- SdFat 2.3.1 : exFAT, format réel de la carte, SPI partagé et conservation des données.

## 6. Contrôles applicatifs obligatoires

Les 40 entrées du menu doivent rester accessibles et testées :
- TOOL : Time, IMU, Files, Spectrum, Calc, Converter, Player, Hash Oven.
- SCAN : WiFi Chan, Sniffer, Wardrive, Drone ID, BLE Scan.
- NET : Hotspot, LAN Scan, NetProbe, Router, SSH, ChatGPT.
- HID : BT Keys, BT Media, Ducky.
- SIG : LoRa, GNSS, Map, IR.
- SKY : Astro, Weather, Planes, Sats, Typhoon, Quake.
- MORE : GitHub, Radio, FX, OKX, Reader, BadApple, Apps, Settings.

Les fonctions réseau doivent rester facultatives. Les transports HTTP actuels de Weather, Planes, GeoLoc et Radio doivent faire l'objet d'un lot sécurité distinct. Chat ne doit jamais autoriser le mode sans validation de certificat dans une configuration de livraison normale. Sats doit conserver un chemin sans clé API ou expliciter sa dépendance comme facultative.

## 7. Critères de fin de l'étape 2

- [ ] Compilation propre de la cible retenue et CI verte sur le SHA exact.
- [ ] Toutes les suites de tests logicielles passées.
- [ ] Versions exactes consignées et verrouillées.
- [ ] RAM/Flash et plus grand bloc contigu mesurés et comparés.
- [ ] Toutes les 40 applications compilent et conservent leur rôle.
- [ ] Aucune fonction remplacée par un stub ou une simulation.
- [ ] Wi-Fi/Bluetooth inactifs au démarrage ; LoRa en réception par défaut.
- [ ] Aucun accès destructif automatique à la microSD.
- [ ] Les essais physiques non réalisés restent explicitement non validés et sont transmis à l'étape 5.

## Références

- Arduino-ESP32 / notes de version : https://release-notes.espressif.com/?project=Arduino
- ESP-IDF 6.1.1 : https://github.com/espressif/esp-idf/releases/tag/v6.1.1
- pioarduino 61.04.00 : https://github.com/pioarduino/platform-espressif32/releases/tag/61.04.00
- pioarduino 55.03.313 : https://github.com/pioarduino/platform-espressif32/releases/tag/55.03.313
- M5Unified : https://github.com/m5stack/M5Unified/releases
- M5GFX : https://github.com/m5stack/M5GFX/releases
- NimBLE : https://github.com/h2zero/NimBLE-Arduino/releases
- RadioLib : https://github.com/jgromes/RadioLib/releases
- FastLED : https://github.com/FastLED/FastLED/releases
- ESP8266Audio : https://github.com/earlephilhower/ESP8266Audio/releases
- ArduinoJson : https://github.com/bblanchon/ArduinoJson/releases
- SdFat : https://github.com/greiman/SdFat/releases
- wolfSSH : https://github.com/wolfSSL/wolfssh/releases
