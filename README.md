# РO3A — firmware autonome pour M5Stack Cardputer ADV

**Cible :** M5Stack Cardputer ADV — ESP32-S3FN8, 8 Mo Flash, sans PSRAM  
**Extension :** M5Stack Cap LoRa-1262 — SX1262 + GNSS ATGM336H-6N / AT6668  
**Stockage prévu :** microSD 128 Go (système de fichiers à identifier avant validation)  
**Langue du projet :** français  
**Philosophie :** hors ligne d'abord ; les fonctions radio et réseau doivent être activées volontairement.

## Projet firmware actif

Le firmware actif est dans [`firmware/`](firmware/). Il est fondé sur [Lewis-Peter/cardputer-adv-firmware](https://github.com/Lewis-Peter/cardputer-adv-firmware), importé depuis le commit `3671c86f7850bd0342f39b5befad5b0c02d33f5b`.

Le projet source utilise **PlatformIO + Arduino-ESP32**. Ce dépôt ne doit donc pas être compilé avec `idf.py` directement. Les anciens fichiers et documents historiques conservés à la racine ne sont pas la configuration de compilation active.

## Compilation locale

Installer Python 3 et PlatformIO, puis :

```sh
cd firmware
pio run -e cardputer-adv
```

La configuration cible un ESP32-S3 avec 8 Mo de Flash. La table de partitions dédiée `partitions_8MB.csv` réserve une seule partition applicative (pas de double slot OTA). La taille réelle du binaire et la mémoire disponible doivent être contrôlées à chaque compilation.

## Compilation continue

Le workflow [ROZA Firmware Build](.github/workflows/roza_esp32s3.yml) compile l'environnement PlatformIO `cardputer-adv` sur chaque changement pertinent de `main`, sur les pull requests vers `main` et sur déclenchement manuel. Il publie les binaires uniquement si les trois fichiers attendus sont produits.

## Contraintes matérielles et de sécurité

- Ne pas supposer la présence de PSRAM : la carte cible en est dépourvue.
- Ne jamais formater automatiquement la microSD. Son système de fichiers doit être identifié avant toute opération d'écriture.
- Le Wi-Fi et le Bluetooth doivent rester arrêtés au démarrage et ne démarrer qu'après une action explicite de l'utilisateur.
- LoRa doit rester en réception/écoute par défaut ; aucune émission automatique.
- Installer l'antenne adaptée avant d'alimenter le module Cap LoRa-1262. Ne jamais brancher/débrancher le module à chaud.
- Les GPIO du bus SPI sont partagés entre la microSD et le module LoRa : l'accès doit être arbitré.
- Aucun test matériel n'est revendiqué sans essai réel sur le Cardputer ADV et le Cap.

## Licences et provenance

Voir [`firmware/LICENSE`](firmware/LICENSE) et [`firmware/UPSTREAMS.md`](firmware/UPSTREAMS.md). Le firmware amont est sous MIT, mais des composants tiers conservent leurs propres licences et avis de copyright.
