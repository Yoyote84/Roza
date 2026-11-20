# Décision d'architecture — narrateur IA Hawkins hors ligne

Date de décision : 20 novembre 2026  
Étape : 4 — intégration logicielle du narrateur local

## Décision

Le narrateur IA fonctionne sur le téléphone Android, pas sur le Cardputer. Le M5Stack Cardputer ADV (ESP32-S3 sans PSRAM) conserve la logique du jeu, le personnage, les règles, les missions et les sauvegardes. Le Samsung Galaxy Z Flip6 exécute le modèle GGUF localement avec llama.cpp. Les deux appareils échangent par USB série CDC, via un câble USB-C compatible données / USB Host.

Aucun serveur Wi-Fi, routeur, partage de connexion, cloud ou accès Internet n'est nécessaire au moment de jouer. Le modèle est importé préalablement dans l'application Android ; l'inférence reste ensuite locale.

## Composants retenus

- Moteur d'inférence : binding Android officiel de llama.cpp dans examples/llama.android/lib.
- Révision du moteur : dépôt ggml-org/llama.cpp, commit épinglé d5bbcd1f31dba754f716a26c3a4784502a6b9ede par le sous-module android/llama.cpp.
- Transport USB : usb-serial-for-android 3.12.0, qui fournit la détection série CDC/ACM côté Android USB Host.
- Modèle candidat : Qwen3.5-4B-Instruct Q4_K_M, GGUF multilingue, à importer dans l'application. Le modèle n'est pas livré dans l'APK et n'est pas téléchargé par l'application.
- Android : application compagnon dédiée, ABI arm64-v8a, API minimale 33, Java 17, NDK 29.0.13113456 et CMake 3.31.6.

Les versions du moteur et des dépendances sont épinglées pour rendre la compilation répétable. La compatibilité effective du fichier GGUF et les performances thermiques doivent être vérifiées sur le téléphone à l'étape 5.

## Répartition des responsabilités

### Cardputer — autorité du jeu

Le firmware conserve :
- l'état du monde et ses événements ;
- les secteurs visités et les découvertes ;
- le personnage, les PV, le stress, l'XP et les compétences ;
- les missions, les objets, les drapeaux scénaristiques et les relations PNJ ;
- la mémoire narrative et les sauvegardes NVS / microSD.

Le Cardputer construit un instantané borné à partir des données déjà enregistrées. Les coordonnées GPS brutes ne sont jamais envoyées au téléphone : seul un identifiant de maille haché peut figurer dans le contexte.

### Téléphone — narrateur

L'application :
- importe un modèle GGUF choisi par l'utilisateur dans son stockage privé ;
- charge le modèle dans llama.cpp ;
- attend les demandes envoyées par le Cardputer ;
- produit une narration française et jusqu'à trois choix ;
- renvoie seulement ce texte et ces choix.

Le modèle ne peut pas écrire dans les sauvegardes, modifier des statistiques ou déclarer une mission terminée. Le Cardputer demeure l'autorité et n'accepte aucune commande d'état issue du modèle.

## Protocole USB HKA/1

Transport : USB série CDC, 115200 bauds, 8 bits, sans parité, 1 bit d'arrêt. Chaque trame commence par le préfixe ASCII HKA/1, contient un objet JSON UTF-8 et se termine par LF.

Le Cardputer réutilise le lecteur série central de ROZA. Il ne crée pas de second consommateur de Serial.read(). Les autres commandes série historiques restent limitées à leur longueur d'origine.

### Téléphone vers Cardputer

- hello version 1 : battement de présence envoyé toutes les cinq secondes.
- narration : identifiant de requête, texte borné et tableau de zéro à trois choix.
- error : identifiant de requête et message d'erreur borné.

### Cardputer vers téléphone

- narrate : identifiant unique et instantané JSON du monde / personnage / missions / mémoire / action choisie.

Le firmware vérifie le préfixe, la version, la structure JSON, les limites de taille et l'identifiant de la requête courante. Les trames invalides, inconnues, trop longues ou tardives sont ignorées. Une demande expire après deux minutes.

## Déroulement en jeu

1. Le téléphone envoie des battements hello pendant que la liaison USB est ouverte.
2. Dans l'écran Hawkins, le joueur appuie sur A pour ouvrir le narrateur.
3. Le Cardputer envoie l'état de jeu utile, sans coordonnées GPS brutes.
4. Le téléphone exécute le modèle local et retourne un texte plus des choix.
5. Le joueur choisit 1, 2 ou 3 sur le clavier du Cardputer. Le choix est transmis comme player_action lors de la demande suivante.
6. Le jeu continue à valider et sauvegarder ses propres règles. Une déconnexion ou une erreur IA ne bloque pas le moteur principal.

## Compilation et tests

- Le workflow firmware compile l'ESP32-S3 et exécute hawkinsaitest : construction des trames, parsing, battement, choix, identifiants, messages invalides, contrôle de taille et caractères de contrôle.
- Le workflow Hawkins AI Android construit l'APK debug et exécute les tests Kotlin du protocole.
- Les deux workflows publient leurs propres résultats ; un APK CI ne constitue pas un test USB matériel.

## Limites connues et étape 5

Étape 4 prépare le firmware, le protocole et l'application Android. Elle ne prouve pas encore :
- que le Z Flip6 reconnaît le Cardputer ADV comme port CDC/ACM avec le câble précis utilisé ;
- que l'alimentation USB entre les deux appareils est stable ;
- que le fichier GGUF choisi est accepté par la révision llama.cpp épinglée ;
- le débit de génération, la température, la consommation et la stabilité pendant une session longue ;
- le comportement physique de l'écran et du clavier pendant les échanges.

Ces vérifications nécessitent les deux appareils et constituent l'étape 5. Le jeu doit continuer à fonctionner sans téléphone ; l'IA est un module facultatif.
