# Aperçu du paquet de flash ROZA — NON AUTORISÉ POUR LE CARDPUTER

La CI produit une image brute fusionnée ESP32-S3 nommée
`ROZA-ESP32S3-FLASH-PREVIEW.bin`. Elle est générée par `idf.py merge-bin`
à partir des fichiers du build, puis vérifiée octet par octet contre
`flash_args`.

## Ce que contient l'image

- bootloader ESP32-S3 à l'adresse `0x0000` ;
- table de partitions à `0x8000` ;
- données OTA initiales à `0xF000` ;
- application NucleoOS avec la surcouche ROZA à `0x20000`.

Ce fichier unique est donc un véritable conteneur binaire fusionné que
l'outil Espressif peut écrire à l'adresse de base `0x0`. Il n'est pas
rembourré jusqu'à 8 Mio ; les espaces entre les blocs sont néanmoins
encodés dans l'image fusionnée et peuvent être réécrits lors du flash.

## Pourquoi il ne faut pas encore l'installer

**NE PAS FLASHER CET APERÇU SUR LE CARDPUTER ADV.**

1. La table de partitions NucleoOS n'est pas identique à celle du profil
   M5Launcher Cardputer ADV. Le changement affecte notamment les métadonnées
   OTA, la zone d'application et la zone coredump. La préservation complète
   des anciennes données NVS n'a pas encore été démontrée.
2. Le portage de l'interface et des fonctions Launcher 3.0.0 est incomplet :
   le build lie le composant réel `launcher_platform`, mais cela ne signifie
   pas que l'application Launcher entière est intégrée.
3. Le démarrage à froid, l'écran, le clavier, la microSD, le retour arrière
   et la procédure de récupération n'ont pas été testés sur le matériel.
4. Le canal de firmware ROZA signé n'est pas configuré. La mise à jour OTA
   reste volontairement désactivée.

Le paquet ne réalise aucun flash, n'efface pas la carte et ne programme aucun
eFuse. Sa présence dans GitHub Actions prouve uniquement que les fichiers
nécessaires ont été assemblés et que leurs offsets/contenus correspondent
au manifeste de flash ESP-IDF. **Le champ `safe_to_flash` reste `false`.**

N'utiliser ce binaire qu'après publication d'une version explicitement
approuvée, accompagnée d'une procédure de sauvegarde/migration et de
récupération validée pour la table de partitions finale.
