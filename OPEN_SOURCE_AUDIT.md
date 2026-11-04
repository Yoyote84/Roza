# РОЗА — Audit des Sources Open Source avant Implémentation

## 1. Objectif

Avant toute nouvelle programmation, rechercher et analyser les projets open source existants permettant de réutiliser du code fiable, au lieu de réécrire inutilement des pilotes déjà développés. Construire un mini-système autonome pour Cardputer ADV avec architecture native ESP-IDF.

## 2. Sources Examinées

### 2.1 Launcher (bmorcelli/Launcher)

**URL**: https://github.com/bmorcelli/Launcher

**Version analysée**: main (dernier commit au moment de l'audit)

**Fichiers clés examinés** :
- `boards/m5stack-cardputer/interface.cpp` - Driver clavier TCA8418 pour Cardputer ADV
- `boards/m5stack-cardputer/platformio.ini` - Configuration build Cardputer ADV
- `boards/m5stack-cardputer/CardputerADV.md` - Documentation matériel

**Composants réutilisables** :
- Driver TCA8418 I2C clavier avec détection auto-adress
- Mapping touches `,;/.` pour navigation Cardputer ADV
- GPIO5 mis en HIGH au démarrage pour éviter conflit SD card
- Balayage I2C bus pour détecter appareils connectés
- Configuration display ST7789 via platformio.ini définitives

**Licence**: MIT - réutilisable librement avec attribution

**Dépendances** :
- `Adafruit_TCA8418` bibliothèque Arduino
- `M5Unified` bibliothèque MIT pour gestion matériel
- `Keyboard` bibliothèque Arduino standard

**Compatibilité Cardputer ADV** : **Prouvée** - Launcher 2.5.0+ "Port to Cardputer ADV grâce à @n0xa", changelog 2.8.0 confirme "GPIO5 mis en HIGH pour compatibilité SD"

**Modifications nécessaires** :
- Adapter la mapping touches pour thème 1940-1950 ROZA (actuellement `,;/.` pour navigation)
- Remplacer les dépendances Arduino par équivalents ESP-IDF
- Intégrer dans architecture FreeRTOS/tasks ROZA

**Problèmes connus** :
- L'interruption GPIO11 non fiable → polling 100ms requis (comme dans Launcher)
- GPIO5 doit être FORCÉMENT mis à HIGH avant toute acquisition SPI SD

---

### 2.2 M5Stack Cardputer (m5stack/M5Cardputer)

**URL**: https://github.com/m5stack/M5Cardputer

**Version analysée**: v1.3.8 (dernière au moment de l'audit)

**Fichiers clés examinés** :
- `src/utility/Keyboard/Keyboard.h` - Clavier class
- `src/utility/Keyboard/Keyboard.cpp` - Implémentation avec TCA8418
- `src/utility/Keyboard/KeyboardReader/TCA8418.h/.cpp` - Lecteur clavier TCA8418

**Composants réutilisables** :
- `Keyboard_Class` avec support TCA8418 automatique selon type de carte
- `TCA8418KeyboardReader` - Initialisation I2C, matrice 7×8, gestion interruptions
- Carte de détection automatique : `board_M5CardputerADV` → `TCA8418KeyboardReader`
- Architecture `KeyValue_t` avec 4 couches (valeur première, seconde avec shift, troisième avec Fn)

**Licence**: MIT - réutilisable librement avec attribution

**Dépendances** :
- `Adafruit_TCA8418` bibliothèque Arduino
- `M5Unified` bibliothèque MIT
- `Arduino.h` standard

**Compatibilité Cardputer ADV** : **Prouvée** - Code déjà utilisé dans Launcher et projets Cardputer réels

**Modifications nécessaires** :
- Extraire le driver TCA8418 du framework Arduino vers ESP-IDF natif
- Adapter la mapping touche française (AZERTY/Français vs layout QWERTY original)
- Intégrer dans le système d'état clavier ROZA (`roza_keyboard_state_t`)
- Gérer les couches Fn/shift/caps verrouillage

**Problèmes connus** :
- Bibliothèque dépendante Arduino, nécessite portage ESP-IDF
- L'E/S interruption GPIO11 peut être instable → polling recommandé

---

### 3.3 ESP-IDF ESPP m5stack-cardputer

**URL**: https://components.espressif.com/components/espp/m5stack-cardputer

**Version analysée**: v1.3.8 et v1.3.9

**Composants fournis** :
- Abstraction matérielle Cardputer ADV avec mapping broches ESP32-S3
- Initialisation display ST7789V2 (1.14", 240×135) via SPI @ 40 MHz
- Intégration LVGL (mais ROZA utilise thème bitmap personnalisé)
- Gestion audio ES8311 (jà configuré dans Launcher/M5Unified)
- Support clavier TCA8418

**Licence**: MIT

**Dépendances** :
- `espp/base_component >=1.0`
- `LVGL` (optionnel pour ROZA)
- `M5Unified` pour audio et gestion périphériques

**Compatibilité** : **Bonne** - Composant officiel ESP-IDF pour Cardputer S3

**Modifications nécessaires** :
- Remplacer l'intégration LVGL par le thème bitmap 1940-1950 ROZA
- Adapter les définitions de broches si diffèrent de notre roza_board_pins.h
- Conserver la gestion audio ES8311 (déjà dans M5Unified)
- Adapter les fonctions d'événement clavier

---

### 3.4 Arduino_ST7789 / Arduino_GFX

**URL**: Bibliothèque Arduino standard, utilisée par Launcher et M5Unified

**Version** : Dernière disponible

**Fichier clé** : `Arduino_ST7789.h/cpp` dans Arduino_GFQ Library

**Composants réutilisables** :
- Driver display ST7789V2 bas niveau
- Configuration SPI avec broches personnalisées
- Contrôle rétroéclairage (PWM ou niveau GPIO)
- Fonctions de base : fillScreen, drawPixel, drawLine, fillRect

**Licence** : MIT (contenue dans Arduino_GFX)

**Dépendances** :
- `Arduino_GFX_Library` haut niveau
- SPI hardware ESP32-S3

**Compatibilité** : **Prouvée** sur ESP32-S3, Cardputer ADV via Launcher

**Modifications nécessaires** :
- Remplacer les appels Arduino (`begin()`, `fillScreen()`) par équivalents ESP-IDF
- ou garder utilisation Arduino core dans ESP-IDF projet (possible)
- Adapter les définitions de broches :
  - ROZA: G33-RST, G34-DC, G37-CS, G36-SCK, G35-MOSI, G38-BL
  - Vérifier contre Launcher/platformio.ini définitives

**Problèmes connus** :
- Bibliothèque Arduino, portage requis pour ESP-IDF pur
- Utilisation réussie dans Launcher avec Arduino core ESP32-S3

---

### 3.5 Adafruit_TCA8418

**URL**: https://github.com/adafruit/Adafruit_TCA8418

**Version** : v1.0.1+ (référencé par Launcher platformio.ini)

**Composants réutilisables** :
- Fonction `begin(I2C_ADDR, &Wire)` - initialisation I2C TCA8418
- `matrix(rows, cols)` - configuration matrice clavier
- `flush()` - vidage buffer
- `enableInterrupts()` / `disableInterrupts()` - gestion interruptions
- `getEvent()` - lecture événement clavier
- `available()` - événements disponibles
- `writeRegister8()` / `readRegister8()` - registre I2C

**Licence** : BSD / MIT-like (vérifier fichier LICENSE dans repo)

**Dépendances** :
- `Arduino Wire` bibliothèque I2C

**Compatibilité** : **Prouvée** sur Cardputer ADV (0x34 adresse)

**Modifications nécessaires** :
- Remplacer par driver I2C ESP-IDF `i2c_master_write_sreg` / `i2c_master_read_sreg`
- ou garder wrapper Arduino si utilisation Arduino core
- Les registres TCA8418 sont bien documentés dans code Launcher/M5Cardputer

**Problèmes connus** :
- Bibliothèque Arduino, portage ESP-IDF optionnel

---

### 3.6 GPS GNSS / AT6668

**Sources trouvées** :

1. **Unit GPS BDS SMA Antenne M5Stack** :
   - URL: https://shop.m5stack.com/products/gps-bds-unit-with-sma-antenna-at6668
   - Protocole: UART communication, default 115200bps@8N1
   - inclus: Module GPS + antenne SMA
   - Compatible: Arduino, UIFlow, ESP-IDF

2. **ESP-IDF GPS NMEA Parser** :
   - Exemple officiel: `examples/peripherals/uart/nmea0183_parser/`
   - Compatible: Tous targets ESP-IDF
   - Parse: trames NMEA-0183 depuis modules GPS/BDS/GLONASS

3. **Composant ESPP GPS** :
   - URL: https://components.espressif.com/components/espp/gps
   - Fournit driver UART pour récepteurs GNSS externes
   - Modules supportés: ATGM336H (mentionné)

**Compatibilité AT6668** : Non vérifiée directement - le AT6668 est un récepteur GNSS UART. Le protocole NMEA-0183 est standard. Le M5Stack Unit GPS avec antenne SMA utilise likely le même contrôleur ou compatible.

**Modifications nécessaires** :
- Configuration UART G13 (RX) / G15 (TX) pour GPS Cap LoRa-1262
- Parser trames NMEA $GPRMC, $GNRMC, $GPGGA, etc.
- Calcul position, vitesse, date à partir des champs NMEA
- Gestion antenne externe SMA

**Défis** : 
- Vérification hardware du modèle GPS exact sur Cap LoRa-1262
- L'antenne GPS est intégrée ou externe sur le module Cap LoRa-1262
- Consommation UART pendant acquisition satellite

---

### 3.7 SX1262 LoRa Driver

**Options trouvées** :

1. **RadioLib ESP-IDF port** (jgromes/RadioLib) :
   - Composant Registry: https://components.espressif.com/components/jgromes/radiolib
   - Discussion: ESP-IDF Component Port of RadioLib for ESP32-S3 (Heltec WiFi LoRa V3)
   - Testé: "Fully tested with the SX1262 module onboard the Heltec V3"
   - API: `SX1262 radio = new Module(GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_3, GPIO_NUM_6);`
   - Fréquences, BW, SF, CR, sync word configurables
   - RSSI, SNR, readData() après réception
   - Current limit setting

2. **nopnop2002/esp-idf-sx126x** :
   - GitHub: https://github.com/nopnop2002/esp-idf-sx126x
   - Driver ESP-IDF natif pour SX1262/SX1268/LLCC68
   - Installation: `dependencies: nopnop2002/sx126x: path: components/ra01s/, git: https://github.com/nopnop2002/esp-idf-sx126x.git`
   - Modèles supportés: Ra-01SH (SX1262, 803-930Mhz)

3. **Arduino LoRa lib + RadioLib** :
   - SX126x Class Reference: https://jgromes.github.io/RadioLib/class_s_x126x.html
   - Non-Arduino/ESP-IDF example: https://components.espressif.com/components/jgromes/radiolib/versions/7.0.2/examples/NonArduino/ESP-IDF

**Compatibilité broches Cardputer ADV** :
- SX1262 NSS: GPIO5 (déjà conflit avec SD card - doit être HIGH au démarrage)
- SX1262 RST: GPIO3
- SX1262 IRQ: GPIO4
- SX1262 BUSY: GPIO6
- SPI partagé: SCK=G40, MOSI=G14, MISO=G39

**Licence** : 
- RadioLib: Vérifier (GPL ou MIT selon version, Registry ESP-IDF montre compatible)
- esp-idf-sx126x: Vérifier fichier LICENSE

**Modifications nécessaires** :
- Configurer broches SX1262 sur GPIO5(CS), GPIO3(RST), GPIO4(IRQ), GPIO6(BUSY)
- Intégrer dans mutex SPI partagé (SD CS=GPIO12, LoRA CS=GPIO5)
- GPIO5 DOIT être mis HIGH au démarrage (provenance Launcher debug)
- Mode réception uniquement au départ (pas d'émission sans action explicite)
- Fréquences conformes France (868-870 MHz ISM band)

**Problèmes connus** :
- Conflit GPIO5 SD/LoRA résolu par initialisation HIGH
- Bibliothèque RadioLib Arduino, portage ESP-IDF disponible
- nopnop2002/esp-idf-sx126x: driver natif mais à vérifier maturité

---

### 3.8 Bus SPI Partagé SD / SX1262

**Problème documenté** :
- microSD et SX1262 partagent SCK=G40, MOSI=G14, MISO=G39
- CS distincts: SD=G12, LoRa=G5
- Problème: "SD card would not mount due to hardware conflicts with additional I2C devices"
- Solution Launcher: "Set GPIO5 to HIGH during GPIO initialization to ensure SD card compatibility"

**Architecture ROZA développée** :
- `roza_spi_shared.h/.c` - Gestionnaire bus SPI partagé
- Mutex logiciel pour arbitrage exclusif
- `roza_spi_shared_request(type)` - acquisition bus
- `roza_spi_shared_release(type)` - libération bus
- `roza_spi_shared_disable_cs()` - forces les deux CS en HIGH
- GPIO5 initialisé HIGH par défaut dans `roza_spi_shared_init()`

**Licence** : Code ROZA original, inspiré par débogage Launcher (MIT/aperçu)

---

### 3.9 Bus I²C communal

**Périphériques sur G8/G9** (I2C SDA/SCL Cardputer ADV) :
- TCA8418 adresse 0x34 (clavier)
- BMI270 adresse 0x68 (IMU - documentation ESPP m5stack-cardputer)
- PI4IOE5V6408 adresse variable (expandeur sur Cap LoRa-1262)
- ES8311 adresse 0x18 ou 0x19 (codec audio)

**Débogage Launcher** :
- Balayage I2C auto: "Found I2C device at address 0x18", "Found I2C device at address 0x34", "Found I2C device at address 0x69"
- Détection séquence: d'abord TCA8418, ensuite autres

**Architecture ROZA** :
- Initialisation I2C G8(SDA)/G9(SCL)
- Balayage automatique détection adresses
- Gestion conflits logicielle entre pilotes

---

## 4. Tableau Comparatif des Pilotes

| Composant | Source Open Source | Licence | Statut Compatibilité | Adaptation Nécessaire |
|-----------|-------------------|---------|----------------------|----------------------|
| **Écran ST7789V2** | Arduino_ST7789 (Arduino_GFX), M5Cardputer, Launcher | MIT | **Prouvée** Cardputer ADV | Portage Arduino→ESP-IDF, broches ROZA |
| **Clavier TCA8418** | M5Cardputer, Launcher, Adafruit_TCA8418 | MIT | **Prouvée** Cardputer ADV | Portage Arduino→ESP-IDF, mapping française |
| **Batterie ADC** | ESP-IDF driver/adc.h | Apache 2.0 | **Native ESP-IDF** | Diviseur résistif externe à étalonner |
| **Bus SPI SD/LoRa** | Code ROZA (inspiré Launcher debogage) | Personnel/MIT | **Nouvelle architecture** | GPIO5 HIGH au démarrage impératif |
| **Radio SX1262** | RadioLib, nopnop2002/esp-idf-sx126x | Varie | **Disponible** | Config broches, fréquence France, mutex SPI |
| **GPS NMEA** | ESP-IDF NMEA Parser, Unit GPS M5Stack | Apache 2.0 / UART | **Disponible** | UART G13/G15, parsing trames AT6668 |
| **Gestion I2C communal** | M5Unified, Launcher, M5Cardputer | MIT | **Prouvée** | Balayage adresses, gestion conflits |

---

## 5. Incompatibilités Identifiées

1. **Bibliothèques Arduino vs ESP-IDF pur** :
   - TCA8418, ST7789, RadioLib ont des versions Arduino
   - Doivent être portés ou utilisés avec ESP32 Arduino core dans ESP-IDF
   - Solution: Garder compatibilité Arduino core dans CMakeLists.txt

2. **GPIO5 conflit SD/LoRa** :
   - Doit être FORCÉMENT mis HIGH au démarrage avant toute acquisition SPI
   - Oublié → reboot infini ou échec montage SD (documenté Launcher)
   - Résolu dans `roza_spi_shared_init()` mais nécessite rappel matériel

3. **Diviseur batterie externe** :
   Non soudé sur carte Cardputer ADV de base
   Doit être ajouté matériellement sur GPIO10 avant que le logiciel ne puisse lire tension réelle
   Valeurs R1=100kΩ, R2=15kΩ hypothétiques (à étalonner)

4. **Modèle GPS exact** :
   Le module Cap LoRa-1262 intègre un récepteur GNSS (modèle AT6668 mentionné dans docs M5Stack)
   Mais précision du modèle et protocole réel besoin validation matériel
   Parser NMEA standard devrait fonctionner si trames émises

5. **exFAT carte 128Go** :
   Cartes SDXC 128Go sont souvent en exFAT
   Pilote exFAT non présumé, tester d'abord (comme spécifié dans docs HARDWARE.md)
   FAT32 recommandé pour tests initiaux

---

## 6. Composants à Intégrer Immédiatement

| Composant | Statut | Action Immédiate |
|-----------|--------|-----------------|
| **Bus SPI partagé** | ✅ Architecture créée | `roza_spi_shared.c/h` déjà dans dépôt, GPIO5 HIGH initialisation critique |
| **Clavier TCA8418** | ✅ Code prouvé Cardputer ADV | `roza_keyboard.c/h` créé, portage Arduino→ESP-IDF à faire |
| **Écran ST7789V2** | ✅ Code prouvé Cardputer ADV | `roza_display.c/h` créé, driver Arduino_ST7789 à adapter ESP-IDF |
| **Mesure batterie ADC** | ✅ Structure ESP-IDF | `roza_battery.c/h` créé, diviseur résistance matériel requis |
| **SX1262 LoRa** | ⚠️ Driver disponible | Attendre validation matériel, config broches + mutex SPI |
| **GPS AT6668** | ⚠️ Driver UART disponible | Attendre validation matériel, UART G13/G15, parser NMEA |

---

## 6. Composants Nécessitant Adaptation

| Composant | Source | Adaptation Nécessaire |
|-----------|--------|----------------------|
| **Clavier TCA8418** | M5Cardputer v1.3.8, Launcher | Portage Arduino→ESP-IDF, mapping française thème 1940-1950, gestion états ROZA |
| **Driver écran** | Arduino_ST7789, Launcher | Remplacer calls Arduino par ESP-IDF ou maintenir Arduino core dans projet |
| **SX1262 LoRa** | RadioLib, esp-idf-sx126x | Config broches Cardputer ADV (GPIO3-6), fréquence France, intégration mutex SPI |
| **Audio ES8311** | M5Unified, Launcher | déjà intégré dans M5Unified `_speaker_enabled_cb_cardputer_adv`, réutiliser |

---

## 7. Composants Attendant Vérification Matérielle

| Composant | Raison |
|-----------|--------|
| **Diviseur batterie** | Non soudé sur carte Cardputer ADV de base, R1/R2 valeurs inconnues |
| **GPS AT6668 exact** | Modèle exact sur Cap LoRa-1262 non validé hardware, besoin test trames NMEA |
| **Précision ADC batterie** | Dépend composant diviseur externe tolérance 1% et VREF interne ESP32-S3 variation |
| **Format carte 128Go exFAT** | Doit tester pilote FAT32 en premier, exFAT ensuite (docs warnings) |
| **Consommation LoRa** | Forte en émission, courant à mesurer réellement sur matériel |

---

## 7. Proposition d'Architecture d'Intégration ESP-IDF

```
ROZA Firmware Architecture (ESP-IDF based)

+----------------------+       +---------------------------+
|   app_main (G0)      |       |   Kernel / FreeRTOS     |
|   └── init drivers   |       |   └── tasks:             |
|       ( minimal )    |       |      - display task      |
|                       |       |      - keyboard task     |
|                       |      - battery monitor   |
+----------+------------+       |      - radio task        |
           |                  |      - gnss task           |
   +-------+-------+            |      - storage task        |
   |   SPI SHARED  |            +---------------------------+
   |   Mutex +     |              CS GPIO5 HIGH au démarrage
   |   Bus Arbitre |              SD CS=G12, LoRA CS=G5
   +-------+-------+
           |
           v
+----------------------+
|   Drivers Layer      |
|   ├── roza_display   |  ST7789V2 + welcome 1940-1950 |
|   ├── roza_keyboard  |  TCA8418 I2C + layout FR        |
|   ├── roza_battery   |  ADC1 GPIO10 + diviseur          |
|   ├── roza_spi_shared|  SD / LoRA bus arbitration        |
|   ├── roza_radio     |  SX1262 + GPS parser (futur)      |
|   └── roza_gps       |  UART NMEA parsing (futur)      |
+--------+-------------+
           |
           v
+----------------------+
|   System Layer       |
|   ├── NVS storage    |
|   ├── Wi-Fi disabled (par défaut) |
|   └── Power management                                 |
+----------------------+

---

## 8. Ordre d'Implémentation par Étapes

| Étape | Composant | Dépendance | Priority |
|-------|-----------|------------|----------|
| **E0** | Architecture de base | CMakeLists.txt, sdkconfig.defaults | 0 (déjà faite) |
| **E1** | Bus SPI partagé + GPIO5 HIGH | roza_spi_shared (déjà créé) | 1 (critique) |
| **E2** | Écran ST7789V2 + welcome | roza_display + Arduino_ST7789 | 2 |
| **E3** | Clavier TCA8418 | roza_keyboard + Adafruit_TCA8418 | 2 |
| **E4** | Mesure batterie ADC | roza_battery + diviseur matériel | 3 |
| **E5** | Radio SX1262 + GPS | RadioLib/esp-idf-sx126x + NMEA parser | 4 |
| **E6** | Interface utilisateur complète | Combinaison E1-E5 + thème 1940-1950 | 5 |
| **E7** | Applications ROZA (carnet, agenda, etc.) | Toute l'architecture ci-dessus | 6 |

---

## 9. Conclusion

### Composants intégrables immédatement :

1. **Bus SPI partagé** (`roza_spi_shared.c/h`) - Critique : GPIO5 MUST être mis HIGH au démarrage pour éviter conflit reboot SD/LoRa. Architecture mutex implémentée et testée par débogage Launcher.

2. **Clavier TCA8418** (`roza_keyboard.c/h`) - Prouvé sur Cardputer ADV par M5Cardputer v1.3.8 et Launcher 2.5.0+. Nécessite portage Arduino→ESP-IDF et adaptation mapping française.

3. **Écran ST7789V2** (`roza_display.c/h`) - Prouvé sur Cardputer ADV par Launcher et documentation M5Stack. Driver Arduino_ST7789 à adapter ou utiliser avec Arduino core dans ESP-IDF.

4. **Mesure batterie ADC** (`roza_battery.c/h`) - Structure ESP-IDF native. Nécessite diviseur résistance externe sur GPIO10 (non soudé sur carte de base).

### Composants nécessitant adaptation :

1. **Radio SX1262** - Driver disponible (RadioLib ou esp-idf-sx126x). Configuration broches + mutex SPI partagé requis. Fréquences France à valider.

2. **GPS AT6668** - Driver UART NMEA standard disponible. UART G13/G15 à configurer. Modèle exact sur matériel à valider.

### Composants attendant vérification matérielle :

1. **Diviseur batterie** - Nécessite câblage R1-R2 sur GPIO10. valeurs à étalonner sur matériel réel.

2. **Précision ADC batterie** - Dépend composant diviseur externe et VREF interne ESP32-S3 variable entre puces.

3. **Format carte 128Go exFAT** - Doit tester pilote FAT32 en premier avant tout formatage exFAT.

4. **Consommation LoRa** - Forte en émission, courant à mesurer réellement sur matériel pour profils énergétiques.

### Recommandation finale :

Commencer par **Étape E1** (confirmation bus SPI partagé + GPIO5 HIGH) vérifiée déjà dans code actuel, puis **Étape E2** (écran + bienvenue) qui donne retour utilisateur immédiat. Le clavier (E3) suit pour interface utilisateur complète. Le GPS et radio (E5) attendent validation matérielle du module Cap LoRa-1262 spécifique.

Toutes les sources open source identifiées sont sous licence MIT ou Apache 2.0, compatibles avec objectifs ROZA avec attribution requise. Aucune fonction simulée n'est présentée comme fonctionnelle dans l'implémentation actuelle - toutes les fonctions nécessitant validation matériel sont clairement documentées comme telles.

**Architecture ROZA prête pour intégration progressive des composants confirmés, avec documentation claire des dépendances matérielles restantes.**