# Installer ROZA / РO3А sur M5Stack Cardputer ADV

Cette livraison contient une **image complète de première installation** pour le Cardputer ADV : ESP32-S3FN8, flash 8 Mio, sans PSRAM. Ce n'est pas une mise à jour OTA.

## Fichiers de la livraison

- `ROZA-Cardputer-ADV-FULL.bin` : image ESP-IDF fusionnée, à écrire à l'adresse `0x000000`.
- `ROZA-BUILD-INFO.json` : révisions sources, tailles, offsets, fonctions incluses et SHA-256.
- `flash_args` : paramètres de flash générés par ESP-IDF.
- `flash.sh` : sauvegarde de l'ancienne NVS puis écriture sans effacement global.
- Les licences des composants sources sont jointes.

## Avant le premier démarrage

1. Sauvegardez le contenu de la microSD 128 Go sur un autre support.
2. Vérifiez que la carte est en FAT32. Cette construction ne fournit pas le support exFAT ; une carte 128 Go formatée en exFAT ne sera pas montée correctement.
3. Vissez l'antenne RP-SMA du Cap LoRa-1262 **avant toute mise sous tension**.
4. Chargez suffisamment la batterie et utilisez un câble USB de données.
5. Fermez les applications qui peuvent occuper le port série.

## Migration de la mémoire interne

La table ROZA conserve l'ancienne plage NVS Launcher à l'identique en la réservant sous le label `launcher_nvs_old` : `0x9000`, taille `0x5000`. Le nouveau NucleoOS utilise une nouvelle partition NVS vierge à `0x13000`, taille `0x6000`. L'ancienne NVS est donc préservée physiquement, mais ses réglages ne sont pas importés automatiquement dans ROZA.

La zone `otadata` reste à `0xE000/0x2000`, mais elle est réinitialisée pour démarrer le slot `ota_0`. L'ancienne application Launcher et son coredump sont remplacés par les deux slots OTA et les partitions de données de ROZA. Comme le binaire fusionné remplit les espaces vides avec `0xFF`, le script restaure l'ancienne NVS à son adresse d'origine après l'écriture complète, puis relit les 20 Kio et compare la sauvegarde octet par octet.

Le script sauvegarde d'abord l'ancienne NVS sur le PC, puis efface seulement :
- `0x11000-0x11FFF` : nouvelle zone PHY ;
- `0x13000-0x18FFF` : nouvelle NVS ;
- `0x19000-0x19FFF` : clés NVS.

Il n'exécute jamais `erase_flash` et ne formate pas la microSD.

## Installation avec le script Linux/macOS

Python 3 et esptool sont nécessaires :

```sh
python3 -m pip install esptool
PORT=/dev/ttyACM0 ./flash.sh ROZA-Cardputer-ADV-FULL.bin
```

Remplacez le port série par celui du Cardputer. Le script vérifie le SHA-256 avec le manifeste avant toute commande de flash, sauvegarde l'ancienne NVS, efface les zones nécessaires, écrit l'image fusionnée à `0x0`, restaure l'ancienne NVS à `0x9000`, puis vérifie la restauration par relecture. Passez le Cardputer en mode téléchargement en maintenant BOOT/G0 pendant la connexion USB ou le reset si esptool ne le détecte pas.

## Installation avec un outil graphique

Choisissez :
- cible : ESP32-S3 ;
- fichier : `ROZA-Cardputer-ADV-FULL.bin` ;
- offset : `0x000000` ;
- flash : 8 MB ;
- mode : DIO.

Avant l'écriture, faites une sauvegarde de la zone `0x9000` sur `0x5000` octets, puis effacez uniquement les plages listées dans la section Migration. N'utilisez pas `Erase All Flash`.

## Ce que contient le firmware

Un seul runtime NucleoOS personnalisé ROZA : interface française et fuseau Europe/Paris par défaut, réglage manuel de la date et de l'heure, fichiers/microSD, Store natif signé, Wi-Fi manuel, WebUI, USB MSC, applications/jeux NucleoOS, diagnostic, partitions en lecture seule, pré-vérification du firmware SD, GNSS du Cap LoRa-1262 et réception LoRa SX1262.

Les fonctions qui se recouvrent entre Launcher et NucleoOS sont fournies par les services natifs NucleoOS. Il n'y a pas de deuxième menu, de second `setup()`/`loop()`, de deuxième montage SD ou de deuxième installateur de paquets Launcher.

Le SX1262 est en réception seule dans cette livraison. L'OTA firmware reste désactivée tant qu'un canal de mise à jour ROZA signé n'est pas publié. Le Store natif conserve la vérification de signature, mais son catalogue n'est pas encore propre à ROZA.

## Premier essai matériel

Le build logiciel et la structure du binaire seront contrôlés par la CI. Le fonctionnement réel doit encore être vérifié sur le Cardputer : démarrage à froid, écran, clavier, microSD, batterie, audio, Wi-Fi, GNSS et radio. Le manifeste indique explicitement que ces essais matériels n'ont pas encore été réalisés.

Après installation, notez les éventuels messages de démarrage série et vérifiez chaque périphérique avant de considérer ROZA comme stable.
