# Hawkins — choix du moteur IA hors ligne (étape 4/5)

État : **recherche et architecture retenue à valider ; moteur pas encore intégré**.

Ce document consigne le choix issu de l'étude des modèles français, des projets de maîtres du jeu IA et des runtimes compatibles avec le matériel réel.

## 1. Contrainte du Cardputer ADV

La cible ROZA est un ESP32-S3FN8 avec 8 Mo de Flash, 512 Ko de SRAM interne et sans PSRAM. La microSD est un stockage bloc/FAT ; elle n'ajoute pas de RAM et ne peut pas être considérée comme une mémoire directement adressable par un moteur qui attend des pointeurs vers un modèle en Flash mappée.

Le projet `therezor/cardputer-ai` démontre qu'un petit LLM de 8 M de paramètres peut tourner sur le même type de carte, avec poids Q4 intégrés à la Flash et un contexte très court. Son modèle TinyTalk est surtout entraîné pour de l'anglais simple et ne remplit pas le besoin d'un maître du jeu francophone complet. Il constitue une preuve technique, pas le modèle final Hawkins.

## 2. Modèles étudiés

### Choix principal à tester : Qwen3.5-4B Instruct en GGUF Q4_K_M

- Modèle multilingue, français pris en charge.
- Raisonnement et capacité à suivre des instructions structurées.
- GGUF Q4_K_M, environ 2,7 Go selon la version proposée par Firn.
- Licence Apache 2.0 pour la version GGUF référencée.
- Ce n'est pas un modèle exclusivement français ni un modèle déjà spécialisé dans Hawkins ; le comportement JDR doit être apporté par le contexte, les règles, les exemples et les tests ROZA.

Références :
- https://huggingface.co/SoAIHQ/Qwen3.5-4B-GGUF
- https://github.com/janik96v/firn-local-ai

### Alternative de français natif : RODIN-1B-Instruct

- Modèle conversationnel entraîné uniquement en français, 1,24 milliard de paramètres.
- Des versions GGUF Q4_K_M existent (environ 860 Mo).
- Licence Apache 2.0.
- Intéressant pour comparer le style et la qualité du français avec Qwen.
- Plus petit, mais sa fiche précise qu'il s'agit d'un modèle expérimental/démonstratif et non d'un grand modèle de raisonnement avancé. Il ne faut pas présumer qu'il suivra mieux des règles de JDR complexes.

Références :
- https://huggingface.co/rodin-llm/rodin-1b-instruct
- https://huggingface.co/mradermacher/rodin-1b-instruct-GGUF

### Projet JDR spécialisé : Amazigh Odyssey — Game Master

- Adaptateur LoRA francophone dédié à la narration, aux PNJ, à l'exploration et aux combats.
- L'adaptateur fait environ 456 Mo, mais dépend de Mistral Nemo Instruct 12B.
- Licence Apache 2.0 annoncée pour l'adaptateur.
- Son modèle de base 12B et son empreinte mémoire rendent ce montage inadapté à la carte ESP32 et trop ambitieux pour un premier déploiement mobile à 12 Go.
- Son intérêt pour ROZA est méthodologique : format des scènes, exemples de PNJ, choix, conséquences, combats et évaluation d'un maître du jeu. Le LoRA ne peut pas être appliqué directement à Qwen3.5.

Référence :
- https://huggingface.co/Ym512/amazigh-odyssey-gm

### Projets de maîtres du jeu

Personal Dungeon Master et DM Loremind montrent des architectures utiles : mémoire de campagne, PNJ, fiches, règles, scènes, jets et contexte documentaire. Ce sont des logiciels de bureau/serveur qui s'appuient sur un moteur IA séparé, pas des firmwares installables sur le Cardputer.

Références :
- https://github.com/danielbsimpson/personal-dungeon-master
- https://github.com/IGMLcreation/LoreMind

## 3. Runtime Android retenu comme candidat : Firn

Firn (anciennement AIpaca) est une application Android open source qui exécute des modèles GGUF localement avec llama.cpp et accélération GPU Adreno. Son catalogue de modèles cite Qwen3.5-4B Q4_K_M (~2,7 Go). L'application expose aussi une API REST locale compatible OpenAI.

Fonctions pertinentes :
- inference locale sans cloud ;
- modèles GGUF importés ou téléchargés dans le téléphone ;
- GPU Adreno/OpenCL ;
- serveur HTTPS sur le port 8443 ;
- appairage client par PIN et clé Ed25519 ;
- certificats TLS auto-signés avec empreinte SHA-256 à épingler.

