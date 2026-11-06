# Applications ROZA sur microSD

## Fonctionnement
Le gestionnaire Apps consulte un catalogue JSON public du dépôt ROZA et télécharge les paquets en HTTPS directement sur la microSD. Le firmware principal reste dans la Flash interne. Installer, lancer ou supprimer un paquet ne reflashe jamais le Cardputer.

Catalogue :
https://raw.githubusercontent.com/Yoyote84/Roza/feature/roza-sd-app-manager/roza-app-catalog/catalog.json

Les fichiers sont téléchargés dans un dossier temporaire, contrôlés par taille et SHA-256, puis publiés dans /ROZA/apps/<id>. Une mise à jour conserve l'ancienne version jusqu'à la publication de la nouvelle.

## Utilisation sur le Cardputer ADV
1. Insérer une microSD montée par ROZA.
2. Connecter le Cardputer au Wi-Fi depuis Settings.
3. Synchroniser l'heure par NTP (nécessaire à la validation TLS).
4. Ouvrir Apps et attendre le catalogue.
5. Appuyer sur t pour passer du catalogue aux applications installées.
6. Dans le catalogue, sélectionner une application puis appuyer sur i pour l'installer.
7. Dans l'onglet microSD, sélectionner une application puis appuyer sur l pour la lancer. Entrée ouvre la fiche ; l lance depuis la fiche.
8. La touche retour (backtick) quitte l'application et revient au gestionnaire.
9. Appuyer sur u puis Entrée pour désinstaller.

Navigation : ; ou , vers le haut ; . ou / vers le bas ; Entrée pour les détails ; l pour lancer ; touche retour pour quitter.

## Runtime embarqué
Les paquets runtime roza-lua sont exécutés par Lua 5.4.6 embarqué dans ROZA. Le script est lu directement depuis la microSD. Le contrat d'application est :

~~~lua
function setup()
  -- appele une fois au lancement
end

function loop()
  -- appele environ toutes les 50 ms
end

function draw()
  -- appele a chaque rafraichissement
end

function on_key(key)
  -- caractere, ENTER ou BACKSPACE
end
~~~

draw() est obligatoire ; les autres fonctions sont facultatives.

API disponible :
- roza.clear([rgb565])
- roza.text(texte, x, y, [rgb565])
- roza.text_size(1 ou 2)
- roza.rect(x, y, largeur, hauteur, [rgb565])
- roza.fill_rect(x, y, largeur, hauteur, [rgb565])
- roza.line(x1, y1, x2, y2, [rgb565])
- roza.width(), roza.height(), roza.millis(), roza.key()
- roza.exit()

Les couleurs sont en RGB565 : vert 0x07E0, blanc 0xFFFF.

## Garde-fous
- Un seul script actif à la fois.
- Limite mémoire Lua de 64 Kio ; source maximale de 96 Kio.
- Chaque callback est interrompu après environ 100 ms d'instructions continues.
- Bibliothèques io, os, package, debug et coroutine non ouvertes.
- dofile, loadfile, load, collectgarbage, pcall et xpcall retirées de l'environnement.
- Pas d'accès direct aux fichiers, au Wi-Fi, aux GPIO ou à la radio depuis les scripts.
- Le retour système est réservé à ROZA.
- Les scripts ne peuvent pas reflasher le système ni modifier les partitions.

Le SHA-256 contrôle l'intégrité par rapport au catalogue HTTPS. Il ne remplace pas une signature indépendante de l'éditeur : la confiance repose aussi sur TLS et le dépôt qui publie le catalogue.

## Format du catalogue
catalog.json utilise format roza-app-catalog et format_version 1. Chaque entrée contient id, name, version, description, runtime, entry, manifest_size, manifest_sha256, entry_size et entry_sha256.

Un fichier firmware ESP32 .bin n'est pas une application microSD : le gestionnaire ne lance pas de firmware externe. Le mode firmware invité reste un chantier distinct qui devra préserver le démarrage et la récupération de ROZA.
