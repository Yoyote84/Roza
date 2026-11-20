# Hawkins AI — compagnon Android hors ligne

Application compagnon du jeu Hawkins sur M5Stack Cardputer ADV.

## Architecture

- Le Cardputer conserve le monde, les règles, le personnage, les missions et les sauvegardes NVS / microSD.
- Cette application Android héberge le modèle GGUF dans la mémoire du téléphone et exécute l'inférence localement avec le binding Android officiel de llama.cpp.
- Les échanges passent par USB série CDC (115200 bauds, 8N1) au moyen du câble USB-C / OTG.
- Aucun serveur local, Wi-Fi, partage de connexion, permission Internet ou service cloud n'est utilisé.
- Le modèle ne peut pas modifier les sauvegardes. Il renvoie un texte et jusqu'à trois choix ; le Cardputer décide ce qu'il accepte.

## Modèle

Importer depuis le sélecteur Android un modèle GGUF compatible llama.cpp. Le candidat recommandé pour le Z Flip6 est **Qwen3.5-4B-Instruct Q4_K_M** (environ 2,7–2,8 Go selon le fichier). Le modèle n'est volontairement pas intégré à l'APK : il doit être obtenu séparément et copié dans le stockage privé de l'application. Le premier import demande donc un fichier déjà présent sur le téléphone ; ensuite l'inférence est hors ligne.

Une taille Q4 de plusieurs gigaoctets nécessite de conserver assez d'espace libre pour la copie et le chargement en mémoire. La fluidité, la température et la consommation doivent être mesurées sur le téléphone réel à l'étape 5.

## Build

Ouvrir ce dossier dans Android Studio avec Android Gradle Plugin 8.13.2 / Gradle 8.14.3, puis construire la tâche app:assembleDebug. Le projet utilise le NDK 29.0.13113456 et CMake 3.31.6. Le module llama-lib est le sous-projet Android amont de llama.cpp, épinglé par le sous-module android/llama.cpp.

Le workflow GitHub Actions Hawkins AI Android construit l'APK debug et publie app-debug.apk comme artefact CI.

## Utilisation

1. Installer l'APK sur le téléphone.
2. Importer le fichier GGUF depuis le stockage local, puis charger le modèle.
3. Allumer le Cardputer et le connecter au téléphone avec un câble USB-C compatible données / OTG.
4. Accorder l'autorisation USB demandée par Android.
5. Dans Hawkins, appuyer sur A IA. Les choix renvoyés par le téléphone se sélectionnent avec 1, 2 ou 3. R relance et Q ferme l'écran du narrateur.

La déconnexion du téléphone ne bloque pas le jeu ; elle désactive uniquement les réponses du narrateur IA.

## Protocole

Chaque trame commence par HKA/1, contient un objet JSON UTF-8 et se termine par LF. Le firmware partage son lecteur USB série avec les commandes ROZA et ne crée pas de second lecteur. Le Cardputer borne les longueurs, vérifie l'identifiant de requête et ignore les réponses tardives ou inconnues.

Le contexte envoyé au modèle comprend des statistiques déjà enregistrées et le résumé mémoire narratif. Les coordonnées GPS brutes ne sont jamais envoyées. Le champ player_action contient le choix de narration fait par le joueur, pas une commande de modification des sauvegardes.

Réponse attendue : {"type":"narration","id":12,"text":"La radio craque...","choices":["Repondre","Suivre le bruit","Se cacher"]}.
