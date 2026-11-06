# ROZA CHANINA — Cardputer ADV

**ROZA CHANINA est maintenant construit sur Launcher 3.0.0**, la base officielle de bmorcelli. Ce dépôt conserve l'identité ROZA, sa personnalisation française et sa chaîne de compilation.

## Aperçu web de l'interface

[Ouvrir la maquette interactive ROZA](https://htmlpreview.github.io/?https://raw.githubusercontent.com/Yoyote84/Roza/migration/roza-chanina-launcher/web-preview/index.html).

La maquette présente l'accueil, le choix des réseaux Wi-Fi, les réglages et l'administration Web. C'est un aperçu de conception, pas une capture du firmware exécuté.

## Source du firmware

Le sous-module `launcher/` est maintenant épinglé sur **bmorcelli/Launcher**, tag **3.0.0**, commit `4b06de691df15b0fe6a9f09f56d1d107bbf32b12`. Les sous-modules des bibliothèques amont sont récupérés récursivement.

La version amont est épinglée volontairement : les changements futurs ne seront intégrés qu'après adaptation de la personnalisation ROZA et validation de la compilation.

## Personnalisation ROZA

Le script `scripts/apply-roza-branding.py` applique l'identité ROZA à la compilation :
- logo bitmap cyrillique **РОЗА** sur l'écran de démarrage ;
- libellés essentiels du menu principal en français ;
- palette papier, encre et bordeaux proposée comme thème ROZA ;
- page d'accueil et écran de connexion de l'interface Web aux couleurs ROZA ;
- liste Wi-Fi affichée avant la connexion, avec réseaux enregistrés identifiés et connexion automatique comme action explicite ;
- mot de passe Wi-Fi masqué dans les journaux série.

Le code amont reste intact dans Git : les adaptations ROZA sont conservées ici et réappliquées à chaque build. Les fonctions natives de Launcher 3.0.0 sont gardées.

## Fonctions de Launcher 3.0.0 conservées

- lancement et gestion des applications installées ;
- installation OTA depuis les sources compatibles du Launcher ;
- gestion des fichiers et installation depuis microSD ;
- interface d'administration Web ;
- stockage USB de la microSD ;
- réglages du lanceur et gestionnaire de partitions.

Le catalogue d'applications tiers dépend des dépôts et firmwares publiés en amont. Cette branche n'ajoute pas encore un catalogue ROZA indépendant.

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

Le workflow GitHub Actions de cette branche compile le firmware et publie les fichiers de build en artefact. Une compilation réussie ne prouve pas encore le démarrage ni tous les périphériques sur le Cardputer réel.

## Installation matérielle — prudence

Le schéma amont 8 MB conserve une NVS à `0x9000` (taille `0x5000`), une zone OTA data à `0xE000` et l'application à `0x10000`. Cela ne dispense pas de vérifier la table de partitions présente sur l'appareil avant toute mise à jour. Ne pas effacer la flash complète ; sauvegarder la NVS reste recommandé.

Aucun flash matériel n'est déclenché par ce dépôt ou par la CI. **Ne flashe pas de binaire tant que la compilation CI et les contrôles du package ne sont pas verts.**

## Provenance et licence

Le firmware amont est [bmorcelli/Launcher](https://github.com/bmorcelli/Launcher), distribué sous sa licence d'origine. Voir `launcher/LICENSE` et les licences des sous-modules pour les conditions détaillées. Le script de personnalisation ROZA et sa documentation restent sous licence MIT.
