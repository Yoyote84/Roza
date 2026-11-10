# Image fusionnée ROZA — candidat logiciel, non flashable

La CI produit une véritable image brute fusionnée ESP32-S3 nommée `ROZA-Cardputer-ADV-FULL.bin`. Elle provient de `idf.py merge-bin` et le validateur compare octet par octet chaque bloc source à son emplacement dans l'image.

## Composition vérifiée

- bootloader ESP32-S3 : `0x0000` ;
- table de partitions binaire : `0x8000` ;
- `otadata` initialisé pour sélectionner `ota_0` : `0xE000` ;
- application NucleoOS avec surcouche ROZA : `0x20000` ;
- flash cible : 8 Mio ; image fusionnée non rembourrée à 8 Mio.

Le manifeste et le paquet contiennent les SHA-256 de l'image, de l'application, du bootloader, de la table de partitions, des données OTA et des fichiers d'installation.

## Table de partitions unique

La table validée pour ESP32-S3FN8, flash 8 Mio, sans PSRAM, contient neuf entrées non chevauchantes et couvre exactement `0x000000–0x7FFFFF` :

| Partition | Offset | Taille |
|---|---:|---:|
| Ancienne NVS Launcher réservée | `0x9000` | `0x5000` |
| otadata | `0xE000` | `0x2000` |
| phy_init | `0x11000` | `0x1000` |
| NVS NucleoOS | `0x13000` | `0x6000` |
| nvs_keys | `0x19000` | `0x1000` |
| ota_0 | `0x20000` | `0x380000` |
| ota_1 | `0x3A0000` | `0x380000` |
| nucleo_nap | `0x720000` | `0x40000` |
| cfg | `0x760000` | `0xA0000` |

Le validateur contrôle les libellés, types, sous-types, offsets, tailles, alignements, chevauchements, limites 8 Mio, taille du slot et présence exacte des blocs de flash. L'ancienne NVS est réservée séparément ; le script d'installation prévoit sa sauvegarde et sa restauration après l'écriture de l'image fusionnée.

## Ce que le succès CI prouve

- Le graphe ESP-IDF combiné compile et se lie avec le `app_main()` NucleoOS unique.
- Les tests hôte du clavier et du parseur GNSS passent.
- Les offsets et contenus du binaire fusionné correspondent aux fichiers produits par ESP-IDF.
- Le manifeste et le paquet logiciel de test sont assemblés.

## Ce qu'il ne prouve pas

**Le candidat n'est pas autorisé au flash.** Le manifeste conserve `safe_to_flash=false` et `hardware_tested=false`.

- Le démarrage à froid et la récupération après démarrage raté n'ont pas été essayés sur le Cardputer.
- La marge de flash du slot est connue (413 792 octets, 11,27 % pour cette construction), mais la marge de heap réelle et les pics RAM ne sont pas mesurés sur un appareil sans PSRAM.
- L'écran, le clavier, la SD, l'USB, la batterie, le Wi-Fi, l'audio, le GNSS et le SX1262 n'ont pas été validés physiquement.
- Le portage complet de toutes les interfaces et applications Launcher 3.0.0 n'est pas terminé.
- Le canal de firmware ROZA signé et le catalogue de paquets ROZA signé restent à configurer. L'OTA firmware reste désactivée.

Le paquet n'efface pas la flash pendant la CI et aucun matériel n'est connecté ni testé. Le script d'installation refuse tout manifeste qui ne déclare pas explicitement `hardware_tested=true` et `safe_to_flash=true`. Ne pas contourner cette barrière.
