# ROZA — plan de fusion Launcher 3.0.0 + NucleoOS

## Objectif non négociable

Livrer **un seul firmware nommé ROZA** pour le M5Stack Cardputer ADV. Au démarrage, l'utilisateur arrive dans un seul accueil ROZA, inspiré du menu de NucleoOS. Il n'existe pas de second système NucleoOS à sélectionner, ni de redémarrage pour passer d'un système à l'autre.

La fusion doit préserver les capacités des deux projets. Les fonctions redondantes (Wi-Fi, stockage, gestion des applications, mises à jour, interface Web, etc.) doivent être réunies derrière un service commun, et non supprimées.

## État des bases vérifiées

- ROZA utilise le sous-module **bmorcelli/Launcher 3.0.0**, commit `4b06de691df15b0fe6a9f09f56d1d107bbf32b12`.
- La branche de travail autorisée est `migration/roza-chanina-launcher`.
- NucleoOS utilise un projet **ESP-IDF/CMake** et expose ses fonctions en composants C/C++.
- Launcher 3.0.0 est actuellement construit avec **PlatformIO + framework Arduino**.
- Le mode « M5Launcher guest » de NucleoOS installe un second firmware dans une partition applicative distincte. Il ne constitue donc pas la fusion demandée.
- Les deux projets ont des organisations de partitions différentes pour la flash de 8 MB. La table de partitions doit être repensée et testée avant tout flash.

## Capacités à préserver

### Depuis ROZA / Launcher 3.0.0

- Menu de lancement et gestion des applications.
- Paramètres de l'appareil, clavier, écran, luminosité et économie d'énergie.
- Configuration Wi-Fi et fonctions réseau.
- Mise à jour OTA et catalogue/installation des firmwares compatibles.
- Parcours d'installation depuis la microSD et gestion des fichiers.
- WebUI d'administration.
- Stockage USB de la microSD.
- Gestionnaire de partitions, sauvegarde et restauration lorsque prises en charge par le matériel.
- Thèmes et personnalisation de l'interface.

### Depuis NucleoOS

- Accueil natif en carrousel et navigation au clavier.
- Interface Web/PWA complète et ses applications.
- Registre d'applications, catégories, recherche, favoris et applications récentes.
- Store et installation d'applications Web et natives signées.
- ANIMA et ses modes locaux, navigateur/PC et services distants configurés par l'utilisateur.
- Fonctions vocales, transcription et synthèse vocale.
- Lecteur multimédia, jeux et émulateurs.
- Outils réseau et de sécurité (Wi-Fi, BLE, Ethernet lorsqu'un adaptateur est présent, IR, USB-HID, FIDO et SSH), avec leurs contrôles d'autorisation.
- Capteurs Cardputer ADV, audio et prise en charge de l'écran externe compatible.
- Bus d'événements, notifications, journalisation, gestion de l'énergie et fonctions multi-appareils/ESP-NOW.
- Stockage sur microSD, gestion des données et services Web.
- Mécanismes de mise à jour vérifiée, double banque A/B et retour arrière.

Cette liste sert de matrice de non-régression. Une capacité ne sera marquée comme intégrée qu'après compilation et test fonctionnel adaptés.

## Architecture cible

- Un seul démarrage, un seul firmware ROZA et un seul registre d'applications.
- Un seul accueil ROZA, visuellement inspiré de NucleoOS et recoloré avec l'identité ROZA : papier clair, encre sombre, bordeaux et libellés français.
- Des services communs pour l'écran/clavier, le Wi-Fi, le stockage, les réglages, les applications, l'OTA, la sécurité et l'interface Web.
- Les fonctions propres à chaque projet deviennent des modules ou applications ROZA. Les fonctions communes ont une implémentation unique.
- Aucun firmware NucleoOS séparé ne sera nécessaire pour utiliser ses fonctions.

### Décision technique à prendre par preuve

Il ne faut pas choisir le moteur de compilation sur une supposition. Deux pistes seront évaluées par un prototype minimal :

1. intégrer les composants NucleoOS ESP-IDF dans une base qui conserve le code Launcher/Arduino ;
2. construire ROZA sur ESP-IDF et y porter/embarquer les composants Launcher nécessaires, éventuellement avec Arduino comme composant de compatibilité.

Le prototype devra prouver la compilation, le démarrage, l'accès à l'écran et au clavier Cardputer ADV, le Wi-Fi, la microSD et la marge RAM/flash. La piste qui permet de conserver le plus de code réel et de fonctions avec le moins de risques sera retenue. Pas de faux modules ni de remplacements factices.

## Flash, partitions et données utilisateur

- Cible matérielle : Cardputer ADV, flash 8 MB, sans supposer de PSRAM.
- Les tables de partitions ROZA/Launcher et NucleoOS ne sont pas identiques. Le schéma final doit prévoir la place des applications, des données, de l'OTA et de la récupération.
- Une modification de table de partitions ne doit pas être proposée comme une simple OTA.
- Préserver la NVS, les réglages Wi-Fi et les données utilisateur. Aucun effacement complet de la flash.
- Aucun flash matériel ne sera lancé sans image vérifiée, procédure de récupération et validation explicite.

## Licences et redistribution

NucleoOS est publié sous **PolyForm Noncommercial 1.0.0**. Toute intégration doit conserver le texte de licence et les notices requises. La redistribution commerciale ou l'usage commercial/production nécessiterait une licence commerciale séparée. Les composants tiers gardent leurs propres licences ; le fichier `THIRD_PARTY.md` et les notices GPL/LGPL applicables devront être conservés et respectés.

## Étapes et critères d'acceptation

1. Établir l'inventaire complet des sources, dépendances, fonctions et licences des deux projets.
2. Faire le prototype de compatibilité des deux environnements de compilation.
3. Définir le HAL commun Cardputer ADV et les services de base.
4. Intégrer le registre d'applications et l'accueil unifié ROZA.
5. Intégrer les fonctions NucleoOS et Launcher dans la matrice de fonctionnalités.
6. Résoudre le stockage et la table de partitions sans perte de données.
7. Ajouter des tests de compilation, des tests hôte et des tests de démarrage/récupération.
8. Vérifier physiquement sur Cardputer ADV l'écran, le clavier, le Wi-Fi, la microSD, l'audio, l'OTA et chaque fonction matérielle concernée.

La fusion ne sera considérée terminée que lorsque le firmware unique démarre sur le Cardputer ADV et que chaque fonctionnalité conservée a un résultat de test documenté. Une compilation verte seule ne suffit pas.

## Restrictions Git du projet

- Modifications autorisées uniquement sur `migration/roza-chanina-launcher`.
- Ne jamais modifier `main`, fusionner automatiquement ou travailler sur une autre branche.
- Garder le code réel des projets : pas de stub pour masquer une erreur de compilation.
