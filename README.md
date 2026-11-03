# РОЗА — firmware autonome pour M5Stack Cardputer ADV

**Version :** 0.1.0-foundation  
**Langue de l'interface :** français  
**Cible :** M5Stack Cardputer ADV (ESP32-S3FN8, 8 Mo Flash)  
**Extension :** M5Stack Cap LoRa-1262 (SX1262 + AT6668)  
**Stockage utilisateur :** microSD 128 Go

## Intention

РОЗА est un firmware hors connexion d'abord, à esthétique historique sobre (années 1940–1950). Le Wi-Fi est désactivé par défaut et ne sera activé que sur demande explicite.

## État de cette livraison

Cette archive est la **fondation du projet**, pas encore un firmware fonctionnel complet. Elle contient un projet ESP-IDF minimal, une configuration cible, les premières définitions matérielles et la documentation d'architecture. Le code de démarrage ne pilote volontairement pas encore l'écran, le clavier, la SD, le GPS ou la radio : chaque pilote sera ajouté et testé sur le matériel avant d'être déclaré opérationnel.

## Matériel confirmé

- ESP32-S3FN8, 240 MHz, Flash interne 8 Mo.
- Écran ST7789V2, 240 × 135.
- Clavier 56 touches via TCA8418 sur Cardputer ADV.
- BMI270 sur I²C.
- microSD via SPI.
- Cap LoRa-1262 : SX1262 en SPI et récepteur GNSS AT6668 en UART.

## Construire

Prérequis : ESP-IDF installé et configuré (version stable 5.x compatible ESP32-S3).

```sh
. "$IDF_PATH/export.sh"
idf.py set-target esp32s3
idf.py build
```

Pour flasher, connecter le Cardputer en USB. Le mode téléchargement M5Stack : mettre l'interrupteur sur OFF, maintenir G0, remettre sous tension, puis relâcher G0. Vérifier la procédure dans la documentation de la révision matérielle utilisée.

**Ne flasher qu'après une compilation réussie.** Cette fondation ne contient pas encore de pilote écran ; son démarrage se vérifie par le journal série.

## Règles de sécurité

- Ne jamais alimenter le module LoRa sans son antenne correctement vissée.
- Ne pas retirer/insérer le Cap sous tension.
- Ne pas formater la microSD avant sauvegarde et identification de son système de fichiers.
- La microSD et le SX1262 partagent SPI : accès arbitré, CS inactifs par défaut.
- Le Wi-Fi/Bluetooth restent désactivés dans la configuration initiale.
