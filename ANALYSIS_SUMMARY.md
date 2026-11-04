# РОЗА — Analyse Complète du Dépôt

## 1. État réel du dépôt

Le dépôt `/public/Roza` se trouve à l'étape **G0 — Fondation** (livraison actuelle). Il ne contient qu'une structure de projet ESP-IDF minimale sans pilotes matériels ni interface utilisateur.

**Fichiers existants (12 fichiers) :**
- `README.md` — Description du projet en français, matériel confirmé, règles de construction
- `CMakeLists.txt` (racine) — Configuration de projet de base
- `sdkconfig.defaults` — Configuration ESP32-S3, Flash 8 Mo, partitions personnalisées, Wi-Fi/Bluetooth désactivés
- `partitions.csv` — Table de partitionnement OTA avec 2 images + NVS
- `main/CMakeLists.txt` — Enregistrement de composant minimal
- `main/roza_main.c` — Point d'entrée `app_main` : journalisation + boucle FreeRTOS infinie sans pilote aucun
- `main/include/roza_board_pins.h` — Définitions de broches pour tout le matériel
- `main/include/roza_version.h` — Nom, version, langue, étape de construction
- `docs/ARCHITECTURE.md` — Architecture logicielle, modules, règles de concurrence, mémoire, partitionnement
- `docs/HARDWARE.md` — Cartographie des broches Cardputer ADV + Cap LoRa-1262, conflits bus partagés, avertissements
- `docs/ROADMAP.md` — Feuille de route G0 à G5 avec cases à cocher
- `docs/SPECIFICATION.md` — Spécifications produit, principes non négociables, identité visuelle, applications prévues

**État de compilation :** ESP-IDF n'est pas installé dans cet environnement, mais le code est syntaxiquement valide C pour ESP-IDF. Le `app_main` ne démarre aucun périphérique — c'est intentional pour la phase G0.

---

## 2. Composants déjà présents et ceux qui manquent

### ✅ Déjà présents (documentation & structure) :

| Composant | Statut |
|-----------|--------|
| Projet ESP-IDF minimal | ✅ |
| Cible ESP32-S3 | ✅ |
| Partitionnement OTA (2 slots) | ✅ |
| NVS | ✅ |
| Cartographie complète des broches | ✅ |
| Langue français (fr-FR) | ✅ |
| Wi-Fi/Bluetooth désactivés par défaut | ✅ |
| Règles de consommation énergétique | ✅ |
| Identité visuelle (palette, typographie, années 1940-1950) | ✅ |
| Spécifications non négociables | ✅ |
| Feuille de route G0-G5 | ✅ |

### ❌ Manquants (pilotes & implémentation) :

| Sous-système | Documentation | Statut |
|--------------|---------------|--------|
| Écran ST7789V2 (240×135) | HARDWARE.md §1 | ❌ Driver initialisation + rétroéclairage |
| Clavier TCA8418 (56 touches) | HARDWARE.md §1 | ❌ Driver lecture |
| ADC batterie | HARDWARE.md §1 | ❌ Lecture tension + caractérisation autonomie |
| BMI270 (IMU I²C) | HARDWARE.md §1 | ❌ Driver lecture accéléromètre/gyroscope |
| microSD (SPI) | HARDWARE.md §1 | ❌ Détection, FAT32/exFAT, montage lecture/écriture |
| GNSS AT6668 (UART) | HARDWARE.md §1 | ❌ Parser NMEA, validation antenne |
| SX1262 LoRa (SPI) | HARDWARE.md §1 | ❌ Driver initialisation, émission réception |
| Expandeur PI4IOE5V6408 | HARDWARE.md §1 | ❌ Gestion interrupteur antenne RF, LED |
| Clavier TCA8418 partagé I²C | ARCHITECTURE.md | ❌ Gestionnaire I²C communal |
| SPI partagé SD/SX1262 | ARCHITECTURE.md | ❌ Mutex/service commun |
| Kernel FreeRTOS tasks | ARCHITECTURE.md | ❌ Cycle de vie, événements, supervision |
| Services API stables | ARCHITECTURE.md | ❌ API pour applications |
| UI thème historique | UI | ❌ Écran d'accueil, widgets |
| Stockage NVS/microSD | storage | ❌ Configuration, données |
| Applications de base | apps | ❌ Carnet, agenda, calculatrice, fichiers |
| Gestionnaire d'événements | ❌ |

---

