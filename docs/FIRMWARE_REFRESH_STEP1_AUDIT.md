# РO3A — Audit actuel de la grande mise à jour VHS (étape 1/5)

**Date de référence :** 25 novembre 2026  
**Dépôt :** https://github.com/Yoyote84/Roza  
**Branche examinée :** `main`  
**Commit de base :** `a18cbbcf1052eab511f001c3df61f0f9a535680b`  
**Cible :** M5Stack Cardputer ADV + Cap LoRa-1262  
**Contraintes confirmées :** ESP32-S3FN8, Flash 8 Mio, absence de PSRAM, écran 240×135, clavier TCA8418, microSD 128 Go à identifier.

> Ce document remplace les conclusions d'étape 1 précédentes qui étaient fondées sur un commit antérieur. Il décrit l'état visible sur `main` au 25/11/2026. Cet audit n'est ni une compilation, ni une validation sur carte physique.

---

## 1. Décision générale

La grande mise à jour doit moderniser **le système et chacune de ses applications**, pas seulement changer les numéros des bibliothèques. Les applications existantes doivent être conservées fonctionnellement. Un remplacement est acceptable uniquement s'il apporte une solution plus récente, maintenue, mieux sécurisée ou réellement plus complète, et si toutes les fonctions utiles sont reprises ou améliorées.

La modernisation se fera en cinq étapes séquentielles :

1. **Audit et préparation** — état réel, inventaire, versions amont, matériel, mémoire, risques et matrice de migration.
2. **Nouveau cœur système** — migration du framework et des bibliothèques, adaptation des API et correction des erreurs de compilation.
3. **Mise à jour intelligente** — dépôt de paquets, manifeste signé, vérification, installation, sauvegarde, OTA A/B et retour arrière.
4. **Nouvelle interface VHS** — identité visuelle VHS/CRT, écran, menu, icônes, typographies, animations et sons facultatifs.
5. **Intégration et test final** — build complet, non-régression des 40 applications, tests réseau/radio/GNSS/SD/audio, BIN, sommes de contrôle et essais sur le Cardputer réel.

**Aucune application n'est autorisée à disparaître pour faciliter une migration.** Une application bloquée par une dépendance doit être corrigée ou portée vers une alternative avant validation de l'étape concernée.

## 2. État du dépôt au commit examiné

### 2.1 Base et chaîne de compilation

- Le projet firmware actif est exclusivement dans `firmware/`.
- La chaîne active est PlatformIO + Arduino-ESP32, environnement `cardputer-adv`.
- Commande de référence : `cd firmware && pio run -e cardputer-adv`.
- Le dépôt utilise actuellement `platform = espressif32@8.0.0`.
- Le build résout Arduino-ESP32 3.3.12 / ESP-IDF 5.5.5.
- PlatformIO Core est épinglé à 6.2.0 dans le workflow firmware.
- M5Unified et M5GFX sont déjà en 0.3.0. NimBLE-Arduino est déjà en 2.5.2.
- Les anciens fichiers racine `main/`, `CMakeLists.txt`, `partitions.csv` et `sdkconfig.defaults` sont historiques : ils ne doivent pas être utilisés pour développer ni compiler le firmware actif.

### 2.2 État de compilation constaté

Le dernier run visible sur le commit de base est :

