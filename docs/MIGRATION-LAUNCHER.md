# Migration ROZA vers Launcher 3.0.0

## Base amont

Le sous-module `launcher` pointe sur **bmorcelli/Launcher**, tag `3.0.0`, commit `4b06de691df15b0fe6a9f09f56d1d107bbf32b12`. La version précédente utilisait un fork et des marqueurs de code qui ne correspondaient plus au code 3.0.0.

Les personnalisations sont appliquées par `scripts/apply-roza-branding.py`, puis `scripts/apply-roza-interface.py`. Le premier vérifie le SHA amont attendu et contrôle les marqueurs de remplacement ; le second remplace le menu principal après avoir vérifié les marqueurs attendus. Les deux scripts sont idempotents.

## Personnalisation

La maquette de référence est disponible dans `docs/interface-roza.html`. Le firmware reprend maintenant réellement sa direction graphique :

- bandeau du menu principal **РОЗА · CARNET**, avec lettres cyrilliques dessinées en bitmap pour ne pas dépendre de la police embarquée ;
- liste d'actions compacte adaptée à l'écran 240 × 135, avec sélection bordeaux et fond papier/crème ;
- pied d'écran indiquant l'état Wi-Fi et le niveau de batterie lorsque celui-ci est disponible ;
- libellés français pour microSD, OTA, WebUI, USB, partitions et réglages ;
- thème **ROZA - Papier** disponible dans les thèmes et utilisé par défaut pour les nouvelles configurations ;
- migration automatique uniquement si l'ancienne palette amont par défaut est détectée ; tout thème personnalisé reste inchangé ;
- WebUI intégrée assortie à la palette ROZA ;
- choix explicite du réseau Wi-Fi et mot de passe masqué dans les logs série.

La maquette HTML reste une référence graphique, pas une interface de contrôle. Les actions embarquées sont celles du firmware Launcher.

Le Web Screen Viewer a été supprimé : la page GitHub Pages sert uniquement de référence graphique. Le système natif d'applications, l'OTA, le gestionnaire de partitions, le stockage USB et les fonctions microSD de Launcher 3.0.0 restent la base. Le catalogue séparé d'applications ROZA n'est pas ajouté dans cette étape.

## Schéma 8 MB

Le fichier amont `support_files/custom_8Mb.csv` définit :
- NVS : `0x9000` / `0x5000` ;
- OTA data : `0xE000` / `0x2000` ;
- application `app0` : `0x10000` / `0x150000` ;
- coredump : `0x160000` / `0x10000`.

Ne jamais supposer que la table de partitions présente sur un appareil est identique sans la lire. Ne pas effacer la flash complète. Une mise à jour applicative OTA doit conserver la table et les partitions de données.

## Installation et validation

Aucun flash matériel n'est lancé par ce dépôt ou par la CI. La CI vérifie la compilation et génère les binaires. Elle ne prouve pas le démarrage, le rendu exact du bandeau, les couleurs, le Wi-Fi, le clavier, l'écran, l'OTA ni l'interface Web sur un appareil réel. Ces essais doivent être réalisés et documentés séparément avant de déclarer la version stable.