## 3. Problèmes techniques détectés

### 3.1 Conflits de bus partagés (documentés mais non résolus)

| Bus | Conflit | Solution architecturale |
|-----|---------|------------------------|
| **SPI** | microSD (CS=G12) et SX1262 (CS=G5) partagent SCK=G40, MOSI=G14, MISO=G39 | ⚠️ Gestionnaire SPI commun avec mutex + CS inactif entre transitions |
| **I²C** | TCA8418 (clavier), BMI270 (IMU), expander Cap (PI4IOE5V6408) partagent G8/G9 | ⚠️ Gestionnaire I²C communal avec détection d'adresses matériel |

### 3.2 Conflits de broches potentielles

| Broche | Usage | Risque |
|--------|-------|--------|
| G38 | LCD rétroéclairage / LED RGB | Déjà utilisé en sortie |
| G3-G6 | SX1262 (RESET, IRQ, BUSY) | ⚠️ Vérifier non conflit avec autres usages |
| G8-G9 | I²C partagé (clavier + BMI270 + expander) | ⚠️ Vérifier adresses I²C sur matériel |
| G10 | ADC batterie | ⚠️ Peut entrer en conflit si autre périphérique sur même broche |
| G14/MOSI | SPI partagé SD + SX1262 | ⚠️ Déjà documenté, gestion mutex nécessaire |
| G39/MISO | SPI partagé SD + SX1262 | ⚠️ Déjà documenté, gestion mutex nécessaire |
| G40/SCK | SPI partagé SD + SX1262 | ⚠️ Déjà documenté, gestion mutex nécessaire |
| G13/G15 | GNSS UART | ⚠️ Vérifier compatibilité avec autres UART |
| G44 | IR TX | ⚠️ Vérifier non utilisé ailleurs |

### 3.3 Contraintes matérielles

- **Flash 8 Mo** : Ressources limitées, pas de charges lourdes
- **microSD 128 Go** : Souvent exFAT — pilote non présumé, tester d'abord
- **Consommation LoRa** : Forte en émission, mesurer courant réel
- **Antenne RF** : RP-SMA obligatoire avant émission
- **Pas de connexion sous tension** : Ne pas brancher/débrancher le Cap sous tension

### 3.4 Logiciel — Problèmes détectés

1. **`roza_main.c`** : Boucle infinie sans aucun périphérique initialisés — OK pour G0 mais nécessite transition
2. **Aucun pilote matériel** : Aucun driver d'écran, clavier, SD, GPS, radio n'est présent
3. **Aucune gestion d'erreurs** : Pas de gestion d'échec d'initialisation des périphériques
4. **Aucune tâche FreeRTOS** : Juste la tâche principale en boucle

### 3.5 Problèmes de conformité

- ✅ Langue française interface
- ✅ Wi-Fi désactivé par défaut
- ✅ Identité visuelle années 1940-1950 spécifiée
- ⚠️ Vérifier les broches contre documentation M5Stack officielle (en cours — cartographie présente mais non validée matériellement)
- ✅ Hors prioritaire
- ✅ Aucune fonction simulée présentée comme fonctionnelle

---

## 4. Plan de développement priorisé

### Phase 0 : Vérification d'environnement (déjà faite)
- ✅ Structure de projet valide
- ✅ Configuration SDK correcte
- ❌ ESP-IDF non disponible dans cet environnement (à installer séparément)

### Phase G0 → G1 : BSP réel (Priorité haute)

**G1.1 — Initialiser LCD ST7789V2**
- Driver affichage bas niveau (SPI)
- Contrôle rétroéclairage (G38)
- Mode veille écran
- **Délai estimé** : 2-3 jours

**G1.2 — Lire clavier TCA8418**
- Initialisation I²C communal
- Détection adresses appareils
- Lecture état touches 56 touches
- **Délai estimé** : 2-3 jours

**G1.3 — Lire ADC batterie**
- Configuration entrée ADC1
- Mesure tension batterie
- Début caractérisation autonomie
- **Délai estimé** : 1-2 jours

**G1.4 — Lire BMI270 (IMU)**
- Initialisation I²C
- Mode accéléromètre/gyroscope
- Calibration basique
- **Délai estimé** : 2 jours

**G1.5 — LED RGB sans allumage inutile**
- Contrôle RGB via expandeur PI4IOE5V6408
- Mode économie d'énergie
- **Délai estimé** : 1 jour

### Phase G2 : Stockage fiable

