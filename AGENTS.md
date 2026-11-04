# ROZA — Règles et Orientations pour les Agents de Développement

## Langue Obligatoire

- Toutes les réponses, analyses, rapports, commentaires et documentations doivent être rédigés en français.
- Les noms techniques imposés par les langages, bibliothèques, API et commandes restent dans leur forme originale.
- Ne jamais passer spontanément à l'anglais, même pour les sujets techniques.

## Publication GitHub Automatisée (Règle Permanente)

Après chaque modification technique terminée :

1. Examiner les changements effectués.
2. Exécuter les tests disponibles.
3. Créer un commit Git avec un message descriptif en français.
4. Effectuer automatiquement un push vers `origin/main`.
5. Vérifier sur GitHub que le commit apparaît bien sur la branche `main`.
6. Fournir le rapport suivant :
   - Branche : main
   - SHA complet du commit
   - Lien GitHub du commit
   - Fichiers modifiés
   - Tests exécutés et leurs résultats
   - Compilation : réussie, échouée ou non exécutée
   - Points restant à vérifier sur le matériel réel

7. Ne jamais prétendre qu'une publication ou compilation a réussi sans vérification.
8. Ne jamais utiliser de force-push.
9. Ne jamais supprimer ni écraser les modifications de l'utilisateur pour résoudre un conflit.

Si le push échoue à cause d'un conflit, d'une authentification ou d'une restriction d'accès, s'arrêter et indiquer clairement le blocage.

## Travail par Étapes

- Plusieurs modifications cohérentes peuvent être réalisées au cours d'une même tâche.
- Cependant, chaque étape fonctionnelle terminée doit être publiée sur GitHub avant de commencer une nouvelle étape indépendante.
- Ne pas garder une série de modifications terminées en attente dans l'environnement local.
- L'ordre des étapes suit la feuille de route définie dans `firmware/AGENTS.md`.

## Communication

- Toutes les explications et tous les rapports doivent être en français.
- Annonce clairement à chaque fin de tâche : "Modifications publiées sur GitHub. Vérification externe possible."
- N'utiliser cette phrase que si le push et la vérification sur GitHub ont réellement réussi.

## Priorité Absolue

- La règle de synchronisation GitHub est prioritaire pour tout le développement de ROZA.
- Ne pas demander de confirmation pour chaque commit ou push : cette autorisation est déjà accordée.
- Ne jamais contourner une protection GitHub, une erreur de sécurité ou une restriction d'accès.

## Restrictions de Développement

- Ne jamais créer de faux pilote, de stub ni de fonction simulée présentée comme fonctionnelle.
- Ne pas toucher à un autre dépôt que /public/Roza.
- Ne jamais déplacer de fichiers simplement pour masquer une erreur.
- Réutiliser intelligemment les pilotes open source compatibles.
- Vérifier les licences et conserver les attributions nécessaires.
- Ne jamais supposer qu'un pilote fonctionne matériellement sans test sur la carte.
- Documenter les modifications et les commandes nécessaires pour compiler.

## Dépôt Officiel

- Dépôt : https://github.com/Yoyote84/Roza
- Branche autorisée : main
- Clone local : /public/Roza
- Dossier de firmware actif : /public/Roza/firmware
- Ne jamais créer un autre dépôt ou une autre branche sans autorisation.

## Base active depuis novembre 2026

- Le seul projet firmware actif est `firmware/`.
- Les anciens fichiers racine `CMakeLists.txt`, `main/`, `partitions.csv` et `sdkconfig.defaults` sont conservés comme historique/audit ; ne pas les utiliser ni les modifier pour développer le firmware actif.
- Le firmware actif est un projet ESP-IDF natif C++23.
- Utiliser le composant ESP Component Registry `espp/m5stack-cardputer` version exacte `1.3.11` (licence MIT) comme BSP matériel.
- Ne pas réécrire les pilotes écran, clavier, IMU, batterie, microSD, SX1262 ou GNSS si le BSP les couvre déjà. Étendre proprement le BSP seulement si une fonction manque réellement.
- Ne jamais supposer que le Cardputer ADV et le Cardputer original ont le même clavier : l'ADV utilise TCA8418.
- Ne pas monter ni formater automatiquement la microSD 128 Go avant identification de son système de fichiers.
- Ne jamais activer la radio LoRa en émission au démarrage. Toute émission nécessite une configuration régionale conforme et une antenne installée.
- Le Wi-Fi et le Bluetooth restent inactifs par défaut.
- Toute nouvelle fonction doit être compilée par la CI avant d'être déclarée intégrée.

## Feuille de route

1. G0 — Fondation ESP-IDF propre et compilation reproductible.
2. G1 — Écran, clavier et IMU via le BSP existant.
3. G2 — Batterie et stockage microSD, après vérification des formats et calibrations.
4. G3 — Interface ROZA et navigation au clavier.
5. G4 — GNSS AT6668 et SX1262, réception d'abord, émission seulement après validation réglementaire et matérielle.
6. G5 — Applications hors ligne : carnet, notes, journal, agenda et outils.
