# Hawkins — faisabilite de l'IA hors ligne sur Cardputer ADV

## Conclusion materielle

La cible actuelle du projet est le M5Stack Cardputer ADV avec un ESP32-S3FN8, 8 Mo de flash et une carte microSD. Le commentaire de configuration PlatformIO du projet precise egalement que cette carte ne dispose pas de PSRAM.

La microSD peut contenir des poids de modele, mais elle ne remplace pas la RAM de travail dont un moteur neuronal a besoin pour les tenseurs intermediaires, le cache de generation et le contexte. Le guide Espressif indique que l'ESP32-S3 ne dispose que de quelques centaines de kilo-octets de RAM interne ; la PSRAM est une memoire externe optionnelle qui doit etre presente sur le materiel.

Sources de reference :
- M5Stack Cardputer ADV : https://docs.m5stack.com/en/core/Cardputer-Adv
- Espressif, RAM externe ESP32-S3 : https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/external-ram.html
- Espressif esp-tflite-micro : https://github.com/espressif/esp-tflite-micro

## Ce qui est et ce qui n'est pas realiste

- Des modeles embarques specialises et compacts (classification, detection ou reconnaissance de mots/commandes) peuvent fonctionner sur ESP32-S3 avec un modele adapte.
- Cela ne constitue pas un moteur de langage generatif : reconnaitre une commande ou classer une entree ne permet pas de rediger librement des dialogues narratifs en francais.
- Ajouter un fichier de modele sur la carte SD sans RAM suffisante ne rend pas possible l'inference d'un grand modele de langage.
- Le runtime esp-tflite-micro fournit notamment un exemple de reconnaissance de deux mots de 20 Ko ; c'est une preuve de faisabilite pour une tache specialisee, pas pour un narrateur conversationnel.

## Decision pour l'etape 4

Aucun modele generatif ni runtime LLM n'est integre tant qu'un essai mesure ne prouve pas qu'il peut charger, generer et tenir dans la RAM de cette carte sans bloquer les fonctions du jeu. Le projet ne doit pas appeler un moteur de dialogues scriptes « IA ».

Pour une veritable generation narrative locale, il faudra ajouter un calculateur externe disposant de RAM suffisante et relie au Cardputer (ou changer de plateforme pour une carte avec PSRAM et ressources adequates). Une execution sur telephone serait une autre architecture et ne serait pas une IA autonome sur le Cardputer.

## Regles de securite du jeu

Quand un moteur compatible sera disponible :
- le modele recevra un contexte compact produit depuis le personnage, l'inventaire, les missions, l'exploration et la memoire narrative ;
- il proposera du texte et des intentions, jamais des modifications directes de PV, stress, XP, objets ou progression ;
- le moteur Hawkins validera chaque action proposee ;
- le Wi-Fi restera desactive par defaut ; aucune API distante ne sera necessaire au mode hors ligne.
