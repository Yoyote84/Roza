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
4. microSD (lecture seulement au premier essai) ;
5. modules radio/GNSS uniquement lorsqu'ils sont présents et activés volontairement.

Ne pas formater la microSD. Ne pas activer d'émission LoRa automatique. Les essais physiques ne sont pas simulés par la CI.
