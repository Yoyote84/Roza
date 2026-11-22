# Hawkins AI Companion — prototype historique

**Statut : non requis par l'architecture actuelle du jeu Hawkins.**

Cette application Android est conservée dans le dépôt comme ancien prototype d'inférence llama.cpp et de transport série USB HKA/1.

Depuis l'étape 4, le narrateur utilise le modèle français LeCarnet-3M directement dans le firmware du Cardputer ADV. Le jeu n'a plus besoin :
- d'installer cet APK ;
- de télécharger Qwen GGUF sur le téléphone ;
- de connecter le téléphone au Cardputer ;
- du protocole HKA/1.

Ne pas suivre les anciennes instructions de cette application pour installer la version actuelle de Hawkins. La procédure à jour se trouve dans `docs/HAWKINS_INSTALLATION.md`, et ne s'appliquera qu'au paquet complet livré à l'étape 5.

Le code Android est laissé en place pour référence et comparaison technique ; il n'est plus le narrateur de production.