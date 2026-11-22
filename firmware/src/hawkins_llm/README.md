# Moteur IA embarqué — port ROZA

Ce dossier contient le moteur d'inférence C++ issu de [therezor/cardputer-ai](https://github.com/therezor/cardputer-ai), adapté à la chaîne PlatformIO + Arduino-ESP32 de ROZA.

## Moteur et modèle

- Source du moteur : REZOR, révision `78c5128672b750977619dc0a6c3b8baed91168ed`, licence MIT.
- Modèle : `MaxLSB/LeCarnet-3M`, révision `0bf134cfceb1b00c7c56a0ebbc1560ec634c3502`, licence MIT.
- Poids : CRDP v3, LLaMA, Q4_0 row-planar.
- Tokenizer : CTK2 GPT-2 byte-level BPE.
- Profil : contexte de 32 tokens ; jusqu'à 9 tokens de texte par réponse, avec un pas supplémentaire réservé au token EOS (10 pas de génération maximum).

## Embarquement Flash

Les fichiers produits à l'étape 3 sont publiés dans `assets/` par le workflow modèle :

- `assets/model_lecarnet_q4.bin`
- `assets/tokenizer_lecarnet_ctk2.bin`
- `assets/model_manifest.json`
- `assets/SHA256SUMS.txt`

`model_data.S` incorpore les deux binaires dans l'image applicative et expose les symboles de début/fin. Le runtime les lit directement depuis la Flash mappée. La microSD n'est pas utilisée pour charger les poids.

## Runtime et application

- `runtime.cpp` valide les limites des binaires, initialise `Transformer`, `Tokenizer` et `Sampler`, vérifie le budget CTK2 avant l'inférence et refuse un prompt trop long au lieu d'en supprimer silencieusement le début.
- `hawkins_app.cpp` construit un prompt court à partir du monde Hawkins, du personnage et de l'action du joueur.
- La génération s'exécute dans une tâche FreeRTOS dédiée sur le cœur 1 ; le moteur garde son worker de calcul Q4 sur le cœur 0.
- Les résultats sont rapatriés dans la boucle principale pour modifier l'écran et la mémoire narrative.

L'IA ne modifie jamais directement les PV, les caractéristiques, l'inventaire, les missions, les compétences ni les sauvegardes. Les propositions visibles sont contrôlées par ROZA.

## Mode de secours

Un chargement invalide, un manque de mémoire ou une génération vide désactive uniquement le narrateur. Le monde Hawkins et ses mécaniques déterministes restent utilisables.

## Limites de validation

L'intégration du code appartient à l'étape 4. Les générations françaises, la mesure de mémoire libre, la vitesse, la stabilité et les essais sur le vrai Cardputer ADV sont reportés à l'étape 5 et aux retours d'essai.