Limite réseau importante : le serveur Firn écoute l'interface Wi-Fi/Ethernet du téléphone, **pas l'interface hotspot ni VPN**. Pour que le Cardputer le contacte, le téléphone et le Cardputer doivent être sur le même réseau Wi-Fi classique. Ce réseau local peut fonctionner sans accès Internet ; le Wi-Fi du Cardputer restera désactivé hors session IA.

Autre limite API : dans `/v1/chat/completions`, les champs inconnus tels que `tools` sont ignorés. ROZA ne doit donc pas supposer que les outils internes de l'agent Firn sont disponibles via cette route. Le contrat de sortie Hawkins sera un JSON borné que le firmware parse et valide lui-même.

Références :
- https://github.com/janik96v/firn-local-ai
- https://github.com/janik96v/firn-local-ai/blob/main/docs/api-client-guide.md
- https://github.com/janik96v/firn-local-ai/releases/latest

Le Galaxy Z Flip6 (Snapdragon 8 Gen 3, 12 Go de RAM selon sa fiche Samsung) est un hôte plausible pour ce modèle 4B. Aucune mesure de vitesse, température ou autonomie n'a encore été faite sur cet appareil ; le résultat doit être mesuré en pratique.

## 4. Architecture Hawkins proposée

Le Cardputer reste l'application de jeu, le maître de l'état et l'unique source de vérité :

1. Le firmware prépare le contexte avec `formatHawkinsAiContext` et la mémoire narrative existante.
2. Il envoie une requête à Firn sur le réseau local, avec le prompt système Hawkins en français.
3. Le modèle retourne uniquement narration, dialogues, choix et propositions typées.
4. Le firmware parse une réponse JSON bornée, rejette les champs inconnus/mal formés et valide chaque proposition avec les règles existantes.
5. Seul le moteur Hawkins peut modifier PV, stress, XP, inventaire, compétences, missions, relations ou sauvegardes.
6. La mémoire de campagne reste persistante dans la NVS et les archives SD du Cardputer ; la mémoire générale de Firn n'est pas utilisée pour stocker la partie.

Exemple de contrat de réponse à implémenter et tester (pas encore un format livré) :

```json
{
  "narration": "Lia baisse la voix...",
  "choices": [
    {"id": 1, "label": "Lui demander ce qu'elle a entendu"}
  ],
  "proposals": [
    {"type": "npc_dialogue", "npc": "lia", "text": "Tu dois m'écouter."}
  ]
}
```

Les propositions ne sont jamais des ordres de mutation. Un objet consommé, un soin, une récompense ou une mission terminée doit passer par la fonction du moteur correspondante et par ses vérifications.

## 5. Travaux obligatoires avant de déclarer l'étape 4 terminée

- Faire approuver le mode de calcul : le modèle tourne sur le téléphone, tandis que le Cardputer conserve le jeu. C'est le compromis réaliste pour un maître du jeu de bonne qualité ; le modèle n'est alors pas chargé depuis la microSD ni exécuté dans les 512 Ko de SRAM de l'ESP32.
- Intégrer côté firmware le client réseau local, l'appairage Firn, la cryptographie Ed25519, l'épinglage du certificat SHA-256 et la gestion d'horloge. Aucun mode TLS non vérifié ne sera conservé.
- Ajouter un écran de configuration minimal (adresse locale, appairage et statut), sans refaire l'interface générale.
- Implémenter le contrat JSON Hawkins, les limites de taille/temps, le refus sûr en cas de perte Wi-Fi, de réponse invalide ou de modèle absent.
- Ajouter des tests natifs : réponses valides, JSON tronqué, propositions inconnues, actions hors limites, réémission après timeout et absence d'impact sur sauvegardes en cas d'échec IA.
- Tester une batterie de scènes en français (dialogue PNJ, découverte GPS, exploration, choix, combat/conséquences) et mesurer latence, débit, mémoire et température sur le téléphone.
- Vérifier sur le Cardputer que Wi-Fi désactivé, tout le jeu existant reste fonctionnel ; aucune API ni Internet ne doit être requis pour continuer la campagne.

## Décision provisoire

**Qwen3.5-4B Q4_K_M hébergé par Firn est le premier couple modèle/runtime à expérimenter.** RODIN-1B-Instruct est le comparatif français léger. Amazigh Odyssey est une référence JDR pour concevoir les prompts, les données et les évaluations, pas le modèle à installer.

L'étape 4 reste en cours tant que le client Cardputer, l'appairage sécurisé, les réponses structurées, les validations du moteur et les essais réels ne sont pas terminés.
