# Image fusionnée ROZA — candidat logiciel, non flashable

La CI produit une véritable image brute fusionnée ESP32-S3 nommée `ROZA-Cardputer-ADV-FULL.bin`. Elle provient de `idf.py merge-bin` et le validateur compare octet par octet chaque bloc source à son emplacement dans l'image.

## Composition vérifiée

- bootloader ESP32-S3 : `0x0000` ;
- table de partitions binaire : `0x8000` ;
- `otadata` initialisé pour sélectionner `ota_0` : `0xE000` ;
- application NucleoOS avec surcouche ROZA : `0x20000` ;
- flash cible : 8 Mio ; image fusionnée non rembourrée à 8 Mio.

Le manifeste et le paquet contiennent les SHA-256 de l'image, de l'application, du bootloader, de la table de partitions, des données OTA et des fichiers d'installation. Tous les champs `path` de `artifacts`, `flash_image` et `partition_layout` sont relatifs à la racine du paquet ZIP ; `build_path` conserve séparément le chemin du workspace CI d'origine. Après assemblage, `verify-roza-package-manifest.py` vérifie que chaque fichier référencé existe dans l'archive, que sa taille et son SHA-256 correspondent, et que le candidat reste explicitement non flashable.

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

## Avertissement sur les artefacts historiques

Ne pas utiliser le paquet du run [#49 / 42751066014](https://github.com/Yoyote84/Roza/actions/runs/42751066014), construit sur le commit `218ca3f32d6d8d52d334fc66a78eb152bb2b66e9`. Son manifeste indique correctement `safe_to_flash=false` et `hardware_tested=false`, mais le `flash.sh` embarqué dans cet ancien artefact contient encore une condition d'approbation inversée : avec ces deux valeurs à `false`, il ne refuse pas le candidat et peut poursuivre vers la demande du port série puis les commandes esptool. Le paquet historique est donc à considérer comme **dangereux et obsolète**, pas seulement comme non validé.

Le script source actuel `tools/installer/flash.sh` sur la branche de migration exige indépendamment `hardware_tested=true` et `safe_to_flash=true`, avant de vérifier esptool ou de demander un port. Le test `tests/test_flash_installer_guard.py` vérifie ces deux refus sans matériel. Cette correction n'est toutefois pas encore présente dans un nouvel artefact : aucune archive reconstruite depuis le HEAD actuel n'est disponible tant que le workflow reste désactivé.

Le run [#38 / 42731777276](https://github.com/Yoyote84/Roza/actions/runs/42731777276) reste un build `engineering-diagnostic-only` antérieur, et ne constitue pas le paquet de fusion à utiliser.
