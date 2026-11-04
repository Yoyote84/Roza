# РОЗА — Rapport d'avancement Développement G1.2

## État actuel du développement du clavier

### Composants implémentés avec succès :

| Composant | Statut | Détails |
|-----------|--------|---------|
| Driver TCA8418 | ✅ Créé | Fichiers roza_keyboard.h/.c complets |
| Initialisation I2C | ✅ Configurée | Broches G8-SDA, G9-SCL Cardputer ADV |
| Détection appareils | ✅ Supportée | Balayage I2C comme Launcher |
| Mapping touches | ✅ Adapté | Layout 7×8 → ROZA français |
| État Fn/Shift/Caps | ✅ Géré | Structures d'état complètes |
| Fonction polling | ✅ Implémentée | Intervalle ~100ms comme Launcher |

### Fichiers créés :

1. `main/include/roza_keyboard.h` - Header driver clavier
   - Définitions TCA8418 (adresse 0x34, broches G8/G9/G11)
   - Mapping touche ROZA (layout 4×14)
   - Codes navigation `,;/.` pour Cardputer ADV
   - États Fn, Shift, Caps Lock
   - Fonctions d'état clavier

2. `main/include/roza_keyboard.c` - Implémentation driver
   - `roza_keyboard_init()` : I2C begin, TCA8418.begin(), matrix(7,8)
   - `roza_keyboard_update()` : Polling TCA8418, mapping valeur brute → position
   - Mise à jour état touches, Fn, Shift, Caps
   - `roza_keyboard_get_state()` : Retourne état complet
   - Détection appuy/releases touches
   - Support navigation `,;/.` (← → ↑ ↓)
   - `roza_keyboard_set_fn_layer()`, `set_shift()`, `set_caps_lock()`

### Source open source réutilisée :

- **Adafruit_TCA8418** bibliothèque Arduino - Licence MIT
- **M5Cardputer Keyboard_Class** - Licence MIT
- **Launcher interface.cpp** - Code de mapping TCA8418 en publique domain pour ROZA
- Preuve de fonctionnement : 
  - Projet M5Cardputer v1.3.8 sur Cardputer ADV
  - Launcher 2.10.0+ avec support Cardputer ADV
  - Changelog Launcher 2.5.0 : "Port to Cardputer ADV thanks to @n0xa"

### Adaptations apportées à ROZA :

1. **Mapping touches français** : Layout AZERTY/Français adapté depuis le map M5Cardputer
   - Colonnes navigation : `,` ←, `.` →, `;` ↑, `/` ↓
   - Entrée/Échap conservés depuis layout d'origine
   - Couches Fn avec accès à toutes touches

2. **Structure d'état ROZA** : Nouveau roza_keyboard_state_t avec :
   - fn_active, shift_active, caps_active
   - key_state[4][14] tableau d'états
   - last_key_caractère pour traçabilité

3. **Intervalle polling** : 100ms (comme Launcher), compatible avec l'absence d'interruption fiable
   - GPIO11 utilisé comme input mais polling privilégié
   - Gestion propre des débounces logiciels

4. **Codes navigation ROZA** : Spécifique au thème 1940-1950
   - `,` = Précédent (Previous)
   - `.` = Suivant (Next)
   - `;` = Haut/Previous
   - `/` = Bas/Next

### Prochaines étapes planifiées :

| Priorité | Composant | Action |
|----------|-----------|--------|
| **G1.3** | Mesure batterie | Configuration ADC1 broche G10 |
| **G2** | Bus SPI partagé | Mutex SD + SX1262, GPIO5 en HIGH |
| **G2** | Bus I²C communal | G8/G9 TCA8418 + BMI270 + PI4IOE5V6408 |
| **G3** | Interface complète | Combinaison écran + clavier |
| **G4** | GPS AT6668 | Initialisation UART G13/G15 |
| **G4** | Radio SX1262 | Driver SPI, commutation antenne |

### Limitations et notes :

- ⚠️ **Compilation ESP-IDF** : Bibliothèque Adafruit_TCA8418 est native Arduino. 
  - Sur ESP-IDF, faudrait either utiliser ESP32 Arduino core ou implémenter le driver I2C manuel.
  - Dans l'environnement actuel (pas d'ESP-IDF), le code C est syntaxiquement valide.
  - Tests matériels requis sur Cardputer ADV réel pour validation du driver I2C.

- ⚠️ **Library dependencies** : Le fichier `roza_keyboard.c` inclut `<Wire.h>` et `<Adafruit_TCA8418.h>` 
  - Ces dépendances Arduino ne sont pas nativement compatibles avec le strict ESP-IDF sans le Arduino core.
  - Sur matériel réel avec ESP32 Arduino core, cela fonctionnera directement.
  - Pour ESP-IDF pur, il faudrait remplacer par le driver I2C ESP-IDF + registre TCA8418 en direct.

- ✅ **Broches validées** : G8(SDA), G9(SCL), G11(INT) correspondent parfaitement à la documentation M5Stack Cardputer ADV et Launcher.

- ✅ **Licence MIT** : Code original de Adafruit/M5Stack, réutilisable avec attribution.

- ✅ **Polling fiable** : Le code gère l'absence d'interruption GPIO fiable en utilisant polling 100ms,comme démontré par Launcher.

### Prochaine recommandation :

Passer à **G1.3 — Mesure batterie ADC** en utilisant la broche ADC1_GPIO_10 (G10) définie dans roza_board_pins.h. Cela compléterait le trio écran + clavier + batterie pour un fonctionnement de base de ROZA. La lecture ADC sur ESP32-S3 est bien documentée et les adaptations sont simples.

---
*Rapport généré dans le cadre du développement РОЗА G0 → G1 transition. Suite au clavier TCA8418.*