# Catalogue et installation des applications ROZA

## Fonctionnement

Le gestionnaire Apps consulte un catalogue JSON public du dépôt GitHub ROZA et télécharge les paquets en HTTPS directement sur la microSD. Le firmware principal reste dans la Flash interne : installer ou supprimer un paquet ne reflashe jamais le Cardputer.

Catalogue utilisé par cette branche :
https://raw.githubusercontent.com/Yoyote84/Roza/feature/roza-sd-app-manager/roza-app-catalog/catalog.json

Les URL de fichiers ne sont pas fournies librement par le catalogue. Le firmware les reconstruit à partir de l'identifiant validé, sur l'hôte raw.githubusercontent.com et dans le répertoire du catalogue ROZA. Les paquets sont limités à un manifeste de 1 024 octets et un fichier d'entrée de 96 Kio au maximum.

## Utilisation sur le Cardputer ADV

1. Insérer une microSD montée par ROZA.
2. Connecter le Cardputer à un réseau Wi-Fi depuis Settings.
3. Vérifier que l'heure est synchronisée (NTP), nécessaire à la validation des certificats HTTPS.
4. Ouvrir Apps. Le catalogue est chargé automatiquement si le Wi-Fi est connecté.
5. Appuyer sur t pour basculer entre le catalogue et les applications installées.
6. Dans le catalogue, sélectionner une application puis appuyer sur i. Le manifeste et le fichier d'entrée sont téléchargés en fichiers temporaires et chacun est vérifié par SHA-256 avant installation.
7. Dans l'onglet microSD, sélectionner une application et appuyer sur u, puis Entrée pour confirmer sa suppression.

Navigation : ; ou , vers le haut ; . ou / vers le bas ; Entrée pour les détails ; touche retour pour quitter.

## Installation sûre

- Aucun formatage de carte et aucune suppression hors de /ROZA/apps/<id>.
- Les identifiants et noms de fichier sont validés avant de construire un chemin.
- Le catalogue, les manifestes et les fichiers sont limités en taille.
- TLS utilise le bundle de certificats racines intégré et refuse les téléchargements si l'horloge n'est pas réglée.
- Les fichiers sont téléchargés dans un dossier temporaire masqué. Les deux SHA-256 doivent correspondre et le manifeste doit correspondre aux métadonnées du catalogue avant publication.
- Une mise à jour renomme l'ancienne version en sauvegarde, publie le nouveau dossier, puis supprime la sauvegarde. En cas d'échec du renommage final, le retour arrière est tenté.
- Un téléchargement partiel ou invalide est supprimé ; l'ancienne version est conservée.

Le SHA-256 fourni dans le catalogue vérifie l'intégrité des fichiers par rapport au catalogue HTTPS. Il ne constitue pas une signature indépendante de l'éditeur : la confiance repose aussi sur TLS et sur le dépôt ROZA qui publie le catalogue.

## Format du catalogue

Le fichier catalog.json porte format = roza-app-catalog et format_version = 1. Chaque entrée contient id, name, version, description, runtime, entry, manifest_size, manifest_sha256, entry_size et entry_sha256.

Le firmware prend actuellement en charge le runtime déclaré roza-lua et un seul fichier d'entrée par paquet. Exemple de manifeste :

~~~json
{
  "format": "roza-app",
  "format_version": 1,
  "id": "exemple",
  "name": "Application exemple",
  "version": "1.0.0",
  "description": "Description.",
  "runtime": "roza-lua",
  "entry": "main.lua"
}
~~~

## Limite actuelle importante

Le téléchargement, la vérification, l'installation, la mise à jour et la désinstallation sont implémentés. L'exécution des scripts Lua n'est pas encore intégrée. Les applications peuvent donc être installées et stockées sur la carte, mais pas lancées depuis ROZA dans cette version. Le paquet d'exemple du catalogue sert à tester le parcours d'installation et n'exécute aucun code.

Le Cardputer ADV ciblé utilise un ESP32-S3 avec 8 Mo de Flash et sans PSRAM. Le moteur d'applications devra être ajouté et mesuré séparément avant d'autoriser l'exécution de code tiers. Un firmware ESP32 .bin n'est pas une application microSD et ne doit pas être installé par ce gestionnaire.
