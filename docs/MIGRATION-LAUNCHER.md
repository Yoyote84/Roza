# Migration ROZA vers Launcher 3.0.0

## Base amont

Le sous-module `launcher` pointe sur **bmorcelli/Launcher**, tag `3.0.0`, commit `4b06de691df15b0fe6a9f09f56d1d107bbf32b12`. La version précédente utilisait un fork et des marqueurs de code qui ne correspondaient plus au code 3.0.0.

Les personnalisations sont appliquées par `scripts/apply-roza-branding.py`. Le script vérifie le SHA amont attendu, contrôle les marqueurs de remplacement et s'arrête si la source change de façon inattendue. Il est idempotent.

## Personnalisation

- logo bitmap cyrillique ROZA dans l'écran de démarrage ;
- libellés essentiels du menu principal en français ;
- palette ROZA papier/encre/bordeaux ;
- interface Web intégrée rebrandée et traduite pour les commandes principales ;
- choix du réseau Wi-Fi avant la connexion, connexion auto proposée comme action et non exécutée en silence ;
- mot de passe masqué dans les logs série.

Le système natif d'applications, l'OTA, le gestionnaire de partitions, le stockage USB et les fonctions microSD de Launcher 3.0.0 restent la base. Le catalogue séparé d'applications ROZA n'est pas ajouté dans cette étape.

## Schéma 8 MB

Le fichier amont `support_files/custom_8Mb.csv` définit :
- NVS : `0x9000` / `0x5000` ;
- OTA data : `0xE000` / `0x2000` ;
- application `app0` : `0x10000` / `0x150000` ;
- coredump : `0x160000` / `0x10000`.

Ne jamais supposer que la table de partitions présente sur un appareil est identique sans la lire. Ne pas effacer la flash complète. Une mise à jour applicative OTA doit conserver la table et les partitions de données.

## Installation et validation

Aucun flash matériel n'est lancé par ce dépôt ou par la CI. La CI vérifie la compilation et génère les binaires. Elle ne prouve pas le démarrage, le Wi-Fi, le clavier, l'écran, l'OTA ni l'interface Web sur un appareil réel. Ces essais doivent être réalisés et documentés séparément avant de déclarer la version stable.
