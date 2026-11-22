# Étape 5/5 — Firmware final Hawkins pour Cardputer ADV

**État :** firmware compilé, paquet Factory assemblé, contrôles CI et SHA-256 réussis. L'installation et les essais sur le matériel réel restent à effectuer.

## Build validé

- Dépôt : `Yoyote84/Roza`, branche `main`.
- Commit du code livré : `83fadcfbe05127339c3218d09d545101817a2955`.
- Exécution GitHub Actions : [44971906521 — build et publication réussis](https://github.com/Yoyote84/Roza/actions/runs/44971906521).
- Cible : M5Stack Cardputer ADV, ESP32-S3FN8, 8 MB Flash, sans PSRAM.
- Image Factory : `ROZA-Cardputer-ADV-Hawkins-factory.bin`, 5 118 320 octets (`0x4e1970)), adresse de flash `0x0000`.
- Image application seule : `ROZA-Cardputer-ADV-Hawkins-application.bin`, adresse `0x10000` si le bootloader et la table ROZA sont déjà installés.

## Mémoire mesurée au build

PlatformIO a reporté :

| Ressource liée | Utilisé | Budget | Utilisation |
|---|---:|---:|---:|
| RAM statique | 123 404 octets | 327 680 octets | 37,7 % |
| Flash applicative | 5 052 425 octets | 8 257 536 octets | 61,2 % |

La partition applicative commence à `0x10000` et a une capacité de `0x7E0000` (8 257 536 octets). Le workflow vérifie que le binaire application ne dépasse pas cette capacité.

Ces chiffres sont ceux du rapport de liaison PlatformIO ; ils ne sont pas des mesures du heap disponible après démarrage.

## Contenu du paquet

L'artefact du workflow comprend :
- image Factory complète et application seule ;
- bootloader et table de partitions ;
- `BUILD_MANIFEST.json` (commit, cible, capacité, tailles, mémoire liée et provenance du modèle) ;
- `ELF_SECTIONS.txt`, `PLATFORMIO_MEMORY.txt`, `PLATFORMIO_BUILD_REPORT.txt` ;
- `model_manifest.json` et `MODEL_SHA256SUMS.txt` ;
- `INSTALLATION.txt` et `SHA256SUMS.txt`.

Le workflow exécute `sha256sum -c SHA256SUMS.txt` avant le téléversement. Tous les fichiers listés ont été validés (`OK`) dans les journaux du run.

**Téléchargement :** [ouvrir l'exécution 44971906521 et récupérer l'artefact ROZA-Cardputer-ADV-Hawkins-83fadcfbe05127339c3218d09d545101817a2955](https://github.com/Yoyote84/Roza/actions/runs/44971906521). Rétention de l'artefact jusqu'au 20 février 2027.

## Relevés au démarrage

Le firmware affiche maintenant ces trois marqueurs série :
- `[hawkins-llm] HEAP avant modele` ;
- `[hawkins-llm] HEAP apres poids` ;
- `[hawkins-llm] HEAP IA prete`.

Chaque ligne indique le heap interne libre et la taille du plus grand bloc libre. Les valeurs ne seront connues qu'après flash et démarrage du Cardputer physique.

## Essais physiques à faire

1. Flasher le fichier Factory à l'adresse `0x0000` avec un outil compatible ESP32-S3.
2. Vérifier l'écran de démarrage, le menu ROZA et l'ouverture du jeu Hawkins.
3. Vérifier que le narrateur fonctionne sans téléphone, Wi-Fi ni serveur.
4. Observer les trois marqueurs heap sur le port série.
5. Tester les choix du joueur, les missions, les sauvegardes et la continuité du jeu lorsque l'IA est indisponible.
6. Relever le délai d'initialisation et plusieurs délais de génération, puis contrôler la stabilité après plusieurs narrations.

**Limite de validation :** CI, compilation, empaquetage, contrôles SHA-256, tests natifs et rendu de l'interface sont réussis. L'installation, le heap réellement libre, la vitesse d'inférence, la qualité des réponses et la stabilité sur la carte réelle ne sont pas encore validés.
