# РОЗА — firmware autonome pour M5Stack Cardputer ADV

**Cible :** M5Stack Cardputer ADV (ESP32-S3FN8, 8 Mo Flash, sans PSRAM)  
**Extension :** M5Stack Cap LoRa-1262 (SX1262 + GNSS ATGM336H-6N / AT6668)  
**Stockage utilisateur :** microSD 128 Go  
**Langue :** français  
**Philosophie :** hors ligne d'abord, Wi-Fi désactivé tant qu'une application ne le demande pas.

## Projet actif

Le projet firmware propre et actif se trouve désormais dans **[`firmware/`](firmware/)**. Il utilise ESP-IDF natif et le BSP ESPP officiel pour Cardputer / Cardputer ADV, afin de réutiliser les pilotes maintenus plutôt que de conserver les pilotes artisanaux défectueux de la première fondation.

Les anciens fichiers C présents à la racine et dans `main/` sont conservés pour audit et historique ; ils ne font plus partie du projet actif et ne doivent plus être étendus.

## Compilation

Depuis le dépôt :

```sh
cd firmware
. "$IDF_PATH/export.sh"
idf.py set-target esp32s3
idf.py build
```

La CI GitHub Actions compile désormais ce dossier avec ESP-IDF 5.5.1. Le BSP `espp/m5stack-cardputer` est épinglé en version 1.3.11.

## Matériel et sécurité

- Le BSP prend en charge l'écran ST7789V2, le clavier ADV TCA8418, l'IMU BMI270, la batterie, la microSD et le Cap SX1262 + GNSS.
- Le Wi-Fi et le Bluetooth ne sont pas démarrés au boot.
- La microSD n'est pas montée automatiquement dans cette fondation : son système de fichiers doit être identifié avant toute opération, sans formatage.
- La radio LoRa n'est pas initialisée ni utilisée pour émettre au démarrage. L'antenne doit être vissée avant toute alimentation du Cap.
- Aucun test matériel n'est revendiqué sans essai réel sur le Cardputer ADV.

Voir [firmware/README.md](firmware/README.md), [firmware/UPSTREAMS.md](firmware/UPSTREAMS.md) et [AGENTS.md](AGENTS.md).
