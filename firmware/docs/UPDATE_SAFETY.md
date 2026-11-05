# Protection contre les paquets de mise à jour invalides

Le paquetage bloque la publication si :
- un des fichiers requis (bootloader, table de partitions, application) est absent ou vide ;
- le bootloader ou la table dépasse sa zone réservée ;
- la table de partitions n'est pas exactement celle de ROZA Cardputer ADV (NVS, PHY et factory) ;
- l'en-tête de l'application est absent, invalide ou cible une puce autre que l'ESP32-S3 ;
- l'application dépasse la capacité de la partition factory ou la Flash de 8 Mio ;
- les fichiers générés échouent à la relecture octet par octet.

La CI exécute des tests négatifs sur les tables modifiées, les partitions qui se chevauchent, les mauvaises cibles, les mauvais en-têtes et les images tronquées.

## Limites

Ces vérifications valident la structure du paquet, pas l'exécution du firmware sur le Cardputer. Une image structurellement valide peut encore rencontrer une régression d'initialisation de l'écran ou d'un périphérique.

Le partitionnement ROZA actuel ne contient qu'une seule partition applicative factory. Il ne peut donc pas conserver simultanément l'ancienne version et la nouvelle, ni effectuer un rollback automatique. Cette fonction demanderait une migration distincte du partitionnement et du bootloader.
