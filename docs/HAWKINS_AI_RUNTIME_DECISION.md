# Décision d'architecture — narrateur IA Hawkins hors ligne

> **Architecture actuelle — étape 4/5.** Le narrateur tourne directement sur le M5Stack Cardputer ADV. Le pont Android/USB HKA/1 est historique et n'est plus requis par le jeu.

## Décision

- Matériel cible : ESP32-S3FN8 du Cardputer ADV, 8 Mo de Flash, sans PSRAM.
- Moteur : port C++ Cardputer AI/REZOR intégré au firmware ROZA.
- Modèle : LeCarnet-3M adapté en français, exporté en CRDP v3 LLaMA Q4_0 row-planar.
- Tokenizer : CTK2 GPT-2 byte-level BPE.
- Stockage : poids et tokenizer intégrés à l'image applicative et lus directement depuis la Flash mappée. Aucun fichier modèle sur la microSD n'est nécessaire.
- Fonctionnement : aucune connexion au téléphone, USB série, Wi-Fi, serveur ou Internet n'est requise pendant une partie.

## Raccordement du firmware

- `firmware/src/hawkins_llm/model_data.S` expose les limites des binaires embarqués.
- `firmware/src/hawkins_llm/runtime.cpp` initialise le moteur, le tokenizer et le sampler, expose un contrôle de budget CTK2 et refuse les prompts trop longs sans les tronquer.
- `firmware/src/hawkins_app.cpp` construit un prompt compact à partir de l'événement Hawkins, du personnage et du choix du joueur.
- Une tâche FreeRTOS dédiée exécute la génération ; la boucle UI principale conserve la gestion des entrées et de l'écran.
- Le protocole compagnon HKA/1 n'est plus utilisé par le jeu et a été retiré du lecteur de commandes série.

## Responsabilités et garde-fous

Le modèle produit uniquement du texte narratif. Les trois propositions exposées par l'interface sont des choix contrôlés par ROZA. Leur sélection est enregistrée dans la mémoire narrative et peut alimenter la narration suivante ; elle n'autorise pas le modèle à modifier les PV, les caractéristiques, l'inventaire, les missions, les compétences ou les sauvegardes.

Si le chargement du modèle ou une génération échoue, le narrateur affiche un état d'indisponibilité et le jeu déterministe reste utilisable.

## Limites matérielles

Le profil utilise un contexte de 32 tokens et réserve 10 pas de génération au maximum : jusqu'à 9 tokens de texte, plus le token EOS, comme dans l'entraînement. Le runtime mesure les prompts avec CTK2 et n'efface jamais silencieusement le début du contexte. La qualité des réponses, la vitesse, le heap réellement libre et la stabilité ne sont pas déclarés validés ici.

## Validation

- L'étape 3 livre les vrais poids et le tokenizer.
- L'étape 4 raccorde ces fichiers au firmware et au jeu.
- Les contrôles de qualité des réponses, les mesures sur la carte, la compilation finale, la vérification Flash/RAM et l'installation sont réservés à l'étape 5, puis aux retours d'essai.

## Ancienne architecture (historique)

L'application Android `android/hawkins-ai-bridge` et le protocole `HKA/1` restent dans le dépôt comme prototype historique. Ils ne sont plus une dépendance du jeu Hawkins.
