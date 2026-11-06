# ROZA CHANINA — Cardputer ADV

**ROZA CHANINA** est un firmware pour M5Stack Cardputer ADV construit sur [Launcher 3.0.0](https://github.com/bmorcelli/Launcher), avec une identité visuelle ROZA et une interface française.

## Interface ROZA

[Ouvrir la référence graphique ROZA](https://yoyote84.github.io/Roza/).

La référence définit le langage visuel à retrouver sur le Cardputer : papier crème, encre brun-noir, accents bordeaux, bandeau **РОЗА**, navigation compacte et libellés français. La maquette Web est uniquement une référence de conception ; les actions doivent être exécutées par le vrai firmware.

Le menu principal du firmware est personnalisé pour l'écran 240 × 135 : liste compacte, sélection bordeaux, état Wi-Fi et indication de batterie. Les autres fonctions natives de Launcher restent la base des écrans secondaires.

## Fonctions conservées

- applications et lancement des applications ;
- installateur OTA et catalogue compatible ;
- gestion des fichiers et installation depuis microSD ;
- interface d'administration Web intégrée au firmware ;
- stockage USB de la microSD ;
- réglages et gestionnaire de partitions.

## Source et versions

Le sous-module `launcher/` est épinglé sur **bmorcelli/Launcher 3.0.0**, commit `4b06de691df15b0fe6a9f09f56d1d107bbf32b12`.

Le script `scripts/apply-roza-branding.py` applique l'identité ROZA, la palette papier/bordeaux, les libellés français, le thème WebUI et la personnalisation du parcours Wi-Fi. Le script `scripts/apply-roza-interface.py` remplace le menu principal en grille par une liste adaptée au format 240 × 135. Les deux scripts sont appliqués par le workflow de compilation.

## Compilation

```sh
git clone --recurse-submodules https://github.com/Yoyote84/Roza.git
cd Roza
git checkout migration/roza-chanina-launcher
git submodule update --init --recursive
python3 scripts/apply-roza-branding.py
python3 scripts/apply-roza-interface.py
pio run -d launcher -e m5stack-cardputer
```

Une compilation réussie ne prouve pas le démarrage ni le fonctionnement de tous les périphériques sur le Cardputer réel. La nouvelle interface doit encore être vérifiée physiquement sur l'écran 240 × 135 avant de déclarer la mise à jour validée.

## Mise à jour matérielle

Le schéma amont 8 MB utilise une NVS à `0x9000` (taille `0x5000`), une zone OTA data à `0xE000` et l'application à `0x10000`. Vérifier la table de partitions de l'image avant toute mise à jour. Ne pas effacer la flash complète ; sauvegarder la NVS reste recommandé.

## Licence

Le firmware amont est [bmorcelli/Launcher](https://github.com/bmorcelli/Launcher), distribué sous sa licence d'origine. Voir `launcher/LICENSE` et les licences des sous-modules.
