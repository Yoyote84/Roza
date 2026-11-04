# Instructions OpenCode — firmware РОЗА

## Dossier et branche

- Projet actif : `/public/Roza/firmware`.
- Dépôt : `Yoyote84/Roza`.
- Branche unique autorisée : `main`.
- Ne jamais développer le firmware actif dans les anciens fichiers racine `main/` ou `CMakeLists.txt`.

## Architecture imposée

- ESP-IDF natif, C++23.
- Dépendance matérielle : `espp/m5stack-cardputer` exactement en version `1.3.11`.
- Consulter `UPSTREAMS.md` avant toute intégration.
- Réutiliser les API BSP existantes pour écran, clavier, IMU, batterie, microSD, SX1262 et GNSS.
- Ne pas réécrire un pilote déjà fourni par le BSP sans raison technique documentée.
- Ne jamais introduire de stub, simulation présentée comme réelle ou pilote vide.

## Matériel et sécurité

- Cardputer ADV : ESP32-S3FN8, 8 Mo Flash, aucune PSRAM.
- Écran ST7789V2 240×135 ; clavier TCA8418 ; IMU BMI270.
- Cap : SX1262 + GNSS ATGM336H-6N@AT6668.
- La microSD et le Cap partagent le bus SPI d'extension : conserver l'arbitrage du BSP.
- Ne jamais monter/formater automatiquement la microSD 128 Go avant identification de son format.
- Wi-Fi et Bluetooth inactifs par défaut.
- Aucune émission LoRa au démarrage. Vérifier la région radio, la configuration, l'antenne et la sécurité avant toute émission.
- Ne pas retirer/insérer le Cap sous tension.

## Procédure après chaque tâche

1. Lire le code et les exemples du BSP avant d'implémenter.
2. Modifier uniquement ce qui est nécessaire.
3. Compiler depuis `firmware/` avec ESP-IDF 5.5.1.
4. Conserver `dependencies.lock` après génération.
5. Vérifier les journaux GitHub Actions.
6. Commit et push sur `origin/main` après validation locale.
7. Fournir SHA, lien, fichiers, tests, résultat CI et tests matériels restant à faire.
8. Ne jamais déclarer un test matériel sans l'avoir réalisé.

## Étapes

1. G0 : fondation propre et build vert.
2. G1 : valider écran, clavier, IMU sur matériel.
3. G2 : batterie et microSD avec mesure et détection du système de fichiers.
4. G3 : interface et navigation.
5. G4 : GNSS et réception SX1262, puis transmission après validation.
6. G5 : applications hors ligne.
