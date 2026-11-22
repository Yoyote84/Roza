# Ancien protocole HKA/1 — référence historique

**Statut : obsolète pour le jeu Hawkins actuel.**

Le protocole HKA/1 décrivait un ancien pont entre le Cardputer et un compagnon Android. L'étape 4 a remplacé ce chemin : le narrateur utilise désormais le moteur LeCarnet embarqué dans la Flash du Cardputer.

Le firmware n'a plus besoin d'une trame `hello`, `narrate`, `narration`, `ack` ou `nack` pour produire une scène. Le lecteur série est réservé aux commandes ROZA habituelles. Aucun deuxième lecteur série n'est créé.

L'application Android et le code du protocole sont conservés uniquement pour retracer l'ancien prototype. Ne pas les utiliser comme procédure d'installation ou comme dépendance à l'étape 5.

Voir :
- `docs/HAWKINS_LOCAL_AI_STEP1_AUDIT.md` — plan des cinq étapes ;
- `docs/HAWKINS_LOCAL_AI_STEP3_MODEL_FR.md` — modèle français livré ;
- `docs/HAWKINS_LOCAL_AI_STEP4_EMBEDDED.md` — intégration embarquée actuelle.
