# Nexus — déplacement du jeu, étape 1/5 : audit et périmètre

Date : 22 novembre 2026
Dépôt source : `Yoyote84/Roza`, branche `main`
Titre retenu : **Nexus** (nom de travail, vérification de marque à effectuer avant publication)

## Objectif
Isoler le jeu actuellement intégré au firmware ROZA pour en faire un dépôt autonome, tout en gardant ROZA comme cible matérielle d'intégration/compilation. Cette étape est documentaire : elle ne déplace ni ne supprime de code et ne modifie pas le firmware.

## État confirmé du dépôt source
- Le dépôt `Roza` est public et l'accès d'administration est disponible.
- La compilation firmware active est PlatformIO/Arduino-ESP32, cible Cardputer ADV.
- Le suivi du projet dans `docs/IMPLEMENTATION_STATUS.md` confirme les 5 étapes du travail IA local Hawkins, le modèle français embarqué et les tests logiciels/CI réussis.
- La validation physique du Cardputer (démarrage, jeu, génération du narrateur, heap et stabilité) demeure à faire par l'utilisateur.
- Le module de narrateur local est dans `firmware/src/hawkins_llm/`, avec `runtime.cpp`, `runtime.h`, `port.h` et les assets/modèles. Le runtime dépend des symboles d'assets embarqués et des interfaces ESP32/Arduino et ROZA.
- Le jeu utilise les systèmes du firmware ROZA (UI/clavier, sauvegardes/SD, GNSS et LoRa). Les pilotes matériels ne doivent pas être copiés en doublon dans le dépôt Nexus.

## Décision de séparation
Nexus sera un dépôt autonome pour le code, les données narratives, les outils, les tests et la documentation du jeu. ROZA restera le firmware hôte et recevra ultérieurement une version empaquetée/intégrée de Nexus.

## Périmètre à extraire et réévaluer
1. Logique de jeu : monde persistant, événements, missions, progression, choix et état déterministe.
2. Interface et interactions : menus, écrans, clavier et affichage, en identifiant les dépendances ROZA qui doivent rester côté firmware.
3. Exploration et localisation : logique de zones/secteurs et interfaces GNSS, sans réimplémenter le pilote GNSS.
4. Narrateur local : moteur, modèle français, tokenizer, format d'embarcation, prompts et tests. Vérifier licences et attribution des assets avant redistribution.
5. Données et persistance : format de sauvegarde, ressources SD, migrations/versionnement des sauvegardes.
6. Tests : tests natifs, tests d'intégration/hôte, tests de protocole et validation du packaging.
7. CI : workflows de tests du jeu autonome et workflow d'intégration/build du firmware ROZA.

## Plan de migration (étapes 2 à 5)
- **Étape 2/5 — extraction :** créer le dépôt Nexus, copier/extrait le code et l'historique lorsque possible, sans toucher aux composants matériels partagés.
- **Étape 3/5 — originalisation :** remplacer les noms, textes, personnages, lieux, ressources, références et éléments de présentation qui pourraient évoquer trop directement une œuvre existante; définir un univers original. Conserver les mécaniques génériques.
- **Étape 4/5 — liaison :** adapter l'intégration, les dépendances, tests et workflow ROZA afin que le firmware compile avec la version Nexus.
- **Étape 5/5 — livraison :** build final, contrôles CI et paquet installable; essais physiques par l'utilisateur avant déclaration de validation matérielle.

## Garde-fous
- Ne pas supprimer de code du dépôt ROZA avant qu'une version Nexus extraite, vérifiée et compilable existe.
- Pas de double pilote d'écran, clavier, SD, GNSS ou LoRa dans Nexus : définir des interfaces/adaptateurs.
- Jeu utilisable sans Internet, téléphone ou Wi-Fi pendant une partie.
- Sauvegardes SD; aucun formatage/effacement automatique.
- Le moteur déterministe garde la maîtrise de l'état, inventaire, missions, choix et progression; le narrateur ne peut pas muter directement ces données.
- Conserver la preuve de CI et distinguer tests logiciels, build et essais physiques.
- Le titre Nexus est retenu pour le projet, mais une recherche d'antériorité (marques, logiciels, jeux et noms de domaines) reste nécessaire avant une publication commerciale ou publique.

## Conclusion de l'étape 1
**Étape 1/5 terminée :** périmètre, dépendances principales, stratégie d'extraction et garde-fous sont documentés. Aucun déplacement de code n'a encore eu lieu. La suite est la création du dépôt Nexus et l'extraction contrôlée.
