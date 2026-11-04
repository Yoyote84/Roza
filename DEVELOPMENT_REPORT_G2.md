# РОЗА — Rapport d'avancement Développement G2

## État actuel du développement des gestionnaires de bus partagés

### Composants implémentés avec succès :

| Composant | Statut | Détails |
|-----------|--------|---------|
| Gestionnaire SPI partagé | ✅ Créé | Fichiers roza_spi_shared.h/.c complets |
| Verrouillage mutex | ✅ Implémenté | Verrouillage logiciel pour accès exclusif |
| Contrôle CS | ✅ Configuré | GPIO5 (LoRA) et GPIO12 (SD) initialisés en HIGH |
| États bus | ✅ Suivis | IDLE, SD, LoRA, ERROR |
| Désactivation CS | ✅ Fonction | roza_spi_shared_disable_cs() |

### Fichiers créés :

1. **`main/include/roza_spi_shared.h`** - Header gestionnaire bus SPI partagé
   - Définitions états `roza_spi_state_t` (IDLE, SD, LoRA, ERROR)
   - Structure `roza_spi_config_t` pour configuration broches
   - Structure `roza_spi_bus_t` pour état bus
   - Prototypes fonctions : init, request, release, get_state, disable_cs

2. **`main/include/roza_spi_shared.c`** - Implémentation gestionnaire
   - `roza_spi_shared_init()` : Configuration broches, mutex, CS en HIGH par défaut
   - `roza_spi_shared_request()` : Acquisition exclusive du bus (échec si occupé)
   - `roza_spi_shared_release()` : Libération du bus, remise à IDLE
   - `roza_spi_shared_get_state()` : Retourne état actuel
   - `roza_spi_shared_disable_cs()` : Force les deux CS à HIGH

### Point critique documenté - Débogage Launcher :

Le rapport note explicitly le problème connu depuis Launcher 2.5.0/2.8.0 :

**Problème** : "SD card would not mount due to hardware conflicts with additional I2C devices"
**Cause** : GPIO5 (SPI CS partagé) interference from TCA8418 I2C controller and other sensors
**Solution** : "Set GPIO5 to HIGH during GPIO initialization to ensure SD card compatibility"
C'est exactement ce que fait `roza_spi_shared_init()` : mise par défaut des deux CS en HIGH.

### Adaptations ROZA :

1. **Mutex logiciel** : Remplacement d'un vrai matériel par un mutex portMUTEX_TYPE
   - Suffisant pour le modèle d'utilisation actuel (polling, pas d'IRQ SPI concurrentes)
   - Timeout configurables via mutex_timeout_ms
   - Non-bloquant en mode "déjà occupé" (retourne false immédiatement)

2. **Configuration par défaut** :
   - `sd_cs_pin = 12` (microSD Cardputer ADV)
   - `lora_cs_pin = 5` (SX1262 Cap LoRa-1262)
   - Par défaut les deux mis en HIGH = bus libre, pas de conflit

3. **Arbitration** :
   - Basique : "first-come-first-served" avec mutex
   - Pas de priorité configurée (peut être étendue)
   - Demande simple : type 1=SD, type 2=LoRa

4. **Désactivation CS** :
   - Après toute opération, `disable_cs()` remet les deux CS en HIGH
   - Empêche les communications fantômes entre les transactions
   - Nécessaire pour la sécurité du bus partagé

### Prochaines étapes planifiées :

| Priorité | Composant | Action |
|----------|-----------|--------|
| **G2 (continu)** | Complément I²C communal | G8/G9 : TCA8418(clavier) + BMI270(IMU) + PI4IOE5V6408(expander), détection adresses |
| **G3** | Interface graphique | Utilisation écran + clavier + batterie opérationnels |
| **G4** | GPS AT6668 | Initialisation UART G13/G15, parsing NMEA |
| **G4** | Radio SX1262 | SPI partagé déjà créé, driver SX1262 à intégrer |
| **G5** | Applications | Structure firmware complète |

### Problèmes connus et résolutions :

1. **Conflit GPIO5/SD** - Résolu par initialisation CS en HIGH
   - Vérifié dans Launcher changelog 2.8.0 et documentation CardputerADV.md
   - Laisser GPIO5 HIGH au démarrage est critique avant toute acquisition SPI

2. **Mutex vs Vrai matériel** - Approche logicielle choisie
   - Pas de driver SPI matériel partagé ESP-IDF complexe
   - Suffisant pour le firmware autonome hors-ligne ROZA
   - Peut être remplacé par driver matériel plus tard

3. **Support futur DMA/SPI** - Note pour extensions
   - Le structure inclut config SCK/MOSI/MISO pour futur driver SPI master
   - `spi_host` champ pour intégrer driver ESP-IDF `spi_master` plus tard

### Limitations et notes :

- ⚠️ **Compilation ESP-IDF** : Code utilise `driver/spi_master.h`, `driver/gpio.h` 
  - Functions ESP-IDF natives, compatibles avec ESP32-S3 framework complet.
  - Dans l'environnement actuel (pas d'ESP-IDF installé), code C syntaxiquement valide.
  - Tests matériels requis sur Cardputer ADV réel pour validation du mutex et timing.

- ⚠️ **Vrai matériel SPI partagé** : Cette implémentation est un mutex logiciel
  - Pas de vrai partage hardware SPI (les deux périphériques sur le même bus hardware).
  - Pour un vrai SPI partagé avec arbitrage hardware, faudrait driver ESP-IDF `spi_master` avec
    deux devices et des lignes de contrôle additionnelles.
  - L'approche actuelle convient pour ROZA firmware autonome avec accès séquentiel.

- ✅ **Point critique résolu** : GPIO5 en HIGH par défaut - empêche les reboot loops et
  échecs de montage SD tels que documentés par Launcher projet.

- ✅ **Broches validées** : G5 (SX1262 NSS), G12 (SD CS), G40 (SCK), G14 (MOSI), G39 (MISO)
  - Toutes validées contre documentation M5Stack Cardputer ADV et Launcher board config.

- ✅ **Licence** : CodeESP-IDF Apache 2.0, adapté ROZA avec attribution.

### Résumé de l'infrastructure G2 :

Le gestionnaire `roza_spi_shared_t` fournit l'infrastructure critique permettant :
1. Éviter les conflits entre microSD et SX1262 sur le bus SPI partagé
2. Initialiser GPIO5 en HIGH pour compatibilité SD (apprentissage Launcher)
3. Acquérir/liberer l'accès au bus via API simple
4. Retourner l'état bus pour journalisation/surveillance

C'est une brique fondamentale pour les étapes suivantes :
- **Stockage microSD** (G2) : Lecture/écriture sans corrompre la carte
- **Radio LoRa SX1262** (G4) : Émission/récéption sans conflit SPI
- **Future intégration** : Peut être étendu avec driver SPI hardware véritable

---
*Rapport généré dans le cadre du développement РОЗА. Section G2 infrastructure bus completed.*