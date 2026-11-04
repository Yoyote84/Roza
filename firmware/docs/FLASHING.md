# Flasher РO3A — M5Stack Cardputer ADV

Ce guide concerne exclusivement le **M5Stack Cardputer ADV** (ESP32-S3FN8, 8 Mio de Flash, sans PSRAM).

## Image prête à flasher

Le workflow GitHub Actions « ROZA Firmware Build » publie un artefact nommé
`roza-cardputer-adv-flash-<commit>`. Il contient :

- `roza-cardputer-adv-8mb.bin` : image fusionnée unique, à écrire à l'adresse `0x0000` ;
- `FLASH-INFO.txt` : cible et disposition mémoire ;
- `SHA256SUMS.txt` : empreinte de contrôle.

L'image fusionne les régions réellement compilées :
- bootloader à `0x0000` ;
- table de partitions à `0x8000` ;
- application `factory` à `0x10000`.

Le paquetage vérifie que la partition factory existe et que le binaire applicatif tient dedans. Il ne modifie aucun fichier de l'interface.

## Données réinitialisées par cette image

L'image fusionnée est écrite en une seule fois depuis l'adresse `0x0000`. Les espaces
entre le bootloader, la table de partitions et l'application sont remplis avec `0xFF`.
Cela inclut la partition NVS (`0x9000`), qui contient les préférences et identifiants
enregistrés par le firmware. **Chaque flash de cette image réinitialise donc les réglages
ROZA** (fuseau horaire, luminosité, réseaux Wi-Fi mémorisés, mot de passe du hotspot, etc.).

La microSD est une mémoire séparée : cette image ne l'efface pas et ne la formate pas. Les cartes détaillées ne sont pas incluses dans le binaire : elles se copient séparément sur la microSD, selon `firmware/docs/MAPS_OFFLINE.md`.
Attention : le pilote SD actuel monte la carte en lecture/écriture ; il ne garantit pas un
montage matériellement protégé en lecture seule. La fonction appelée « Format SD » ne
recrée pas le système de fichiers : elle supprime récursivement les fichiers et dossiers.
Ne l'utiliser sous aucun prétexte pendant les premiers essais. Utiliser une carte de test
vide ou une sauvegarde vérifiée. Le pilote Arduino SD courant ne garantit pas la prise
en charge d'une carte SDXC exFAT de 128 Go ; vérifier le format avant de compter dessus.

## Mise à jour USB sans réinitialiser les réglages

Pour une ROZA déjà installée avec la table de partitions actuelle, le workflow publie
un artefact séparé `roza-cardputer-adv-update-<commit>`. Il contient `ROZA_UPDATE.bin`,
qui est uniquement le binaire applicatif destiné à la partition factory.

Conditions impératives :
- Cardputer ADV ESP32-S3FN8, 8 Mio ;
- firmware ROZA déjà installé ;
- table de partitions inchangée (le SHA-256 attendu est indiqué dans `UPDATE-INFO.txt`) ;
- écrire `ROZA_UPDATE.bin` à **`0x10000`** ;
- désactiver toute option « Erase all / Erase flash / Chip erase » dans l'outil.

Cette opération ne contient ni bootloader, ni table de partitions, ni NVS : elle ne
réécrit donc pas ces zones. Les préférences et la microSD ne sont pas volontairement
effacées. Le nouveau firmware peut toutefois effectuer ses propres migrations NVS au
démarrage ; une sauvegarde des réglages importants reste prudente.

**Ne pas utiliser ce paquet** pour passer de M5Launcher ou d'un autre firmware à ROZA,
ni si la table de partitions a changé. Dans ces cas, utiliser l'image complète à
`0x0000` et prévoir la réinitialisation des réglages. Il ne s'agit pas d'une OTA :
le Cardputer doit toujours être connecté en USB et placé en mode téléchargement.

## Entrer en mode téléchargement

1. Débrancher le câble USB.
2. Mettre l'interrupteur latéral sur **OFF**.
3. Maintenir le bouton **G0** (bouton utilisateur).
4. Brancher le câble USB-C de données tout en maintenant G0.
5. Relâcher G0 une fois la carte alimentée : elle est en mode téléchargement.

Cette séquence correspond à la procédure officielle M5Stack pour Cardputer-Adv.

## Flash

Dans un outil de flash ESP32-S3 qui accepte une image binaire unique :

1. Sélectionner la puce **ESP32-S3**.
2. Sélectionner `roza-cardputer-adv-8mb.bin`.
3. Régler l'adresse de flash sur **`0x0000`**.
4. Lancer l'écriture et attendre la confirmation de fin.
5. Débrancher, puis redémarrer normalement (interrupteur ON).

Ne pas sélectionner séparément `firmware.bin` à l'adresse 0 : ce fichier ne contient que l'application, pas le bootloader ni la table de partitions.

## Depuis Android

Le téléphone doit prendre en charge l'USB OTG et le câble doit transporter les données. La prise en charge du port série USB dépend de l'application et du navigateur utilisés ; Chrome Android ne fournit pas le Web Serial natif de bureau. On vérifiera le moyen de flash réellement disponible sur ton téléphone avant de lancer l'opération, plutôt que de te faire installer un outil qui ne voit pas le Cardputer.

## Premier démarrage

Après un flash réussi, vérifier d'abord :
1. démarrage et affichage ;
2. clavier et navigation ;
3. batterie et veille ;
4. microSD : vérifier la détection et la lecture, sans suppression ni écriture ;
5. modules radio/GNSS uniquement lorsqu'ils sont présents et activés volontairement.

Ne pas formater la microSD. Ne pas activer d'émission LoRa automatique. Les essais physiques ne sont pas simulés par la CI.
