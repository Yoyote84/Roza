# Nexus — étape 2/5 : inventaire d'extraction et préparation

Date : 22 novembre 2026
Source : `Yoyote84/Roza` (`main`)
Titre : Nexus

## Inventaire source repéré

### Modules jeu à transférer (en adaptant les frontières firmware)
- `firmware/src/hawkins_app.cpp`, `hawkins_app.h` — écran/contrôleur principal du jeu (fortement couplé ROZA; à refactoriser vers adaptateur UI).
- `hawkins_runtime.cpp`, `hawkins_runtime.h` — boucle et orchestration runtime.
- `hawkins_world.cpp`, `hawkins_world.h` — état monde.
- `hawkins_character.cpp`, `hawkins_character.h` — état personnage.
- `hawkins_missions.cpp`, `hawkins_missions.h` — missions.
- `hawkins_adventure.cpp`, `hawkins_adventure.h` — aventure/choix.
- `hawkins_adventure_store.cpp`, `hawkins_adventure_store.h` — persistence associée.
- `hawkins_exploration.cpp`, `hawkins_exploration.h` — exploration/secteurs; adapter la source de position.
- `hawkins_ai_protocol.cpp`, `hawkins_ai_protocol.h` — protocole du narrateur.
- `hawkins_narrative_memory*`, `hawkins_narrative_store*` — mémoire narrative et stockage.
- `hawkins_save_codec*`, `hawkins_sd_backup*`, `hawkins_nvs*` — sauvegardes et formats; interfaces firmware à remplacer/adapt.
- `firmware/src/hawkins_llm/` — moteur, port, runtime, SIMD, assets du modèle/tokenizer. Les assets doivent être attribués/licenciés et leur taille vérifiée pour les builds ciblés.

### Tests, outils et documentation à transférer/réorganiser
- `firmware/tools/hawkinstest/` — tests de logique, aventure, missions, mémoire narrative.
- `firmware/tools/hawkinsllmtest/` et `firmware/tools/hawkinsaitest/` — harness/tests natifs du moteur.
- `firmware/tools/hawkinsllm/` — scripts de vérification/entraînement/convertisseur; vérifier la pertinence et les licences des corpus avant redistribution.
- `firmware/docs/hawkins-rpg-engine.md`, `firmware/docs/hawkins-missions.md` — documentation gameplay.
- `docs/HAWKINS_AI_*`, `docs/HAWKINS_LOCAL_AI_*`, `docs/HAWKINS_INSTALLATION.md` — docs techniques à reclasser/actualiser.
- Workflows `.github/workflows/hawkins-*` — à réévaluer; ne pas recopier les workflows firmware/Android non pertinents dans Nexus.

## Architecture du nouveau dépôt
Proposition de destination:
- `src/core/`: état du jeu, monde, personnage, missions, aventure, progression et règles déterministes.
- `src/exploration/`: détection des secteurs/zone et interface de localisation (pas le pilote GNSS).
- `src/narrator/`: protocole, contexte et runtime LLM (adaptateurs plateforme).
- `src/storage/`: codec de sauvegarde, mémoire et interface SD.
- `src/platform/cardputer/`: adaptateurs ROZA (UI, clavier, SD, GNSS); séparés du cœur.
- `assets/model/`: poids/tokenizer, manifestes, checksums et notices de licences, si la redistribution est autorisée.
- `tests/`: tests unitaires et hôtes.
- `tools/`: pipeline modèle et utilitaires.
- `docs/`: architecture, installation, formats, provenance et portage.
- `.github/workflows/`: CI autonome.

## À ne pas copier dans le dépôt de jeu
- Les pilotes ROZA génériques de l'écran, clavier, GNSS, LoRa, SD, Wi-Fi et Bluetooth.
- Les autres applications firmware et le launcher ROZA.
- Secrets, données utilisateur, sauvegardes réelles et configurations personnelles.
- Le bridge Android compagnon (sauf preuve d'une nécessité future, et en tant que composant optionnel).

## Préservation et sûreté
- Le code source reste intact dans ROZA tant que Nexus n'est pas extrait et vérifié.
- L'extraction doit conserver l'historique quand faisable, avec attribution et licences; sinon, un snapshot documenté avec références de commits.
- Refactoriser les références et identifiants Hawkins/Stranger Things vers l'univers original Nexus lors de l'étape 3, pas dans cette étape.
- Le noyau déterministe reste la source d'autorité des données de jeu; le narrateur ne modifie pas directement le state.
- Gameplay hors ligne; aucun téléphone ou réseau requis.
- Tests logiciels et build distincts des tests physiques.

## Blocage d'exécution pour la création du dépôt
La connexion GitHub disponible dans cette session peut lire et modifier les fichiers du dépôt ROZA, mais ne propose pas d'action API de création de dépôt. Le navigateur GitHub n'a pas de session authentifiée enregistrée. La création effective de `Yoyote84/Nexus` nécessite de valider la session GitHub dans le navigateur connecté; aucun mot de passe ne doit être partagé dans le chat.

## État de l'étape 2
**Préparation de l'extraction et inventaire terminés.** L'extraction dans un dépôt Nexus nouvellement créé n'est pas encore réalisée, car le dépôt cible doit d'abord être créé avec l'accès GitHub authentifié. Aucune source n'a été déplacée ou supprimée.