# ROZA CHANINA — Cardputer ADV

**ROZA CHANINA est construit sur Launcher 3.0.0**, la base officielle de bmorcelli. Ce dépôt conserve l'identité ROZA et sa personnalisation française.

## ROZA Web Screen Viewer

[Ouvrir ROZA Web Screen Viewer](https://yoyote84.github.io/Roza/).

Le Viewer est adapté à Launcher 3.0.0 : accueil, applications, Wi-Fi, réglages, installateur OTA, carte SD, partitions et interface Web. Il présente un écran 240 × 135, une navigation tactile et clavier ainsi qu'un filtre par catégorie.

**Limite importante :** cette version est un aperçu graphique interactif dessiné dans le navigateur, pas une capture du firmware exécuté sur le Cardputer. Les statuts et actions sont simulés ; le Viewer ne pilote pas l'appareil. Le workflow GitHub Pages publie depuis `migration/roza-chanina-launcher`.

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