- Workflow : [ROZA Web Screen Viewer + Cardputer ADV firmware](https://github.com/Yoyote84/Roza/actions/runs/45512359543)
- Conclusion : **échec**.
- La résolution PlatformIO et le test de retrait des anciens composants du jeu passent.
- La compilation s'arrête dans `firmware/src/hotspot.cpp:26`.

Cause exacte : l'appel `dhcp_search_ip_on_mac(staList.sta[i].mac, &ip)` utilise l'ancienne signature. Avec le framework actuellement résolu, `dhcp_search_ip_on_mac()` attend d'abord un pointeur `dhcps_t*`, puis l'adresse MAC et l'adresse IP. Le compilateur refuse donc de convertir `uint8_t*` en `dhcps_t*`.

Conséquences :
- Le commit actuel n'est pas compilable dans ce run.
- Aucun nouveau binaire de firmware n'est produit par ce run.
- La taille Flash/RAM du firmware **actuel** n'est donc pas confirmée.
- Les étapes d'assemblage et de publication du paquet flash sont ignorées.
- Aucune réussite matérielle n'est revendiquée.

Ce défaut sera corrigé à l'étape 2, après validation de la stratégie de migration. Il ne faut pas contourner l'erreur par un stub ou une fonction simulée.

### 2.3 Inventaire vérifié du menu

Le tableau `APPS[]` et les groupes `GROUPS[]` de `firmware/src/globals.cpp` confirment **40 applications réelles, dans 7 groupes** :

| Groupe | Applications | Nombre |
|---|---|---:|
| TOOLS | Time, IMU, Files, Spectrum, Calc, Converter, Player, Hash Oven | 8 |
| SCAN | WiFi Chan, Sniffer, Wardrive, Drone ID, BLE Scan | 5 |
| NET | Hotspot, LAN Scan, NetProbe, Router, SSH, ChatGPT | 6 |
| HID | BT Keys, BT Media, Ducky | 3 |
| SIG | LoRa, GNSS, Map, IR | 4 |
| SKY | Astro, Weather, Planes, Sats, Typhoon, Quake | 6 |
| MORE | GitHub, Radio, FX, OKX, Reader, BadApple, Apps, Settings | 8 |
| **Total** | | **40** |

Le document `firmware/docs/feature-inventory.md` annonce 40 applications / 85 écrans, mais son tableau de groupes est incohérent : il compte seulement 5 applications dans NET, place ChatGPT dans MORE alors que le code la place dans NET, et totalise 39. Le code `APPS[]` / `GROUPS[]` est la référence actuelle : la documentation et le comptage des écrans devront être régénérés à partir du code, sans modifier les applications.

### 2.4 Catalogue d'applications

Le catalogue actuel `firmware/data/apps/catalog.json` contient une seule application déclarative : **Guide ROZA 1.0.0**. Les paquets `.rapp.json` sont des descriptions de mini-applications et non des binaires natifs exécutables.

Le futur magasin devra distinguer clairement :
1. le firmware natif, publié uniquement via des versions vérifiées et signées ;
2. les mini-applications déclaratives, installables sans exécuter de code arbitraire ;
3. les ressources volumineuses sur microSD : cartes, audio, vidéo, documents, données et packs régionaux.

L'objectif est un vrai catalogue enrichi, avec versions, notes, compatibilités, empreintes, dépendances, mise à jour et restauration, sans permettre à un paquet non fiable d'exécuter du code natif.

## 3. Compatibilité matérielle et contraintes

### 3.1 Cardputer ADV

La documentation officielle M5Stack confirme :
- ESP32-S3FN8 double cœur, 240 MHz ;
- 8 Mo de Flash, sans PSRAM externe ;
- écran ST7789V2 240×135 ;
- clavier 56 touches ;
- IMU BMI270 ;
- codec audio ES8311, amplificateur NS4150B, haut-parleur et microphone MEMS ;
- microSD et batterie intégrée de 1750 mAh.

Le budget RAM est la contrainte la plus sensible : TLS, BLE, cartes, audio et gros buffers se disputent la mémoire contiguë. Chaque migration doit fournir une mesure Flash, heap libre, plus grand bloc libre et minimum historique. Les valeurs d'anciens builds ne sont pas des mesures du commit actuel.

### 3.2 Cap LoRa-1262 / GNSS

La documentation M5Stack du Cap LoRa-1262 indique :
- radio Semtech SX1262, plage annoncée 868–923 MHz ;
- antenne RP-SMA adaptée ;
- GNSS ATGM336H-6N avec contrôleur AT6668, NMEA 0183 à 115200 bauds ;
- constellations GPS, QZSS, BeiDou 2/3, Galileo et GLONASS ;
- liaison GNSS UART et radio SPI.

Broches du Cap sur Cardputer ADV :
- SX1262 : RST GPIO3, IRQ/DIO1 GPIO4, NSS GPIO5, BUSY GPIO6 ;
- bus SPI partagé : SCK GPIO40, MOSI GPIO14, MISO GPIO39 ;
- GNSS : RX GPIO15 et TX GPIO13 ;
- expandeur I²C : SDA GPIO8, SCL GPIO9.

Le code ROZA décrit le module comme ATGM336H et lui envoie des commandes propriétaires PCAS. La référence officielle complète est ATGM336H-6N / AT6668 : la compatibilité des commandes PCAS et des modes dynamiques devra être vérifiée sur le vrai Cap. Le fait de recevoir des phrases NMEA ne suffit pas à valider toutes les commandes propriétaires.

Le SX1262 et la microSD partagent physiquement le même SPI. Il faut conserver un seul gestionnaire de bus, gérer les lignes CS et vérifier les transitions de périphérique. Ne pas créer un second bus SPI concurrent. L'expandeur du Cap commande aussi le chemin RF/antenne : son initialisation et le séquencement doivent être validés matériellement.

**LoRa reste en réception par défaut.** Aucune émission automatique ne sera ajoutée. Toute émission éventuelle devra être soumise à une validation distincte de fréquence, puissance, antenne, duty-cycle et réglementation France/UE.

### 3.3 microSD 128 Go

L'application active utilise actuellement l'API Arduino `SD.h`. Les audits précédents indiquent un montage en lecture/écriture et ne démontrent pas la prise en charge exFAT de cette carte SDXC. Le système de fichiers réel de la carte utilisateur est inconnu.

Candidat technique à tester : **SdFat 2.3.1**, qui prend en charge FAT16/FAT32/exFAT et a corrigé le traitement de champs exFAT. Ce n'est pas un remplacement automatique : il faut mesurer sa taille, vérifier les API utilisées par toutes les applications, préserver le SPI partagé et tester d'abord sur une carte de test.

Règles non négociables :
- identifier le format réel avant toute opération ;
- ne jamais formater automatiquement ;
- ne pas effacer ou réorganiser la carte personnelle ;
- sauvegarder avant tout test d'écriture ;
- séparer les données de l'utilisateur, les ressources, les sauvegardes et les paquets de mise à jour.

## 4. Versions : état actuel et dernières bases stables candidates

Vérification des sources officielles/amont effectuée le 25/11/2026. Une release stable récente n'est pas automatiquement compatible : chaque changement sera épinglé, compilé proprement et testé avant d'être adopté.

| Composant | Version dans ROZA | Version stable candidate / constat | Décision |
|---|---:|---|---|
| PlatformIO Core | 6.2.0 | 6.2.0 dans le workflow actuel | Conserver et épingler ; reproduire localement et en CI. |
| PlatformIO Espressif officiel | 8.0.0 | 8.0.0, base Arduino 3.3.12 / IDF 5.5.5 | Base officielle actuelle du dépôt ; alternative conservatrice. |
| pioarduino communautaire | non utilisé | 61.04.00 : Arduino 4.0.0 / IDF 6.1.1 ; 55.03.313 : Arduino 3.3.13 / IDF 5.5.6 | Candidat de migration à éprouver en environnement séparé, puis épingler exactement. |
| Arduino-ESP32 | 3.3.12 | 4.0.0 stable, IDF 6.1.1 | Cible prioritaire de test, pas encore validée par ROZA. Ruptures importantes à corriger. |
| ESP-IDF | 5.5.5 | 6.1.1 stable | Tester les API réseau, TLS, DHCP, BLE, USB, SPI/I²C, audio et FreeRTOS. |
| M5Unified | 0.3.0 | 0.3.0 | Déjà à jour. Garder. |
| M5GFX | 0.3.0 | 0.3.0 | Déjà à jour et à synchroniser avec M5Unified. Contrôler rendu, DMA et rotation. |
| NimBLE-Arduino | 2.5.2 | 2.5.2 | Déjà sur la dernière release trouvée. Garder et tester cycles scan/HID/démontage. |
| RadioLib | 7.7.1 | 8.0.0 | Migration nécessaire : nouvelle configuration en structure, paramètres entiers, TCXO enum et API IRQ remaniée. |
| FastLED | 3.10.3 | 3.10.10 | Candidat de mise à niveau ; mesurer Flash/RAM et valider RMT / LED SK6812 avec audio. |
| ESP8266Audio | 1.9.9 | 2.4.2 | Candidat de mise à niveau ; corrections WAV, AAC et I²S, mais tester tous les lecteurs sur la cible sans PSRAM. |
| ArduinoJson | 7.4.3 | 7.4.3 | Déjà sur la dernière release trouvée ; conserver. |
| TinyGPSPlus | paquet PlatformIO 1.1.0 | Dépôt amont : v1.0.3a | Écart de numérotation à élucider (source exacte, commit et paquet réellement résolu) avant toute modification. |
| LibSSH-ESP32 | 5.9.0 | Port amont testé explicitement jusqu'aux cores 2.0.17 et 3.0.7 seulement ; wolfSSH 1.6.0 existe | Point de risque majeur : tester le port actuel sous core 3.3.12 et 4.0.0. Comparer wolfSSH sans supprimer SSH. |
| SdFat | absent | 2.3.1 | Candidat pour exFAT 128 Go, uniquement après tests d'intégration et sauvegarde. |

### Choix du cœur recommandé

**Premier candidat :** pioarduino 61.04.00 + Arduino-ESP32 4.0.0 + ESP-IDF 6.1.1. C'est la génération la plus récente observée dans les releases stables au 25/11/2026. Elle provient d'une plateforme communautaire PlatformIO, alors que ROZA utilise aujourd'hui la plateforme officielle Espressif 8.0.0 : le gain de fraîcheur doit être mis en balance avec la reproductibilité de l'outil.

**Alternative de repli :** pioarduino 55.03.313 + Arduino-ESP32 3.3.13 + ESP-IDF 5.5.6 ; autre solution conservatrice : plateforme officielle 8.0.0 + Arduino 3.3.12 + IDF 5.5.5. Cette seconde génération peut être préférée si le port IDF 6 bloque une dépendance critique.

La stratégie correcte est de construire un petit firmware de référence, puis de porter le projet par lots. Ne pas modifier toutes les dépendances en même temps : on doit pouvoir attribuer chaque régression à une seule migration.

## 5. Audit des 40 applications — améliorations prévues

Les noms et fonctionnalités existants restent la base de non-régression. Les améliorations ci-dessous sont des cibles d'étude pour les étapes suivantes, pas des fonctionnalités déjà installées.

| Application | État / migration à étudier |
|---|---|
| **Time** | Conserver les cadrans et l'agenda. Ajouter date/heure manuelles fiables, fuseau Europe/Paris, changement été/hiver et synchronisation optionnelle NTP/GNSS. Le temps local doit rester utilisable sans Wi-Fi. |
| **IMU** | Garder BMI270/M5Unified. Revoir calibration, orientation, filtre, économie d'énergie et diagnostic capteur. |
| **Files** | Conserver toutes les opérations existantes. Tester SdFat/exFAT pour 128 Go, navigation plus rapide, recherche, copie, déplacement, informations et sauvegarde non destructive. |
| **Spectrum** | Garder FFT, barres et waterfall. Vérifier latence micro, buffer, LED et audio après migration M5Unified/M5GFX. Étudier export de mesures. |
| **Calc** | Conserver calcul local hors ligne ; ajouter fonctions scientifiques et conversions sans alourdir le démarrage. |
| **Converter** | Élargir les catégories et vérifier les facteurs/unités ; le mode réseau ne doit pas devenir nécessaire aux conversions locales. |
| **Player** | Conserver WAV. Évaluer MP3/AAC locaux avec ESP8266Audio 2.4.2, métadonnées, playlists et reprise ; confirmer le budget RAM et les formats pris en charge. |
| **Hash Oven** | Conserver les modes historiques en les signalant comme pédagogiques/CTF. Étudier chiffrement authentifié moderne (AES-GCM ou ChaCha20-Poly1305), SHA-2 et HMAC ; ne pas présenter RC4/MD5 comme sûrs pour un usage réel. |
| **WiFi Chan** | Garder l'analyse passive et ses détails. Corriger le cycle radio pour que le scan soit une action explicite et que la radio soit ensuite arrêtée. |
| **Sniffer** | Préserver le mode promiscuous et ses statistiques. Améliorer filtres, compteurs, export et garde-fous de durée/consommation. |
| **Wardrive** | Conserver la collecte explicitement lancée par l'utilisateur. Améliorer format CSV/JSON, journalisation robuste et protection de la vie privée des traces GPS. |
| **Drone ID** | Conserver Remote ID. Vérifier les formats d'annonces Bluetooth/Wi-Fi récents, les décodeurs et les tests de non-régression. |
| **BLE Scan** | NimBLE 2.5.2 est déjà à jour. Conserver les scans passifs, filtres et détails ; mesurer le coût RAM et tester la sortie/réinitialisation BLE. |
| **Hotspot** | Corriger d'abord l'appel DHCP qui bloque la compilation. Ensuite améliorer gestion des clients, bail IP, sécurité du mot de passe, QR et coexistence AP/scan. |
| **LAN Scan** | Conserver les fonctions locales. Moderniser temporisations, concurrence, reprise après échec et métriques ; ne pas monopoliser les sockets ou le heap. |
| **NetProbe** | Conserver les tests réseau locaux. Améliorer profils, résultats, timeout, et protocoles réellement compatibles avec lwIP/IDF cible. |
| **Router** | Garder l'intégration Clash/Mihomo facultative. Vérifier l'API du contrôleur, l'authentification, les erreurs et les valeurs configurables sans secret dans Git. |
| **SSH** | Conserver terminal et authentification. Valider libssh-ESP32 sur les nouveaux cores ; tester wolfSSH 1.6.0 comme alternative si le port actuel est bloqué. Vérifier clé d'hôte, authentification et SCP. |
| **ChatGPT** | Remplacer le chemin OpenAI historique Chat Completions par la Responses API pour le fournisseur OpenAI, en gardant un mode compatible pour les passerelles qui en ont besoin. Choix du fournisseur/modèle configurable ; TLS strict ; réponses limitées et cache. Clé API jamais publiée dans Git. |
| **BT Keys** | Conserver le clavier HID BLE. Mettre à jour profils, reconnexion, état des touches et sécurité d'appairage avec NimBLE actuel. |
| **BT Media** | Conserver les commandes multimédias. Vérifier compatibilité AVRCP/HID côté hôtes et cycle d'appairage. |
| **Ducky** | Conserver les scripts de frappe. Ajouter aperçu, validation, arrêt immédiat et avertissement avant exécution sur un appareil jumelé. |
| **LoRa** | Porter RadioLib 8.0.0 si retenu. Garder réception par défaut, profils régionaux et logs SD ; étudier une réception décodée compatible Meshtastic comme mode distinct, sans promettre une implémentation complète sur 320 Ko RAM. |
| **GNSS** | Conserver navigation, vitesse, altitude, qualité et temps GNSS hors ligne. Vérifier NMEA multi-constellation et les commandes PCAS du modèle exact ATGM336H-6N/AT6668. |
| **Map** | Priorité aux cartes locales WGS-84 sur SD. Revoir la prise en charge exFAT ; proposer des packs de tuiles JPEG adaptés et un téléchargement HTTPS manuel/optionnel. Ne pas faire du réseau une condition de la carte. |
| **IR** | Conserver l'émetteur intégré et ses protocoles. Ne pas promettre l'apprentissage/réception IR : le Cardputer ADV documenté possède un émetteur, pas un récepteur dédié. |
| **Astro** | Conserver les calculs hors ligne. Actualiser les algorithmes/éphémérides utiles, ajouter choix de lieu, date et fuseau, avec cache local. |
| **Weather** | Remplacer les URL HTTP par HTTPS. Enrichir données actuelles, horaires, journalières et qualité de l'air ; Open-Meteo/Météo-France offre des modèles pertinents pour la France. Cache local et dernier résultat disponible hors ligne. |
| **Planes** | Maintenir l'accès ADS-B Internet comme service externe, pas comme récepteur radio ADS-B. Utiliser HTTPS, cache, gestion 429/Retry-After et respecter la limite du service choisi. |
| **Sats** | Ne plus rendre N2YO obligatoire. Étudier CelesTrak GP/OMM en JSON ou KVN, puis calcul orbital local (SGP4) et cache SD ; conserver N2YO comme source facultative si l'utilisateur fournit sa clé. Prendre en charge les identifiants NORAD à six chiffres. |
| **Typhoon** | Conserver la source officielle JMA et la carte des trajectoires. Ajouter cache, actualisation contrôlée, état/heure de mise à jour et source de secours seulement si elle apporte une donnée vérifiable. |
| **Quake** | Conserver USGS GeoJSON. Ajouter filtres par magnitude, distance et période, tri et cache hors ligne ; éviter de télécharger des flux trop volumineux. |
| **GitHub** | Garder la contribution heatmap et ajouter dépôts, versions/releases, issues et activité du compte. Cache et requêtes groupées ; token facultatif car l'API publique non authentifiée est limitée à 60 requêtes/heure, contre 5 000 avec authentification personnelle. |
| **Radio** | Remplacer les seules stations HTTP codées en dur par recherche Radio Browser, favoris et pays/genre/codec. Ajouter flux HTTPS avec validation des certificats, ainsi que la gestion des pertes réseau et du buffer. |
| **FX** | Migrer Frankfurter v1 vers v2 (v1 est dépréciée). Ajouter devises, actifs couverts, horodatage, source, cache et affichage du dernier taux disponible hors ligne. |
| **OKX** | Conserver le cours P2P USDT/CNY comme source distincte si son endpoint reste exploitable. Ajouter un mode marchés au comptant via API publique documentée OKX v5, avec limites de requêtes et solution de repli. |
| **Reader** | Conserver lecture et position. Améliorer UTF-8, gros fichiers, recherche, signets et reprise de lecture depuis la SD. |
| **BadApple** | Conserver la lecture BAP1 actuelle. Le fichier `/video/badapple.dat` n'est pas fourni dans le catalogue de ressources contrôlé ; prévoir un pack ou un convertisseur pour médias que l'utilisateur possède. Étudier une vidéo JPEG/MJPEG basse résolution avec audio séparé seulement après mesures réelles : le Cardputer n'a pas de PSRAM et ne doit pas être présenté comme un lecteur MP4 généraliste sans preuve. |
| **Apps** | Étendre le catalogue au-delà du seul Guide ROZA. Distinguer mini-apps déclaratives, packs de ressources et firmware natif signé ; afficher versions, dépendances, compatibilité, intégrité, sauvegarde et mise à jour. |
| **Settings** | Enrichir réglages radio/réseau, date/heure, énergie, stockage, sauvegarde, maintenance, mise à jour et récupération. Wi-Fi/BT arrêtés par défaut. |

## 6. Défauts transversaux prioritaires

### Priorité critique

1. **Build cassé** : signature DHCP dans `hotspot.cpp`.
2. **Politique Wi-Fi contradictoire** : `setup()` appelle `bootWifiStart()`; `wifi_net.cpp` démarre automatiquement la connexion au réseau mémorisé, active l'auto-reconnexion et laisse la radio allumée même après timeout. Cela contredit l'objectif hors ligne et la règle de connexion volontaire. À corriger à l'étape 2 : Wi-Fi et Bluetooth arrêtés au démarrage ; connexion seulement sur action utilisateur ou tâche de mise à jour explicitement autorisée ; réseaux enregistrés uniquement ; échec sans effet sur les applis locales.
3. **OTA A/B absent** : la table `partitions_8MB.csv` ne contient qu'une partition `factory`. Pas de `ota_0`, `ota_1` ni `otadata`; le paquet `ROZA_UPDATE.bin` est une image app-only pour flash USB, pas un moteur OTA.
4. **Signature absente** : SHA-256 permet de détecter un fichier corrompu, mais ne prouve pas l'identité de l'auteur si le binaire et le manifeste peuvent être remplacés ensemble.
5. **Carte SD** : format inconnu ; montage actuel non prouvé exFAT ni réellement lecture seule.

### Priorité élevée

- **Chat** : secrets définis dans `secrets.h` au moment de la compilation, modèle par défaut codé en dur, pas de configuration depuis l'interface. Le mode TLS sans vérification existe en option explicite (`CHAT_ALLOW_INSECURE`) et doit rester interdit dans la configuration de livraison normale.
- **Radio, météo, cartes et données avions** : certaines requêtes utilisent HTTP en clair ; la radio refuse HTTPS. Migrer vers HTTPS validé lorsque la source le permet. Le réseau doit rester facultatif et non bloquant.
- **Sats** : la fonction refuse de démarrer sans clé N2YO, ce qui rend cette app dépendante d'un compte/API. Étudier un catalogue orbital public et la prédiction locale.
- **GNSS** : le code configure le module avec des commandes PCAS ; confirmer le modèle et les commandes sur le Cap réel.
- **Catalogue App Store** : un seul paquet Guide ROZA.
- **Inventaire d'écrans** : les 85 écrans annoncés ne sont pas recalculés ici à partir de toutes les déclarations ; automatiser ce contrôle.

## 7. Mises à jour, partitions et récupération

### État actuel

- Partition Flash : NVS, PHY et une seule partition application `factory`.
- Le paquetage produit une image fusionnée USB et une image application seule, avec manifeste SHA-256.
- Les contrôles actuels vérifient structure, cible ESP32-S3, taille et relecture du paquet ; ils ne vérifient pas une signature cryptographique de confiance.
- Aucun moteur OTA applicatif de production n'a été identifié dans `firmware/src/`.
- Le retour arrière de l'application n'est pas possible avec une seule partition factory.

### Architecture recommandée pour l'étape 3

1. Construire un manifeste versionné signé (Ed25519 ou ECDSA selon les bibliothèques validées), qui couvre modèle de carte, version, taille, hash SHA-256, partition minimale, version minimale du bootloader et URL/nom du paquet.
2. Intégrer uniquement la clé publique de vérification au firmware ; garder la clé privée de signature hors du dépôt, dans le système de publication protégé.
3. Vérifier signature **avant installation**, puis hash, taille, modèle et compatibilité.
4. Prévoir slots OTA A/B et `otadata`, contrôle du premier démarrage et confirmation explicite après autotests ; utiliser le rollback du bootloader ESP-IDF.
5. Prévoir l'installation via Wi-Fi optionnel et via microSD, sans dépendance internet permanente.
6. Sauvegarder les préférences NVS avant migration ; l'installation initiale d'une nouvelle table de partitions devra se faire par USB-C. La microSD ne doit jamais être formatée par le processus d'update.
7. Dimensionner les deux slots uniquement après compilation du firmware actuel et inspection de la table générée. Aucune capacité A/B n'est déclarée acquise à ce stade.
8. Conserver une méthode de récupération USB-C complète si OTA ou SD ne sont plus accessibles.

Ne pas activer de manière irréversible Secure Boot/eFuses au cours de cette migration sans procédure de provisionnement des clés, de restauration et de validation distincte.

## 8. Recommandation technique consolidée

Le meilleur chemin est une migration en lots contrôlés :

- **Socle** : commencer par un prototype propre pioarduino 61.04.00 / Arduino 4.0.0 / IDF 6.1.1 ; comparer immédiatement avec 3.3.13 / IDF 5.5.6 et la plateforme officielle 8.0.0 / 3.3.12 / 5.5.5. Retenir la base la plus récente qui compile et passe les tests de tous les services, pas seulement un exemple minimal.
- **M5Stack** : garder M5Unified/M5GFX 0.3.0 déjà présents, puis vérifier rotation, audio, écran, DMA et clavier ADV.
- **Périphériques** : mettre à niveau RadioLib/FastLED/ESP8266Audio séparément ; garder NimBLE/ArduinoJson tant qu'ils sont déjà à jour ; résoudre la provenance TinyGPSPlus ; valider ou remplacer LibSSH sans perdre SSH.
- **SD** : prouver d'abord exFAT sur carte de test avec SdFat 2.3.1 et le bus SPI partagé ; conserver l'accès aux données actuelles.
- **Services** : passer les flux HTTP en HTTPS, utiliser les API officielles documentées, cache local, quotas et dates de fraîcheur ; ne jamais imposer une clé API pour les fonctions qui disposent d'une source publique utilisable.
- **Apps** : préserver les 40 entrées et ajouter des options/écrans ou des sources modernes ; n'en retirer aucune.
- **Ressources** : déplacer les fichiers volumineux sur SD, ne pas les incorporer au binaire Flash sans justification mesurée.

## 9. Critères de sortie de l'étape 1

- [x] Dépôt, branche et commit courants identifiés.
- [x] Chaîne de compilation active et versions réellement épinglées relevées.
- [x] État CI courant et blocage de compilation identifiés.
- [x] 40 applications et 7 groupes vérifiés directement dans `globals.cpp`.
- [x] Incohérences du catalogue et de l'inventaire documentaire relevées.
- [x] Matériel Cardputer ADV / Cap LoRa-1262, broches et bus partagés rapprochés de la documentation M5Stack.
- [x] Candidats de versions stables vérifiés dans les dépôts amont officiels.
- [x] Budget/risques RAM, Flash, SD, OTA, TLS, Wi-Fi et services externes documentés.
- [x] Plan de migration applicatif établi, sans suppression de fonction.
- [ ] Compilation verte du commit courant : bloquée par `hotspot.cpp`, à corriger à l'étape 2.
- [ ] Mesures RAM/Flash du binaire courant : après réparation du build.
- [ ] Essais physiques Cardputer ADV + Cap + carte SD : à effectuer avec le matériel ; aucune simulation ne les remplace.

**Conclusion :** l'étape 1 est auditée et son livrable documentaire est prêt. Le firmware n'est pas encore compilable sur le commit examiné. L'étape suivante est exclusivement l'étape 2/5 : corriger le build puis porter le cœur et les applications par lots, sans démarrer la refonte VHS ni l'OTA avant leur tour.

## 10. Sources primaires consultées

### M5Stack et Espressif
- Cardputer ADV : https://docs.m5stack.com/en/core/Cardputer-Adv
- Cap LoRa-1262 : https://docs.m5stack.com/en/cap/Cap_LoRa-1262
- Arduino-ESP32 4.0.0 : https://github.com/espressif/arduino-esp32/releases/tag/4.0.0
- ESP-IDF OTA / rollback : https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html

### Framework et bibliothèques
- pioarduino 61.04.00 : https://github.com/pioarduino/platform-espressif32/releases/tag/61.04.00
- pioarduino 55.03.313 : https://github.com/pioarduino/platform-espressif32/releases/tag/55.03.313
- M5Unified : https://github.com/m5stack/M5Unified/releases
- M5GFX : https://github.com/m5stack/M5GFX/releases
- RadioLib : https://github.com/jgromes/RadioLib/releases
- FastLED : https://github.com/FastLED/FastLED/releases
- ESP8266Audio : https://github.com/earlephilhower/ESP8266Audio/releases
- NimBLE-Arduino : https://github.com/h2zero/NimBLE-Arduino/releases
- ArduinoJson : https://github.com/bblanchon/ArduinoJson/releases
- TinyGPSPlus : https://github.com/mikalhart/TinyGPSPlus/releases
- SdFat : https://github.com/greiman/SdFat/releases
- LibSSH-ESP32 : https://github.com/ewpa/LibSSH-ESP32
- wolfSSH : https://github.com/wolfSSL/wolfssh/releases

### Services / formats / API
- Open-Meteo : https://open-meteo.com/en/docs
- Météo-France via Open-Meteo : https://open-meteo.com/en/docs/meteofrance-api
- Air Quality API : https://open-meteo.com/en/docs/air-quality-api
- Frankfurter : https://frankfurter.dev/
- CelesTrak GP data : https://celestrak.org/NORAD/elements/gp.php
- ADSB.lol API : https://www.adsb.lol/docs/open-data/api
- Radio Browser API : https://api.radio-browser.info/
- GitHub REST rate limits : https://docs.github.com/en/rest/using-the-rest-api/rate-limits-for-the-rest-api
- OpenAI Responses API : https://developers.openai.com/api/docs/guides/text
- Protection des clés API OpenAI : https://help.openai.com/en/articles/8304786-keeping-your-openai-account-secure
