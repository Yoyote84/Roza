# ROZA CHANINA — M5Stack Cardputer ADV

ROZA vise un **seul firmware**, un seul accueil et un seul runtime pour le Cardputer ADV. NucleoOS reste propriétaire de l'écran, du clavier, du stockage microSD et de l'USB. Les fonctions retenues de Launcher 3.0.0 sont adaptées comme applications ou services natifs ; aucun second firmware Launcher ne doit être démarré.

## État réel du projet

La branche de travail est `migration/roza-chanina-launcher`. Les sources Launcher 3.0.0 et NucleoOS sont épinglées à des révisions précises et compilées dans le projet ESP-IDF combiné.

La CI produit désormais un **candidat logiciel complet de test appareil** : bootloader, table de partitions, données OTA initiales et application réunis dans une image ESP32-S3. Les contrôles vérifient les offsets, les contenus fusionnés, la géométrie 8 Mio et les empreintes SHA-256.

**Ce candidat n'est pas autorisé au flash.** Une compilation verte ne prouve pas le premier démarrage ni le fonctionnement du Cardputer réel. Le manifeste garde `hardware_tested=false` et `safe_to_flash=false`, et le script d'installation refuse ces paquets.

## Architecture et limites actuelles

- Un seul `app_main()` NucleoOS ; pas de second `setup()`/ `loop()`, de réinitialisation M5 ou de montage SD parallèle.
- Le composant plateforme réel de Launcher 3.0.0 est compilé. Des tranches natives ROZA (diagnostic, partitions en lecture seule, contrôle Firmware SD, GNSS et LoRa RX) utilisent le cycle de vie et les périphériques NucleoOS.
- Le portage complet de toutes les interfaces et bibliothèques applicatives de Launcher n'est pas terminé. Le prototype ne doit pas être présenté comme la fusion exhaustive de chaque écran et fonction Launcher.
- Le Store natif NucleoOS est l'unique gestionnaire de paquets. Aucun second installateur Launcher n'est ajouté ; le catalogue signé propre à ROZA reste à configurer.
- Les outils Partitions et Firmware SD restent en lecture seule.
- L'OTA du firmware reste désactivée tant qu'un canal de publication ROZA signé et sa politique de retour arrière n'existent pas.
- Cible : ESP32-S3FN8, flash 8 Mio, sans PSRAM. La carte microSD doit être en FAT32 dans cette construction ; exFAT n'est pas activé. Le pourcentage batterie est une estimation basée sur la tension, pas une mesure par compteur de charge.
- Le SX1262 du Cap LoRa-1262 est limité à la réception dans cette version.

## Documentation

- [Plan et état de la fusion](docs/FUSION-ROZA-NUCLEOOS.md)
- [Statut du candidat et contrôles de flash](docs/FLASH-PREVIEW-NOT-READY.md)
- [Contenu du paquet de test](docs/INSTALL-ROZA-FR.md)
- [Prototype d'intégration ESP-IDF](integration/nucleoos-bridge/README.md)

Aucun flash, effacement ou essai matériel n'est lancé par la CI.
