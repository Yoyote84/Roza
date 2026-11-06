# ROZA CHANINA — Cardputer ADV

**ROZA CHANINA est construit sur Launcher 3.0.0**, la base officielle de bmorcelli. Ce dépôt conserve l'identité ROZA et sa personnalisation française.

## ROZA Web Screen Viewer

[Ouvrir ROZA Web Screen Viewer](https://yoyote84.github.io/Roza/).

Le Viewer est désormais une **visionneuse de captures réelles importées** : PNG, JPEG et WebP, navigation entre plusieurs captures, dimensions natives, affichage sans redessiner les pixels. Les fichiers sont traités localement dans le navigateur et ne sont pas téléversés.

### Utilisation

1. Démarrer le firmware ROZA sur le Cardputer ADV.
2. Obtenir une image du véritable écran (capture depuis un outil externe ou photographie).
3. Ouvrir le Viewer et importer le fichier image.
4. Parcourir les captures dans la liste.

Pour une capture sans mise à l'échelle, utiliser une image de 240 × 135 pixels. Une photographie est également acceptée, mais elle conserve les reflets et la perspective.

### Limite technique explicite

Le Viewer ne prétend plus simuler les écrans du firmware. **La capture automatique du framebuffer depuis Launcher 3.0.0 n'est pas encore implémentée ni validée sur le Cardputer ADV.** Le code amont consulté expose l'objet d'affichage et une console série de commandes, mais aucune commande de capture d'écran prête à l'emploi n'a été trouvée. Une lecture matérielle du panneau ne doit pas être supposée possible sans vérifier le câblage et les capacités du contrôleur. Il faut développer et tester une voie de capture adaptée au matériel avant de promettre un transfert automatique USB/Wi-Fi.

Le workflow GitHub Pages publie le Viewer depuis la branche `migration/roza-chanina-launcher`.

## Source du firmware

Le sous-module `launcher/` est épinglé sur **bmorcelli/Launcher**, tag **3.0.0**, commit `4b06de691df15b0fe6a9f09f56d1d107bbf32b12`. Les sous-modules des bibliothèques amont sont récupérés récursivement.

## Personnalisation ROZA

Le script `scripts/apply-roza-branding.py` applique l'identité ROZA : logo cyrillique **РОЗА**, libellés français, palette papier/encre/bordeaux, personnalisation WebUI, sélection Wi-Fi avant connexion et masquage des mots de passe dans les journaux série. Les fonctions natives de Launcher 3.0.0 sont conservées.

## Fonctions conservées

- applications installées et leurs actions ;
- installation OTA depuis les sources compatibles ;
- gestion des fichiers et installation depuis microSD ;
- interface d'administration Web ;
- stockage USB de la microSD ;
- réglages et gestionnaire de partitions.

## Compilation

```sh
git clone --recurse-submodules https://github.com/Yoyote84/Roza.git
cd Roza
git checkout migration/roza-chanina-launcher
git submodule update --init --recursive
python3 scripts/apply-roza-branding.py
pio run -d launcher -e m5stack-cardputer
```

Une compilation réussie ne prouve pas le démarrage ni tous les périphériques sur le Cardputer réel.

## Installation matérielle

Le schéma amont 8 MB conserve une NVS à `0x9000` (taille `0x5000`), une zone OTA data à `0xE000` et l'application à `0x10000`. Vérifier la table de partitions présente sur l'appareil avant toute mise à jour. Ne pas effacer la flash complète ; sauvegarder la NVS reste recommandé.

## Provenance et licence

Le firmware amont est [bmorcelli/Launcher](https://github.com/bmorcelli/Launcher), distribué sous sa licence d'origine. Voir `launcher/LICENSE` et les licences des sous-modules.