**G2.1 — Détecter carte et capacité**
- Initialisation SPI partagée
- Détection présence carte SD
- Identification système fichiers (FAT32/exFAT)
- **Délai estimé** : 2-3 jours

**G2.2 — Monter en lecture seule**
- Montage système de fichiers
- Droit d'accès lecture seule pour tests
- **Délai estimé** : 1 jour

**G2.3 — Écrire/lire fichier de test**
- Opérations CRUD basiques
- Gestion retrait/coupure
- **Délai estimé** : 1-2 jours

### Phase G3 : Interface et noyau

**G3.1 — Thème historique**
- Palette vert olive, ivoire, noir d'encre, bordeaux
- Typographie bitmap compacte avec accents français
- Contraste élevé 240×135
- **Délai estimé** : 3-4 jours

**G3.2 — Menu pilotable au clavier**
- Navigation écran/clavier
- Sélection/menu
- **Délai estimé** : 2-3 jours

**G3.3 — Gestionnaire d'événements**
- infrastructure événements FreeRTOS
- Dispatch vers services
- **Délai estimé** : 2-3 jours

**G3.4 — Services applications**
- API stables indépendants des pilotes
- **Délai estimé** : 2-3 jours

**G3.5 — Carnet persistant**
- Stockage notes dans microSD
- **Délai estimé** : 2-3 jours

### Phase G4 : GNSS et radio

**G4.1 — Parser NMEA du AT6668**
- Initialisation UART G13/G15
- Parsing trames NMEA
- Validation fix GPS
- **Délai estimé** : 2-3 jours

**G4.2 — Valider antenne GNSS**
- Vérification réception satellite
- **Délai estimé** : 1 jour

**G4.3 — Initialiser expandeur commutateur RF**
- PI4IOE5V6408 P0 antenne LoRa
- **Délai estimé** : 1 jour

**G4.4 — Pilote SX1262**
- Initialisation SPI partagée
- Mode réception/émission
- Fréquences conformes France
- **Délai estimé** : 3-4 jours

**G4.5 — Mesures consommation**
- Veille/réception/émission
- **Délai estimé** : 2-3 jours

### Phase G5 : Applications et finition

**G5.1 — Applications de base**
- Accueil/lanceur
- Carnet/journal
- Agenda/tâches
- Calculatrice
- Gestionnaire fichiers/lecteur texte
- Paramètres/état batterie
- **Délai estimé** : 5-7 jours

**G5.2 — Cartes hors ligne par tuiles**
- **Délai estimé** : 3-4 jours

**G5.3 — Audio (budget mémoire/énergie)**
- **Délai estimé** : 2-3 jours

**G5.4 — Wi-Fi manuel**
- Activation explicite utilisateur
- Désactivation automatique
- **Délai estimé** : 2-3 jours

**G5.5 — Tests robustesse & sauvegarde**
- **Délai estimé** : 3-4 jours

**G5.6 — Image firmware versionnée & procédure récupération**
- **Délai estimé** : 2-3 jours

---

### Critères d'acceptation "Firmware compilable et fonctionnel"

1. **Compilation réussie** avec `idf.py build` ciblant esp32s3
2. **Démarrage sur matériel** : Journal série affiche message de bienvenue français
3. **Écran allumé** : ST7789V2 affiche l'écran d'accueil thème années 1940-1950
4. **Clavier fonctionnel** : Toutes les touches lisent des entrées
5. **Batterie lue** : Tension affichée sur écran
6. **SD card détectée** : Capacité identifiée (FAT32 exFAT)
7. **GNSS fixé** : Trames NMEA valides parsées
8. **Radio LoRa** : Pilotage SX1262 en réception (pas d'émission sans action explicite)
9. **Consommation maîtrisée** : Veille mode opérationnel
10. **Aucune fonction simulée** : Tous les pilotes testés sur matériel réel

---

### Ordre de priorité absolue (MVP minimum)

1. **LCD ST7789V2 + rétroéclairage** — Sans écran, rien n'est visible
2. **Clavier TCA8418** — Pour l'interface utilisateur
3. **ADC batterie** — Pour l'indicateur d'état
4. **SPI partagé avec mutex** — Infrastructure pour SD + LoRa
5. **Parser GNSS AT6668** — Pour positionnement
6. **SX1262 LoRa réception** — Pour communication radio

Tous les autres éléments peuvent suivre dans les itérations ultérieures.