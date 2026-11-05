# Paquets d'applications ROZA

Le catalogue est charge depuis `firmware/data/apps/catalog.json`. Les paquets sont telecharges depuis GitHub Raw en HTTPS avec verification TLS, puis installes dans `/ROZA/apps/` sur la microSD.

## Format v1

Chaque entree du catalogue contient `id`, `name`, `version`, `description` et `url`. L'URL doit pointer vers `https://raw.githubusercontent.com/`.

Un paquet est un document JSON de la forme :

```json
{
  "format": 1,
  "id": "mon-app",
  "name": "Mon application",
  "version": "1.0.0",
  "pages": [
    {
      "title": "Accueil",
      "text": "Texte affiche sur le Cardputer.",
      "action": "none"
    }
  ]
}
```

Limites : 8 applications dans le catalogue, 8 Ko par catalogue/paquet, 6 pages par application, 32 caracteres pour un titre, 180 caracteres pour un texte. Actions autorisees : `none`, `time`, `files`, `weather`, `gnss`, `calc`, `settings`.

## Securite et compatibilite

Ces paquets sont des mini-applications declaratives ROZA : pages de contenu et raccourcis vers des fonctions ROZA existantes. Ils ne contiennent pas de code natif executable. Un firmware `.bin` ne peut pas etre installe comme plugin avec la partition actuelle : le flasher remplacerait ROZA. Le gestionnaire n'ecrit jamais dans les partitions flash et ne formate jamais la microSD.
