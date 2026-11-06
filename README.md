# ROZA CHANINA — Cardputer ADV

**ROZA CHANINA est désormais construit sur CardputerADV-Launcher.** Le moteur, les applications binaires, l'installation depuis microSD/GitHub, l'OTA, l'interface Web et le gestionnaire de partitions proviennent du projet Launcher ; ce dépôt porte la personnalisation ROZA et sa chaîne de compilation.

## Source du firmware

Le dossier `launcher/` est un sous-module Git épinglé sur **AMasetti/CardputerADV-Launcher**, commit `cf4c594c0494a3d4175cf8542b27e10dc611f05c`. Ses trois sous-modules (ArduinoJson, Arduino_GFX et DisplayDrivers) sont récupérés récursivement.

La version exacte est volontairement épinglée : elle cible le Cardputer ADV et ne doit pas être remplacée automatiquement par une version destinée à d'autres cartes.

## Personnalisation ROZA

Le script `scripts/apply-roza-branding.py` adapte le firmware lors de chaque compilation : marque et écran de démarrage ROZA CHANINA, barre d'état et en-têtes ROZA, menus principaux en français, palette olive/encre/ivoire, libellés français pour la boutique d'applications.

Le code du sous-module reste intact dans Git. Les modifications ROZA sont conservées ici et réappliquées à chaque build.

## Fonctions conservées de CardputerADV-Launcher

- APPS : dépôts GitHub, catalogue de versions, téléchargement et installation des applications ;
- OTA : installation de firmwares en ligne ;
- SD : explorateur, copie, déplacement, renommage et installation de binaires ;
- WUI : interface d'administration depuis un navigateur ;
- USB : stockage de la microSD via USB ;
- CFG : configuration du lanceur ;
- PMan : visualisation et gestion des partitions ;
- raccourcis clavier et démarrage rapide des applications installées.

Les applications sont des binaires autonomes prévus pour le système de partitions de Launcher. L'ancien runtime Lua et l'ancien catalogue ROZA ne font plus partie de cette nouvelle base.

## Compilation

Avec PlatformIO 6.1.19 et Python 3.12 :

```sh
git clone --recurse-submodules https://github.com/Yoyote84/Roza.git
cd Roza
git checkout migration/roza-chanina-launcher
git submodule update --init --recursive
python3 scripts/apply-roza-branding.py
pio run -d launcher -e m5stack-cardputer
```

Le workflow GitHub Actions de cette branche effectue ces étapes et publie un artefact contenant l'image fusionnée ainsi que les fichiers séparés de compilation.

## Installation matérielle — attention

Cette migration remplace le système de partitions ROZA actuel par le schéma CardputerADV-Launcher. Le flash de l'image fusionnée au décalage `0x0` réinitialise la NVS : les paramètres Wi-Fi et réglages enregistrés ne sont pas garantis d'être conservés. Le flash n'est pas lancé automatiquement par le dépôt.

Ne flashe pas une image issue d'une compilation non vérifiée. La première validation doit se faire après compilation CI, puis sur un appareil de test ou avec la procédure de récupération disponible.

## Provenance et licence

Ce projet conserve CardputerADV-Launcher comme sous-module, avec ses licences et sous-modules d'origine. CardputerADV-Launcher est un fork du projet [bmorcelli/Launcher](https://github.com/bmorcelli/Launcher), lui-même issu de M5Launcher. Voir launcher/README.md et launcher/LICENSE pour les crédits et conditions détaillés.

Le dépôt ROZA conserve également le script de personnalisation et sa documentation sous licence MIT.
