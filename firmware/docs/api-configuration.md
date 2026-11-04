# ROZA — configuration facultative des services API

## Règle du projet

Aucun compte externe, serveur personnel, jeton ou clé API n'est requis pour compiler ROZA.
Les applications restent présentes dans le firmware. Lorsqu'un service nécessite une
configuration absente, seule la fonction distante concernée affiche un message explicite ;
cela ne doit ni bloquer le démarrage, ni empêcher les fonctions locales/hors ligne.

Les valeurs privées sont facultatives et se placent dans `firmware/src/secrets.h`.
Ce fichier est ignoré par Git. Ne jamais ajouter de vraie clé, de jeton ou de mot de passe
au dépôt. Aucun secret fictif ou clé de remplacement n'est fourni.

## Services configurables

| Application | Configuration facultative | Comportement sans configuration |
|---|---|---|
| Chat | `CHAT_BASE_URL`, `CHAT_API_KEY` ; facultatifs : `CHAT_MODEL`, `CHAT_ROOT_CA`, `CHAT_ALLOW_INSECURE` | L'application compile et indique que l'URL ou la clé manque ; aucune requête n'est envoyée. |
| Satellite Radar | `N2YO_API_KEY` | Le radar compile ; la récupération N2YO indique que la clé manque. Les calculs locaux restent disponibles. |
| Router / Clash | `CLASH_BASE`, `CLASH_PROXY_GROUP` ; `CLASH_SECRET` facultatif selon la configuration du serveur | L'application compile. Sans URL, elle indique que le serveur Clash n'est pas configuré. Sans jeton, aucun en-tête Authorization vide n'est envoyé. |
| GitHub Contributions | `GITHUB_USER` (nom de compte, pas une clé API) | L'application compile et indique qu'un nom de compte doit être configuré. |

## Services publics sans clé

Les services suivants utilisent des API publiques sans clé dans l'implémentation actuelle :
ADS-B (adsbdb / adsb.lol), séismes (USGS), météo et qualité de l'air (Open-Meteo),
typhons (JMA), géolocalisation IP (ip-api), cotations (OKX / CoinGecko) et flux de cartes
configurés par URL. Ils ne nécessitent aucun secret pour compiler. Leur disponibilité
dépend d'une connexion réseau, du service distant et de ses conditions d'utilisation.

## Exemple de configuration locale

Créer localement `firmware/src/secrets.h` avec uniquement les valeurs que l'utilisateur
possède réellement. Les valeurs ci-dessous sont volontairement vides : il ne s'agit pas
de clés de démonstration.

```cpp
#pragma once
#define CHAT_BASE_URL ""
#define CHAT_API_KEY ""
#define N2YO_API_KEY ""
#define GITHUB_USER ""
#define CLASH_BASE ""
#define CLASH_SECRET ""
#define CLASH_PROXY_GROUP ""
```

Il est aussi possible de ne définir que les macros nécessaires. `api_config.h` fournit
les valeurs vides par défaut pour toutes les autres.

## Garanties à préserver

- Ne jamais rendre une clé API obligatoire au niveau de la compilation.
- Ne jamais remplacer une clé manquante par une fausse clé ou un jeton factice.
- Ne jamais supprimer une application ou désactiver son code pour faire passer la CI.
- Ne jamais envoyer une requête à une URL vide ou un en-tête d'authentification vide.
- Les appels réseau doivent rester déclenchés par l'utilisation de l'application concernée,
  pas par le démarrage général du firmware.
- Les fonctionnalités hors ligne et les pilotes matériels ne doivent pas dépendre des API.
