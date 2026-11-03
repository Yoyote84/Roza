# Architecture logicielle

```text
Application / Lanceur
        |
API de services (événements, stockage, périphériques)
        |
+------------------ Noyau РОЗА ------------------+
| tâches FreeRTOS | gestion énergie | supervision |
| événements      | configuration   | journal     |
+-------------------------------------------------+
        |
+--------------------- Services ------------------+
| affichage | clavier | audio | fichiers | GNSS   |
| radio     | Wi-Fi   | IMU    | batterie | temps  |
+-------------------------------------------------+
        |
Abstraction matérielle (BSP Cardputer ADV + Cap)
        |
ESP32-S3 / ST7789V2 / TCA8418 / BMI270 / SD / SX1262 / AT6668
```

## Modules

- `kernel`: cycle de vie, événements, supervision.
- `bsp`: broches et initialisation de la carte.
- `drivers`: pilotes matériels isolés.
- `services`: API stables pour les applications.
- `apps`: applications autonomes.
- `ui`: écran d'accueil, widgets, thème historique.
- `storage`: configuration NVS et données microSD.

## Règles de concurrence

- Un seul propriétaire logique du bus SPI partagé.
- Le pilote SD et le pilote SX1262 acquièrent le bus SPI via un mutex/service commun.
- Tous les CS sont inactifs avant de changer de périphérique.
- Les opérations longues SD ne doivent pas bloquer les tâches d'interface.
- Un seul gestionnaire I²C arbitre les accès clavier, IMU et expandeur du Cap.
- Les tâches ont des priorités justifiées et des piles mesurées.
- Les callbacks IRQ sont courts : ils signalent un événement à une tâche.

## Mémoire

- Flash interne : bootloader, table de partitions, firmware, NVS et métadonnées.
- microSD : documents, notes, traces, cartes, médias et sauvegardes.
- Ne jamais charger une carte complète en RAM.
- Éviter les allocations dynamiques répétées dans les chemins critiques.

## Partitionnement initial

La table CSV fournie réserve deux emplacements OTA symétriques et NVS.
La taille réellement disponible pour chaque image doit être vérifiée avec la compilation.
La récupération matérielle et la procédure de mise à jour seront testées avant toute mise à jour OTA.
