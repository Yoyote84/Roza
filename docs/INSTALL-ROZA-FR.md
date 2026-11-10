# ROZA / РO3А — paquet candidat Cardputer ADV (flash non autorisé)

> **Ne pas flasher ce paquet.** Ce document décrit le contenu et les limites du candidat logiciel ; ce n'est pas une procédure d'installation pour la version actuelle.

Le manifeste `ROZA-BUILD-INFO.json` fait foi. Pour le candidat produit par la CI, il indique explicitement :

- `release_ready: false`
- `safe_to_flash: false`
- `hardware_tested: false`

Le script `flash.sh` vérifie ces champs et s'arrête avant de rechercher esptool, de demander un port série ou d'envoyer une commande au Cardputer. Il n'est pas possible de contourner ce contrôle avec une simple confirmation au clavier.

## Contenu du paquet

- `ROZA-Cardputer-ADV-FULL.bin` : image ESP-IDF fusionnée contenant bootloader, table de partitions, données OTA initiales et application.
- `ROZA-BUILD-INFO.json` : révisions des sources, géométrie des partitions, tailles, empreintes et limites connues.
- `flash_args` : arguments générés par ESP-IDF.
- `flash.sh` : installateur initial protégé par le manifeste.
- `audit/` : image fusionnée de contrôle, ELF, application, bootloader, table binaire, données OTA, CSV des partitions et verrou de dépendances.
- Les licences des composants intégrés.

## Cible et mémoire flash

- M5Stack Cardputer ADV, ESP32-S3FN8.
- Flash : 8 Mio ; PSRAM : absente.
- Image fusionnée écrite à l'offset logique `0x0` lorsqu'une version sera autorisée.
- La table unique réserve l'ancienne NVS Launcher à `0x9000/0x5000`, conserve `otadata` à `0xE000/0x2000`, et place les deux slots applicatifs à `0x20000/0x380000` et `0x3A0000/0x380000`.
- Le slot applicatif fait 3 670 016 octets. Le binaire de cette construction utilise 3 256 224 octets, soit 413 792 octets (11,27 %) de marge flash dans le slot.
- La marge flash est calculée ; la marge de heap en fonctionnement n'est pas encore mesurée sur le matériel. L'absence de PSRAM rend cette validation indispensable.

L'installateur est conçu pour sauvegarder puis restaurer l'ancienne plage NVS et vérifier sa restauration octet par octet. Il n'utilise pas d'effacement global et ne formate pas la microSD. Cela ne remplace pas un essai de récupération réel.

## Fonctions présentes dans ce candidat

- Runtime NucleoOS unique avec identité et interface ROZA françaises.
- Services natifs NucleoOS pour l'écran, le clavier, la SD, l'USB, les fichiers et le Store.
- Store natif unique avec vérification des signatures et installation atomique ; le catalogue signé propre à ROZA n'est pas encore configuré.
- Applications ROZA de diagnostic, de consultation des partitions en lecture seule et de pré-vérification des fichiers Firmware SD.
- Réglage manuel de la date et de l'heure, fuseau Europe/Paris, Wi-Fi manuel et WebUI via les services NucleoOS.
- GNSS NMEA du Cap LoRa-1262 et réception LoRa SX1262.

Le portage complet de l'interface et de toutes les applications/bibliothèques Launcher 3.0.0 n'est pas achevé. La présence du composant plateforme Launcher dans le graphe ESP-IDF ne signifie pas que tous les écrans Launcher ont été portés.

## Blocages avant toute autorisation

1. Premier démarrage, écran, clavier, batterie, microSD, USB, Wi-Fi, audio, GNSS et SX1262 non testés sur un Cardputer ADV réel.
2. Mesure de la RAM libre et du comportement sans PSRAM à réaliser en fonctionnement.
3. Catalogue de paquets signé propre à ROZA non configuré.
4. Canal de firmware ROZA signé et politique de retour arrière non publiés ; l'OTA reste désactivée.
5. Portage fonctionnel complet des parties Launcher retenues encore à terminer.

Aucun flash ni essai matériel n'a été exécuté pendant la compilation. La CI ne peut pas remplacer les contrôles de premier démarrage et de périphériques sur le matériel ; le paquet reste donc bloqué.
