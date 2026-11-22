# Installation finale — Hawkins sur Cardputer ADV

**Ce document décrit la cible de l'étape 5. Aucun firmware intermédiaire ne doit être installé pendant les étapes 1 à 4.**

## Architecture finale prévue

Le narrateur LeCarnet-3M est intégré directement dans le firmware ROZA / Hawkins :

- poids CRDP v3 et tokenizer CTK2 compilés dans l'image applicative ;
- inférence locale sur l'ESP32-S3 du Cardputer ADV ;
- aucune application Android compagnon requise ;
- aucun téléphone, Wi-Fi, serveur ou accès Internet requis pendant une partie ;
- aucune copie du modèle sur microSD nécessaire.

Le compagnon Android présent dans le dépôt correspond à un ancien prototype et ne fait plus partie de la procédure d'installation actuelle.

## Fichiers de livraison

À l'étape 5, le workflow final doit fournir un artefact contenant au minimum :

- `ROZA-Cardputer-ADV-Hawkins-factory.bin` — image complète à flasher à `0x0000` ;
- `ROZA-Cardputer-ADV-Hawkins-application.bin` — application seule, prévue pour `0x10000` uniquement avec le bootloader et la table ROZA correspondants ;
- `bootloader.bin` ;
- `partitions.bin` ;
- `INSTALLATION.txt` ;
- `SHA256SUMS.txt` et manifeste de build.

Les poids du modèle et le tokenizer sont incorporés dans ces binaires. Ils ne seront pas à télécharger séparément après le flash.

## Flash USB-C

1. Télécharger l'artefact Factory de l'étape 5.
2. Connecter le Cardputer ADV par un câble USB-C compatible données.
3. Utiliser un outil de flash ESP32-S3.
4. Pour une installation complète, flasher le fichier Factory à l'adresse `0x0000`.
5. Suivre les adresses précises inscrites dans `INSTALLATION.txt` de cet artefact.

**Copier un fichier BIN sur la microSD ne flashe pas le microcontrôleur.** Le flash s'effectue par USB avec un outil ESP32-S3.

Ne pas flasher une image intermédiaire produite avant l'étape 5. La compilation ou les contrôles CI ne remplacent pas un essai sur le Cardputer ADV réel.

## Contrôles de l'étape 5

Après la livraison du firmware complet, l'essai sur carte devra vérifier le démarrage, l'ouverture du narrateur, une génération française locale, la navigation parmi les choix, la poursuite du jeu sans téléphone ni réseau, les sauvegardes et le comportement lorsque l'IA est indisponible.

Les mesures de mémoire libre, de vitesse et de stabilité seront consignées à ce moment-là.