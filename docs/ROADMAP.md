# Feuille de route et portes de validation

Cette feuille de route décrit le firmware actif situé dans `firmware/`.
L'architecture de référence est **PlatformIO + Arduino-ESP32**, environnement
`cardputer-adv`. Les anciens fichiers ESP-IDF présents à la racine sont
conservés à titre historique et ne constituent pas la chaîne de compilation active.

**Règle de validation :** la présence d'une initialisation ou d'un pilote dans le
code ne prouve pas son fonctionnement sur la carte. Chaque porte distingue
l'intégration logicielle, la compilation CI sur le commit concerné et les essais
sur le Cardputer ADV réel. Ne jamais marquer un essai matériel comme réussi sans
mesure effectuée sur la carte.

## G0 — Fondation PlatformIO + Arduino-ESP32

- [x] Environnement PlatformIO `cardputer-adv` défini dans `firmware/platformio.ini`
- [x] Cible ESP32-S3 avec Flash 8 Mo et partitionnement dédié
- [x] Versions de plateforme, framework et bibliothèques épinglées
- [x] Workflow GitHub Actions PlatformIO présent : `.github/workflows/roza_esp32s3.yml`
- [ ] Obtenir une compilation CI verte sur le commit courant de `main`
- [ ] Démarrer le firmware sur un Cardputer ADV réel et vérifier le journal série

Commande de compilation de référence :

```sh
cd firmware
pio run -e cardputer-adv
```

Ne pas utiliser `idf.py` ni basculer vers ESP-IDF natif sans décision explicite.

## G1 — BSP réel : écran, clavier et IMU

Les pilotes et points d'intégration existants doivent être réutilisés. Aucun
nouveau pilote ne doit être créé avant d'avoir démontré qu'une fonction manque.

- [ ] **Écran ST7789V2 :** vérifier initialisation, résolution, orientation et rendu
- [ ] **Rétroéclairage :** vérifier commande, niveaux de luminosité et réveil après extinction
- [ ] **Clavier TCA8418 :** vérifier détection I²C, matrice, mappage des touches, Fn et modificateurs
- [ ] **IMU BMI270 :** vérifier détection, initialisation, mesures et stabilité des valeurs
- [ ] Vérifier l'absence de conflits sur le bus I²C interne partagé
- [ ] Réaliser les essais fonctionnels sur Cardputer ADV réel et consigner les résultats

**Critère de sortie G1 :** les quatre fonctions écran/rétroéclairage/clavier/IMU
sont intégrées, compilent sur le commit exact et ont été vérifiées sur matériel.
Une compilation seule ne suffit pas.

La LED RGB existante est un périphérique auxiliaire : sa présence dans le code
n'est pas un critère bloquant de G1. Son comportement et sa consommation seront
contrôlés lors des essais de finition et de gestion d'énergie.

## G2 — Batterie et stockage microSD fiables

### Batterie

- [ ] Vérifier la mesure ADC via le BSP existant et la cohérence des valeurs
- [ ] Vérifier la calibration et les limites de l'estimation du niveau de charge
- [ ] Vérifier les états charge/décharge et les cas de mesure indisponible
- [ ] Documenter les limites des estimations sans capteur de courant ni signal matériel de charge

### microSD

- [ ] Détecter la présence de la carte et lire sa capacité sans écriture
- [ ] Identifier le système de fichiers réellement présent et confirmer sa compatibilité
- [ ] Effectuer le premier montage en lecture seule
- [ ] Après sauvegarde et validation explicite, tester lecture/écriture avec un fichier dédié
- [ ] Tester retrait, coupure et récupération
- [ ] Vérifier l'arbitrage SPI partagé avec le module LoRa

**Interdictions :** ne jamais formater automatiquement la carte, ne jamais effacer
son contenu pour un test et ne pas supposer qu'une carte de 128 Go est en FAT32.
Aucune écriture de test avant identification du système de fichiers et sauvegarde.

## G3 — Interface ROZA et noyau

- [ ] Définir le périmètre et le thème de l'interface ROZA sans supprimer les applications existantes
- [ ] Menu et navigation au clavier
- [ ] Gestionnaire d'événements et cycle de vie des applications
- [ ] Services partagés et persistance locale
- [ ] Carnet, notes et journal hors ligne
- [ ] Vérifier le budget RAM/Flash sur ESP32-S3FN8 sans PSRAM

## G4 — GNSS et radio

- [ ] Vérifier le flux NMEA du GNSS AT6668 et son fonctionnement hors ligne
- [ ] Valider l'antenne et les commandes d'alimentation GNSS
- [ ] Vérifier l'expandeur d'E/S et le commutateur RF du module LoRa
- [ ] Valider le pilote SX1262 et le bus SPI partagé avec la microSD
- [ ] Conserver la réception LoRa comme mode par défaut
- [ ] N'autoriser les essais d'émission qu'après validation matérielle et réglementaire France/UE
- [ ] Mesurer la consommation en veille et en réception ; mesurer l'émission uniquement lors d'essais autorisés

## G5 — Applications hors ligne et finition

- [ ] Vérifier les applications existantes et leur fonctionnement hors ligne
- [ ] Agenda, tâches, calculatrice, archives et outils locaux
- [ ] Cartes hors ligne par tuiles, dans le budget mémoire disponible
- [ ] Audio selon le budget mémoire, énergie et conflits de périphériques
- [ ] Wi-Fi et Bluetooth activés uniquement sur action explicite
- [ ] Vérifier LED RGB, consommation, veille et réveil
- [ ] Tests de robustesse, sauvegarde et récupération
- [ ] Image firmware versionnée et procédure de récupération

## Règles communes de validation

- Une API externe reste facultative : aucune clé ni aucun secret ne doit être inventé.
- Préserver les fonctionnalités, licences et attributions existantes.
- Rechercher les fonctions et pilotes déjà présents avant toute modification afin
  d'éviter les doublons.
- Ne jamais remplacer du code réel par un stub ou une simulation.
- Respecter ESP32-S3FN8, 8 Mo Flash et l'absence de PSRAM.
- Toute modification technique doit être compilée par la CI sur le commit exact.
- Documenter séparément les résultats de compilation, les tests logiciels et
  les essais effectués sur le matériel réel.
