# Feuille de route et portes de validation

## G0 — Fondation (livraison actuelle)
- [x] Projet ESP-IDF minimal
- [x] Cible ESP32-S3
- [x] Partitionnement initial
- [x] Cartographie des broches
- [ ] Compiler avec l'environnement ESP-IDF retenu
- [ ] Démarrer sur Cardputer ADV et lire le journal série

## G1 — BSP réel
- [ ] Initialiser LCD ST7789V2 et rétroéclairage
- [ ] Lire le clavier TCA8418
- [ ] Lire batterie ADC
- [ ] Lire BMI270
- [ ] Tester LED RGB sans la laisser allumée inutilement

## G2 — Stockage fiable
- [ ] Détecter carte et capacité
- [ ] Identifier FAT32/exFAT
- [ ] Monter en lecture seule pour le premier test
- [ ] Écrire/lire un fichier de test après sauvegarde
- [ ] Tester retrait/coupure et récupération

## G3 — Interface et noyau
- [ ] Thème historique
- [ ] Menu pilotable au clavier
- [ ] Gestionnaire d'événements
- [ ] Services d'applications
- [ ] Carnet persistant

## G4 — GNSS et radio
- [ ] Parser NMEA du AT6668
- [ ] Valider antenne GNSS
- [ ] Initialiser expandeur et commutateur RF
- [ ] Pilote SX1262
- [ ] Essais radio autorisés en France, fréquence et paramètres conformes
- [ ] Mesures de consommation en veille/réception/émission

## G5 — Applications et finition
- [ ] Agenda, tâches, calculatrice, archives
- [ ] Cartes hors ligne par tuiles
- [ ] Audio selon budget mémoire/énergie
- [ ] Wi-Fi manuel
- [ ] Tests de robustesse et sauvegarde
- [ ] Image firmware versionnée et procédure de récupération
