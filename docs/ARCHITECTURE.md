# Architecture logicielle

```text
Application / Lanceur
        |
API de services (événements, stockage, périphériques)
        |
+------------------ Noyau РO3A ------------------+
| tâches FreeRTOS | gestion énergie | supervision |
| événements      | configuration   | journal     |
+------------------------------------------------+
        |
+--------------------- Services -----------------+
| affichage | clavier | audio | fichiers | GNSS   |
| radio     | Wi-Fi   | IMU    | batterie | temps  |
+------------------------------------------------+
        |
Abstraction matérielle (BSP Cardputer ADV + Cap)
        |
ESP32-S3FN8 / ST7789V2 / TCA8418 / BMI270 / SD / SX1262 / AT6668
```

## Modules

- `kernel` : cycle de vie, événements, supervision.
- `bsp` : broches et initialisation de la carte.
- `drivers` : pilotes matériels isolés.
- `services` : API stables pour les applications.
- `apps` : applications autonomes.
- `ui` : écran d'accueil, widgets et thème ROZA.
- `storage` : configuration NVS et données microSD.

## Règles de concurrence

- Un seul propriétaire logique du bus SPI partagé.
- Le pilote SD et le pilote SX1262 doivent acquérir le bus SPI via un mutex ou un service commun dont l'efficacité est démontrée par les tests.
- Tous les CS sont inactifs avant de changer de périphérique.
- Les opérations longues SD ne doivent pas bloquer les tâches d'interface.
- Un seul gestionnaire I²C arbitre les accès clavier, IMU et expandeur du Cap.
- Les tâches ont des priorités justifiées et des piles mesurées.
- Les callbacks IRQ sont courts : ils signalent un événement à une tâche.

## Mémoire

- Flash interne : bootloader, table de partitions, firmware, NVS et métadonnées.
- microSD : documents, notes, traces, cartes, médias et sauvegardes, après identification du format.
- Ne jamais charger une carte complète en RAM.
- Éviter les allocations dynamiques répétées dans les chemins critiques.
- Le Cardputer ADV est traité comme une cible sans PSRAM ; chaque allocation volumineuse doit être mesurée.

## Partitionnement réellement utilisé par le firmware actif

La configuration active est `firmware/partitions_8MB.csv`, sélectionnée dans `firmware/platformio.ini`. Elle réserve NVS, PHY init et une seule partition applicative `factory` de 0x7E0000 octets à partir de 0x10000.

**Il n'y a actuellement ni partition `otadata`, ni `ota_0`, ni `ota_1`.** Le firmware ne dispose donc pas d'un partitionnement A/B et ne peut pas garantir de rollback logiciel après une mise à jour. Le fichier racine `partitions.csv` est historique et n'est pas utilisé pour compiler le firmware actif.

## Architecture OTA à envisager à l'étape 3

Une future mise à jour sécurisée devra disposer d'au moins deux partitions d'application OTA et d'une partition OTA Data, vérifier la signature du paquet avant son installation, contrôler le premier démarrage et confirmer la nouvelle image après autotests. Le dimensionnement doit être recalculé sur le binaire effectivement compilé, avec l'ensemble des zones Flash de 8 Mo et la croissance possible du firmware.

L'installation USB-C complète reste le chemin de récupération obligatoire. Ne pas activer de mécanisme Secure Boot/eFuse irréversible sans procédure de provisionnement et de restauration éprouvée.
