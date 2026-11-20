# Installation finale — Hawkins sur Cardputer ADV

Cette livraison comporte deux programmes distincts :

- **ROZA / Hawkins firmware** : le fichier `.bin` à flasher sur le M5Stack Cardputer ADV.
- **Hawkins AI Companion** : l'application Android qui exécute le modèle localement sur le téléphone et communique avec le Cardputer par USB-C.

L'application Android ne remplace pas le firmware du Cardputer. Les deux doivent être installés pour utiliser le narrateur IA.

## Télécharger les fichiers

- [Builds et APK Android — workflow Hawkins AI Android](https://github.com/Yoyote84/Roza/actions/workflows/hawkins-ai-android.yml)
- [Firmware Cardputer ADV et image BIN — workflow ROZA](https://github.com/Yoyote84/Roza/actions/workflows/roza-web-viewer.yml)

Sur chaque page, ouvrez la dernière exécution réussie de la branche `main`, puis téléchargez son artefact :

- Android : `hawkins-ai-android-offline`. Il contient `Hawkins-AI-Companion.apk`, une notice courte et `SHA256SUMS.txt`.
- Cardputer : `ROZA-Cardputer-ADV-Hawkins-<commit>`. Il contient l'image usine, l'image application seule, les fichiers de démarrage et la procédure de flash.

Les artefacts CI sont conservés pendant 90 jours. Téléchargez-les et gardez-en une copie locale.

## 1. Installer le firmware sur le Cardputer

1. Téléchargez et extrayez l'artefact firmware.
2. Pour une installation complète, utilisez **`ROZA-Cardputer-ADV-Hawkins-factory.bin` à l'adresse `0x0000`**. Cette image regroupe le bootloader, la table de partitions et l'application.
3. Branchez le Cardputer ADV en USB-C à un ordinateur ou à un appareil compatible USB-OTG et utilisez un outil de flash ESP32-S3.
4. Vérifiez la procédure et les adresses dans le fichier `INSTALLATION.txt` inclus dans l'artefact.

N'essayez pas de flasher le fichier usine à l'adresse `0x10000`. L'image application seule, elle, est prévue pour `0x10000` et uniquement si le bootloader et la table de partitions ROZA correspondants sont déjà installés.

**Copier le fichier BIN sur la microSD ne flashe pas le microcontrôleur.** Le flash se fait via la liaison USB avec un outil compatible ESP32-S3.

## 2. Installer le compagnon Android

1. Extrayez l'artefact `hawkins-ai-android-offline`.
2. Copiez `Hawkins-AI-Companion.apk` sur le téléphone et ouvrez-le pour l'installer. Android peut demander d'autoriser temporairement l'installation depuis cette source.
3. L'application cible Android 13 ou supérieur ; elle est prévue pour fonctionner sur le Galaxy Z Flip6.
4. Ouvrez Hawkins AI Companion.

L'APK ne contient pas le modèle de langage : ce choix évite d'intégrer plusieurs gigaoctets au programme et de devoir retélécharger le modèle à chaque mise à jour.

## 3. Préparer le modèle local

Le modèle choisi pour cette première livraison est **Qwen3.5-4B Q4_K_M**, au format GGUF :

[Ouvrir le modèle GGUF officiel ggml-org sur Hugging Face](https://huggingface.co/ggml-org/Qwen3.5-4B-GGUF)

Téléchargez le fichier `Qwen3.5-4B-Q4_K_M.gguf` une seule fois, lorsque vous avez une connexion, ou transférez-le sur le téléphone par USB. Sa fiche indique environ 3,03 Go et une licence Apache 2.0.

Dans l'application :
1. Appuyez sur **Choisir un modèle GGUF** et sélectionnez le fichier.
2. Attendez la fin de la copie dans le stockage privé.
3. Appuyez sur **Charger le modèle** et attendez le statut indiquant que l'inférence locale est prête.

L'application copie le modèle dans son espace privé. Il faut donc prévoir environ 3,3 Go d'espace libre supplémentaire pour cette copie, en plus du fichier source. Le modèle doit aussi disposer de suffisamment de mémoire vive pour être chargé.

## 4. Relier le téléphone et le Cardputer

1. Allumez le Cardputer ADV avec le firmware ROZA / Hawkins.
2. Reliez le téléphone au Cardputer avec un câble USB-C compatible données et OTG.
3. Dans Hawkins AI Companion, appuyez sur **Connecter le Cardputer en USB**.
4. Autorisez l'accès USB lorsqu'Android le demande.
5. Dans le jeu, ouvrez le narrateur avec la touche **A**. Utilisez les choix **1**, **2** ou **3** ; **R** relance la scène et **Q** quitte l'écran du narrateur.

La liaison utilise le port série USB à 115200 bauds. Elle ne nécessite ni Wi-Fi, ni partage de connexion, ni données mobiles.

## Fonctionnement hors ligne

Après installation de l'APK, flash du Cardputer et import initial du GGUF, le jeu et son narrateur local sont conçus pour fonctionner sans réseau. Le premier téléchargement des fichiers nécessite toutefois une connexion ou un transfert local préalable.

Le Cardputer reste l'autorité pour les personnages, missions, inventaire, statistiques et sauvegardes. L'IA fournit une narration et des choix ; elle ne modifie pas directement les sauvegardes.

## Limites de validation

Les builds CI vérifient la compilation, les tests logiciels et la génération des paquets. Ils ne prouvent pas encore le fonctionnement sur les deux appareils physiques. Le test complet à réaliser après installation est : charger le modèle sur le Z Flip6, connecter le Cardputer en USB-C, demander une narration, choisir une réponse et vérifier que la partie continue correctement après déconnexion du téléphone.
