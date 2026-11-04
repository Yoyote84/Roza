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
- L'ordre des étapes suit la feuille de route G0→G1→G2→G3→G4→G5.

## Communication

- Toutes les explications et tous les rapports doivent être en français.
- Annonce clairement à chaque fin de tâche :
  "Modifications publiées sur GitHub. Vérification externe possible."
- N'utiliser cette phrase que si le push et la vérification sur GitHub ont réellement réussi.

## Priorité Absolue

- La règle de synchronisation GitHub est prioritaire pour tout le développement de ROZA.
- Ne pas demander de confirmation pour chaque commit ou push : cette autorisation est déjà accordée.
- Ne jamais contourner une protection GitHub, une erreur de sécurité ou une restriction d'accès.

## Restrictions de Développement

- Ne jamais créer de faux pilote, de stub ni de fonction simulée présentée comme fonctionnelle.
- Ne pas toucher à un autre dépôt que /public/Roza.
- Ne jamais déplacer de fichiers simplement pour masquer une erreur.
- Respecter l'architecture propre de ROZA.
- Réutiliser intelligemment les pilotes open source compatibles.
- Vérifier les licences et conserver les attributions nécessaires.
- Ne jamais supposer qu'un pilote fonctionne matériellement sans test sur la carte.
- Documenter les modifications et les commandes nécessaires pour compiler.

## Ordre de Priorité de Développement

1. G0 — Fondation (déjà réalisée)
2. G1.1 — Écran ST7789V2 (en cours correction)
3. G1.2 — Clavier TCA8418
4. G1.3 — Mesure batterie ADC
5. G2 — Bus SPI partagé + I²C communal
6. G3 — Interface graphique complète
7. G4 — GPS AT6668 + Radio SX1262
8. G5 — Applications ROZA (carnet, agenda, etc.)

## Dépôt Officiel

- Dépôt : https://github.com/Yoyote84/Roza
- Branche autorisée : main
- Dossier de travail : /public/Roza
- Ne jamais créer un autre dépôt ou une autre branche sans autorisation.

## État des Fichiers Non Modifiés

- Les fichiers existants ne doivent pas être supprimés ni écrasés sans justification technique valide.
- Les fichiers de documentation et d'audit (MARKDOWN, README, etc.) doivent être préservés et synchronisés avec GitHub.
- Les pilotes et le code source font l'objet d'une révision continue.