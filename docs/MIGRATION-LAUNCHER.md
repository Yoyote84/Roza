# Migration ROZA vers CardputerADV-Launcher

## Architecture retenue

Le contenu de cette branche remplace l'ancien firmware ROZA par le dépôt complet CardputerADV-Launcher sous forme de sous-module Git. Le commit amont est épinglé par le gitlink launcher ; les sous-modules des bibliothèques du lanceur sont initialisés récursivement.

Les modifications de marque sont appliquées par scripts/apply-roza-branding.py au moment du build. Le script contrôle le texte amont attendu et s'arrête si la source change de façon inattendue. Il est idempotent.

## Partitionnement

Le lanceur utilise support_files/custom_8Mb.csv :
- NVS : 0x9000 / 0x5000
- OTA data : 0xE000 / 0x2000
- Lanceur protégé (test) : 0x10000 / 0x150000
- coredump : 0x160000 / 0x10000

Le reste de la Flash est géré dynamiquement par le gestionnaire de partitions pour les applications et leurs données.

## Installation

Le README amont demande de flasher Launcher-m5stack-cardputer.bin fusionné à l'adresse 0x0. Cette opération remplace le bootloader, la table de partitions et l'application ; les octets de remplissage de l'image fusionnée peuvent réinitialiser la NVS. Les réglages ROZA précédents et les réseaux enregistrés ne seront pas automatiquement migrés.

Aucune action de flash matériel n'est déclenchée par ce dépôt ou par la CI. Vérifier les fichiers de build et les offsets avant toute installation sur le Cardputer.

## Validation

La CI valide uniquement la compilation et le contrôle de la taille de la partition du lanceur. Elle ne prouve pas le démarrage, le Wi-Fi, l'installation d'applications ou les mises à jour sur le matériel réel. Ces essais doivent être documentés séparément.
