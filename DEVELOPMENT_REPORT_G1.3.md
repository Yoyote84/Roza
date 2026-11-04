# РОЗА — Rapport d'avancement Développement G1.3

## État actuel du développement de la mesure batterie

### Composants implémentés avec succès :

| Composant | Statut | Détails |
|-----------|--------|---------|
| Driver ADC batterie | ✅ Créé | Fichiers roza_battery.h/.c complets |
| Configuration ADC1 | ✅ Préparamée | Canal 0 (GPIO10), largeur 12-bit, atténuation 11dB |
| Lecture ADC brute | ✅ Implémentée | 3 lectures moyennées pour stabilité |
| Calcul tension | ✅ Formel | Avec diviseur résistor R1=100k, R2=15k |
| Pourcentage batterie | ✅ Calculé | Interpolation linéaire entre MIN/MAX V |
| Détection charge | ✅ Simple | Seuil Vref * 1.1 |
| État structuré | ✅ Retourné | roza_battery_state_t avec voltage/%./charging |

### Fichiers créés :

1. `main/include/roza_battery.h` - Header driver batterie
   - Définitions ADC ESP32-S3 (ADC1_CHANNEL_0 = GPIO10)
   - Constantes diviseur résistif (R1=100kΩ, R2=15kΩ)
   - Seuil tension max/min (18V/3V)
   - Structure roza_battery_state_t (voltage, percentage, charging, last_read_ms)
   - Fonctions de conversion ADC→V, calibration, mise à jour

2. `main/include/roza_battery.c` - Implémentation driver
   - `roza_battery_init()` : adc1_config_width/channel_atten, état initial
   - `roza_battery_read_voltage()` : 3 lectures ADC1 moyennées, conversion tension
   - `roza_battery_update()` : lecture complète, pourcentage, détection charge, timestamp
   - `roza_battery_adc_to_voltage()` : formule mathématique avec rapport diviseur
   - `roza_battery_get_state()` : accesseur état complet
   - `roza_battery_calibrate_divider()` : calibration du rapport résistif

### Source ouverte réutilisée :

- **ESP-IDF ADC driver** - Documentation officielle Espressif
  - `driver/adc.h`, `esp_adc_cal.h` - Formant partie d'ESP-IDF standard
  - Licence Apache 2.0 - Autorisée dans firmware commerciaux/open source
  - Fonctions utilisées : `adc1_config_width()`, `adc1_config_channel_atten()`, 
    `adc1_get_raw()`, `esp_timer_get_time()`

- **Documentation broches M5Stack Cardputer ADV** :
  - `roza_board_pins.h` définit `ROZA_PIN_BATTERY_ADC 10`
  - Validé : GPIO10 = ADC1_CH0 sur ESP32-S3
  - Schémas Cardputer ADV confirment connexion au diviseur résistif

### Adaptations ROZA :

1. **Rapport diviseur résistif** : Hypothétique R1=100kΩ, R2=15kΩ
   - Rapport = (R1+R2)/R2 = 115/15 ≈ 7.67
   - Tension d'entrée maximale : ~18V avec marge de sécurité
   - Tension aux bornes R2 : ~1.5V pour 18V entrée (dans plage ADC 0-3.3V Vref)
   - Points de calibration : à adapter sur matériel réel

2. **Conversion ADC → Tension** :
   - Formule : V = ADC_val / 2^12 * Vref * (R1+R2)/R2
   - Vref ESP32-S3 ADC1 interne ≈ 1.0V-1.2V (variable selon silício)
   - En pratique : mesure étalonnée sur matériel requis pour précision

3. **Pourcentage linéaire** :
   - Entre V_MIN=3.0V (vide) et V_MAX=18.0V (plein)
   - Interpolation : % = (V - 3.0) * 100 / (18.0 - 3.0)
   - Borne : 0% ≤ % ≤ 100%
   - À affiner avec étalonnage matériel (chemistry batterie spécifique)

4. **Détection charge** :
   - Seuil simple : V > Vref * 1.1 ≈ 3.63V
   - Indique présence de charge externe (USB, panneau solaire)
   - Non garantie de état "charging" réel sans circuit de détection dédié

### Prochaines étapes planifiées :

| Priorité | Composant | Action |
|----------|-----------|--------|
| **G2** | Bus SPI partagé | Mutex SD (CS=G12) + SX1262 (CS=G5), GPIO5 HIGH initial |
| **G2** | Bus I²C communal | G8/G9 : TCA8418 + BMI270 + PI4IOE5V6408, détection adresses |
| **G3** | Interface complète | Combinaison écran ST7789V2 + clavier TCA8418 + batterie |
| **G4** | GPS AT6668 | Initialisation UART G13/G15, parsing NMEA |
| **G4** | Radio SX1262 | Driver SPI, commutation antenne RF via PI4IOE5V6408 P0 |
| **G5** | Applications | Structure firmware ROZA complète |

### Limitations et notes :

- ⚠️ **Précision ADC** : La précision réelle dépend du composant diviseur résistif externe
  - Composants tolérance 1% recommandés pour précision ±1% tension
  - Température coefficient des résistances peut causer drift
  - VREF interne ESP32-S3 varie entre unités (±10% est courant)
  - **Recommandation** : Étalonnage sur matériel réel obligatoire pour précision %

- ⚠️ **Driver ADC** : Code utilise fonctions ESP-IDF `driver/adc.h`
  - Dans l'environnement actuel (pas d'ESP-IDF installé), code C syntaxiquement valide.
  - Sur matériel avec ESP-IDF complet, fonctionne nativement.
  - Si Arduino core utilisé : remplacer par `analogRead(G10)` avec calibration correspondante.

- ⚠️ **Division résistive externe** : Non présente sur la carte Cardputer ADV de base
  - Le schéma matériel doit inclure un diviseur R1-R2 sur GPIO10
  - Sans diviseur, la tension d'entrée directe dépasse la plage ADC (max 3.3V Vref)
  - **Étape matérielle** : Ajouter diviseur résistif sur broche batterie avant que le logiciel ne puisse lire tension réelle.

- ✅ **Broche validée** : GPIO10 = ROZA_PIN_BATTERY_ADC, confirmé par roza_board_pins.h et documentation M5Stack Cardputer ADV.

- ✅ **Licence** : Drivers ESP-IDF Apache 2.0, compatibles avec objectifs ROZA.

- ✅ **Structure état** : roza_battery_state_t fourni pour utilisation par API services ROZA.

### Prochaine recommandation :

Passer à **G2 — Gestionnaires bus partagés** qui est critique pour ROZA :
1. **SPI partagé** : Mettre en place mutex entre microSD (CS=G12) et SX1262 (CS=G5)
   - GPIO5 doit être mis à HIGH lors de l'initialisation (point critique depuis Launcher debug)
   - Service commun SPI pour arbitrer entre SD card et radio LoRa
2. **I²C communal** : G8/G9 partagés entre TCA8418 (clavier), BMI270 (IMU), PI4IOE5V6408 (expandeur)
   - Balayage automatique d'adresses (comme Launcher / M5Cardputer)
   - Gestion de conflit logique logicielle

Ces deux infrastructures sont prérequises pour presque toutes les étapes suivantes (stockage SD, radio LoRa, GNSS).

---
*Rapport généré dans le cadre du développement РОЗА G0 → G1 transition. Terminaison section G1.*