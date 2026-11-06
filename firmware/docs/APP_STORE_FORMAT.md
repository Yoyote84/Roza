# Format des applications ROZA

## Objectif

Les applications ROZA sont des paquets de données et de scripts stockés sur la microSD. Elles ne remplacent pas le firmware système et ne nécessitent pas de reflashage pour être ajoutées ou retirées.

Le répertoire réservé est :

```
/ROZA/
  apps/
    identifiant-application/
      manifest.json
      main.lua
```

ROZA ne doit jamais formater la carte pour installer une application. Le gestionnaire ne supprime aucun fichier. Il ne parcourt que les sous-dossiers de `/ROZA/apps`.

## Manifeste version 1

Exemple prévu pour le futur moteur de scripts :

```json
{
  "format": "roza-app",
  "format_version": 1,
  "id": "exemple",
  "name": "Application exemple",
  "version": "1.0.0",
  "description": "Exemple de paquet ROZA.",
  "runtime": "roza-lua",
  "entry": "main.lua"
}
```

Contraintes :
- `id` correspond exactement au nom du dossier et contient uniquement des lettres minuscules, chiffres, tirets ou caractères soulignés.
- `name` fait au maximum 24 caractères.
- `manifest.json` fait au maximum 1 024 octets.
- `entry` doit être un nom de fichier simple, sans chemin absolu, séparateur ou séquence de remontée de dossier.
- Le fichier d'entrée doit exister dans le même dossier que le manifeste.
- Le format accepté est `roza-app`, version 1.

## État de l'implémentation

La première étape ajoute un écran « Apps » qui :
- crée uniquement `/ROZA` et `/ROZA/apps` lorsque la carte est montée et que ces dossiers manquent ;
- lit les manifestes en lecture seule ;
- vérifie les identifiants, les champs obligatoires et le fichier d'entrée ;
- limite l'index mémoire à 12 paquets et trie le résultat ;
- ne lance aucun script, ne télécharge rien et ne supprime rien.

L'exécution des scripts n'est pas encore activée. Le Cardputer ADV ciblé dispose de 8 Mo de Flash et n'a pas de PSRAM ; le moteur devra être choisi et mesuré avec la mémoire réellement disponible avant d'autoriser l'exécution d'applications tierces.

## Étapes suivantes

1. Évaluer un interpréteur Lua compatible avec le compilateur C++11 et les dépendances actuellement verrouillées.
2. Mesurer l'empreinte Flash, le tas libre et le plus grand bloc contigu, sans dégrader l'interface de base.
3. Définir une API ROZA restreinte pour l'affichage, les entrées et le stockage de l'application.
4. Ajouter le catalogue GitHub et les téléchargements HTTPS vers un répertoire temporaire, vérifier SHA-256 et manifeste, puis publier le paquet dans `/ROZA/apps/<id>` uniquement après validation complète.
5. Étudier séparément le mode firmware invité. Un fichier ESP32 `.bin` ordinaire ne doit jamais être traité comme une application microSD.

Les paquets téléchargés devront être considérés comme du code non fiable : aucune API brute GPIO, radio, stockage global ou commande système ne sera exposée automatiquement au moteur.
