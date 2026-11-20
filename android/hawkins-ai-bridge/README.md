# Hawkins AI — compagnon Android hors ligne

Application compagnon du jeu Hawkins sur M5Stack Cardputer ADV.

## Architecture

- Le Cardputer conserve le monde, les règles, le personnage, les missions et les sauvegardes NVS / microSD.
- Cette application Android héberge le modèle GGUF dans la mémoire du téléphone et exécute l'inférence localement avec le binding Android officiel de llama.cpp.
- Les échanges passent par USB série CDC (115200 bauds, 8N1) au moyen du câble USB-C / OTG.
- Aucun serveur local, Wi-Fi, partage de connexion, permission Internet ou service cloud n'est utilisé.
- Le modèle ne peut pas modifier les sauvegardes. Il renvoie un texte et jusqu'à trois choix ; le Cardputer décide ce qu'il accepte.

## Modèle

Importer depuis le sélecteur Android un modèle GGUF compatible llama.cpp. Le modèle retenu pour la première livraison est [ggml-org/Qwen3.5-4B-GGUF](https://huggingface.co/ggml-org/Qwen3.5-4B-GGUF), fichier `Qwen3.5-4B-Q4_K_M.gguf` (environ 3,03 Go d'après sa fiche). Le modèle n'est volontairement pas intégré à l'APK : il doit être téléchargé ou transféré une première fois, puis copié dans le stockage privé de l'application. Après cet import, l'inférence est hors ligne.

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


## Diagnostic de chargement GGUF

Le workflow applique `patch_llama_android.py` au sous-module llama.cpp épinglé avant la compilation. Ce correctif conserve les messages natifs émis pendant le chargement du modèle et les affiche dans le journal de l'application en cas d'échec. L'application analyse également les métadonnées GGUF et affiche l'architecture détectée. Cela évite de présenter tous les échecs natifs comme une prétendue incompatibilité d'architecture.

Pour compiler localement après un checkout avec sous-modules :

```bash
python3 android/hawkins-ai-bridge/patch_llama_android.py
cd android/hawkins-ai-bridge
gradle :app:assembleDebug
```


## Chargement du backend CPU Android

Le moteur llama.cpp charge ses backends dynamiques depuis `ApplicationInfo.nativeLibraryDir`. L'application configure donc `jniLibs.useLegacyPackaging = true` pour qu'Android extraie les bibliothèques natives lors de l'installation. Sans extraction, le moteur peut trouver zéro plugin CPU dans ce répertoire même si les fichiers apparaissent dans l'archive APK.

Le pont JNI vérifie que le backend CPU est enregistré et tente, en secours, le chargement explicite du plugin ARMv8.0 générique. La CI vérifie également que le plugin CPU ARM64 est présent et compressé dans l'APK pour garantir son extraction à l'installation.


## Génération de narration et délais

Le binding Android limite maintenant la sortie au nombre de tokens demandé : la position courante inclut déjà le prompt utilisateur, donc la longueur de ce prompt ne doit pas être ajoutée une seconde fois à la limite de génération. L'application indique aussi la réception d'une requête du Cardputer et la progression du flux de tokens. En cas d'expiration des 240 secondes, elle renvoie une trame `error` au Cardputer au lieu de le laisser attendre jusqu'à son propre délai de sécurité.
