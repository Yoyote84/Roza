# Décision d'architecture — narrateur IA Hawkins hors ligne

> **Décision historique supersédée le 21 novembre 2026.**  
> Le pont IA Android/USB décrit dans ce document n'est plus l'architecture cible. La nouvelle décision est le narrateur génératif embarqué directement sur le Cardputer ADV, basé sur le moteur open source Cardputer AI/REZOR et adapté au jeu ROZA. Voir [l'audit de l'étape 1/5](HAWKINS_LOCAL_AI_STEP1_AUDIT.md).

Ce document est conservé pour l'historique technique du pont Android/HKA/1 et pour expliquer les versions déjà compilées. Il ne doit plus guider les nouveaux développements du narrateur.

## Ancienne architecture (historique uniquement)

Le narrateur IA fonctionnait sur le téléphone Android, avec le Cardputer comme autorité du jeu, via USB série CDC. Cette approche a été testée en compilation mais a entraîné des difficultés répétées de réception et de décodage HKA/1 sur le matériel réel.

La cible nouvelle ne dépend plus de ce pont. Le firmware Hawkins devra exécuter son petit modèle génératif sur l'ESP32-S3 lui-même, sans téléphone, sans serveur et sans réseau.

Pour les détails de la décision actuelle, du modèle candidat, des limites matérielles et du plan en cinq étapes, consulter `docs/HAWKINS_LOCAL_AI_STEP1_AUDIT.md`.
