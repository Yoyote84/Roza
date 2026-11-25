# ROZA — Flash et migration Cardputer ADV

Cible : M5Stack Cardputer ADV, ESP32-S3FN8, Flash 8 Mio, sans PSRAM.

## Règles

- Ne jamais utiliser Erase all, Chip erase ou un effacement complet pour conserver les réglages NVS.
- Ne pas flasher un binaire applicatif à une adresse supposée.
- La microSD ne participe pas aux mises à jour : elle ne sera ni montée, ni formatée, ni écrite.
- L'image complète écrite à 0x0000 réinitialise les préférences NVS ; les fichiers de la microSD restent séparés et ne sont pas inclus dans l'image.

## 1. Migration USB unique de l'ancienne table factory vers A/B

Nécessaire uniquement aux appareils utilisant encore l'ancienne table factory à 0x10000. Cette ancienne table ne possède aucun slot OTA.

Le paquet de migration contient :
- bootloader.bin à 0x0000 ;
- partitions.bin à 0x8000 ;
- firmware.bin à 0x20000 (slot OTA_0) ;
- migrate_to_ota.sh et SHA256SUMS-migration.txt.

Depuis le dossier du paquet :
    
    sha256sum -c SHA256SUMS-migration.txt
    chmod +x migrate_to_ota.sh
    ./migrate_to_ota.sh /dev/ttyACM0 .

Remplace /dev/ttyACM0 par le port série réellement attribué au Cardputer. Sur Android, identifie le port OTG avant d'exécuter le script.

Le script exige que tu saisisses MIGRATE-ROZA. Il efface seulement otadata (0xF000, 8 Kio), écrit ensuite le bootloader, la table et OTA_0, sans effacer la puce. NVS (0x9000..0xDFFF) et PHY (0xE000..0xEFFF) sont préservées. Ne débranche pas le câble et ne coupe pas l'alimentation pendant les écritures. Après migration, redémarre et vérifie ROZA avant toute OTA.

## 2. Image complète USB

Pour une installation neuve, une récupération ou un changement depuis un autre firmware :
1. Vérifie SHA256SUMS-flash.txt.
2. Flashe roza-cardputer-adv-8mb.bin à l'adresse 0x0000.
3. Cette image complète réinitialise les préférences NVS ; elle ne contient pas les fichiers de la microSD et ne la formate pas.

## 3. Mode téléchargement Cardputer ADV

1. Débranche le câble USB.
2. Mets l'interrupteur latéral sur OFF.
3. Maintiens G0.
4. Branche le câble USB-C de données.
5. Relâche G0 quand la carte est alimentée.

## 4. OTA après migration

Après installation de la table A/B et vérification du démarrage :
1. Active Wi-Fi depuis Paramètres et connecte-toi à un réseau mémorisé.
2. Ouvre Paramètres → Mise à jour.
3. Appuie sur R ou Entrée pour vérifier le manifeste signé.
4. Vérifie le numéro de build puis appuie sur Y pour confirmer.
5. Laisse le Cardputer alimenté jusqu'au redémarrage.
6. ROZA ne confirme le nouveau firmware qu'après le contrôle de démarrage ; un échec doit déclencher le retour au slot précédent.

Une OTA signée n'est publiée qu'après configuration du secret Actions ROZA_OTA_SIGNING_KEY_PEM. Sans ce secret, aucune image OTA non signée n'est publiée ou installable.

## 5. Essais matériels — étape 5/5

Tester l'écran, le clavier, le son, le démarrage avec et sans SD, la conservation NVS/SD, GNSS, IMU, SX1262, une mise à jour A/B et un rollback provoqué volontairement sur un appareil de test. La CI ne remplace pas ces essais.
