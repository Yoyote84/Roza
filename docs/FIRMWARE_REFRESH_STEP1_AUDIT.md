# РO3A — Audit de préparation à la modernisation du firmware (étape 1/5)

> Mise à jour du périmètre au 25 novembre 2026 : le module de jeu et ses composants dédiés ont été retirés de la branche `main` (commit `ec9a4ed2646527a652cf549f826a93ff8a30581f`). Ce document reste un audit historique de la base examinée le 24 novembre ; les mesures antérieures au retrait ne décrivent pas le firmware actuel.

**Date de l'audit :** 24 novembre 2026  
**Dépôt :** `Yoyote84/Roza`  
**Branche examinée :** `main`  
**Commit examiné :** `7d344a7956b7c8ffa633e83201e7a2457bb1becb`  
**Cible :** M5Stack Cardputer ADV + Cap LoRa-1262 (SX1262/GNSS)  
**Contraintes :** ESP32-S3FN8, 8 Mio de Flash, absence de PSRAM, microSD annoncée 128 Go.

> Cet audit est documentaire. Il ne modifie ni les dépendances ni le firmware, et ne vaut ni compilation ni essai sur le Cardputer physique. Les versions proposées sont des cibles candidates au 24/11/2026 ; elles devront être épinglées et validées par une compilation propre et des tests ciblés avant adoption définitive.

## 1. Conclusion exécutive

La base actuelle est fonctionnelle et très étendue, mais elle repose encore sur une ancienne génération Arduino-ESP32 : **Arduino 2.0.17 / ESP-IDF 4.4.7**, PlatformIO Espressif 7.0.1, M5Unified 0.2.20 et M5GFX 0.2.27. Les composants stables plus récents existent et plusieurs imposent des ruptures d'API. La mise à niveau doit donc être une migration contrôlée de la fondation, pas une simple modification de numéros de version.

Trois décisions techniques ressortent :

1. **Socle cible à prototyper :** Arduino-ESP32 4.0.0 / ESP-IDF 6.1.1, avec la plateforme PlatformIO communautaire pioarduino 61.04.00. La plateforme officielle PlatformIO 8.0.0 reste une option plus conservatrice, mais elle fournit Arduino 3.3.12 / ESP-IDF 5.5.5, pas Arduino 4.0.0. Le choix doit être confirmé par un build de référence avant de porter toutes les applications.
2. **OTA A/B :** le partitionnement actuel n'a qu'une partition applicative `factory`. Il ne peut ni installer une image alternative, ni faire un retour automatique à la précédente. Un premier flash USB-C complet sera nécessaire pour changer les partitions.
3. **Préserver le fonctionnement hors ligne :** conserver les applications générales, les pilotes GNSS/LoRa, le stockage et les outils réseau. Les modules explicitement retirés ne doivent pas être réintroduits lors de la modernisation.

**Périmètre de conservation :** toutes les applications générales et tous les pilotes ROZA restent préservés. Le module de jeu et ses composants dédiés ont ensuite été retirés explicitement du dépôt ; ce retrait ne constitue pas une autorisation de supprimer d'autres fonctions.

## 2. État réel du dépôt

### 2.1 Structure et chaîne de compilation

L'arbre GitHub contient 501 entrées suivies au commit examiné. Le firmware actif est exclusivement dans `firmware/` et utilise :

- PlatformIO, environnement `cardputer-adv` ;
- framework Arduino-ESP32 ;
- commande de référence : `cd firmware && pio run -e cardputer-adv` ;
- workflow CI : `.github/workflows/roza_esp32s3.yml`.

Les dossiers racine `main/`, `CMakeLists.txt`, `partitions.csv` et `sdkconfig.defaults` sont historiques et ne sont pas la fondation du firmware actif. Il ne faut pas migrer par erreur le mauvais projet.

Le workflow CI utilise actuellement **PlatformIO Core 6.1.6**. Il compile le firmware, exécute des suites de tests hôte (GNSS, LoRa, IR, JSON, batterie, RAM, horloge, calculatrice, astronomie et routeur/RID), vérifie les fichiers binaires et contrôle le paquet de flash. Il n'effectue pas d'essai sur carte. Pour le commit `7d344a7...`, aucun statut de compilation CI n'a été retourné par la vérification du statut GitHub dans cet audit : un succès sur le commit courant n'est donc pas confirmé.

### 2.2 Matériel réellement pris en charge

| Élément | Configuration présente dans le dépôt | Conséquence de migration |
|---|---|---|
| SoC | ESP32-S3FN8, double cœur, 8 Mio Flash, sans PSRAM | Budget Flash et RAM serré ; aucun calcul ne doit supposer de PSRAM |
| Écran | ST7789, 240×135, paysage | Garder le pilote BSP M5Stack ; revoir rotation et timings après M5GFX 0.3 |
| Clavier ADV | TCA8418, I²C 0x34, SDA 8 / SCL 9, IRQ 11 | Conserver le pilote ADV existant ; ne pas reprendre le scan GPIO du Cardputer original |
| Audio | Codec ES8311, ampli NS4150B, micro MEMS | Tester microphone, haut-parleur, I²S et transitions micro/haut-parleur |
| IMU | BMI270 via M5Unified | Tester détection et lectures réelles |
| LED | SK6812, données GPIO21 ; alimentation partagée avec le rétroéclairage GPIO38 | Vérifier le pilote FastLED et préserver la gestion du rétroéclairage |
| microSD | CS12, SCK40, MISO39, MOSI14 | SPI partagé avec le SX1262 ; vérifier le système de fichiers de la carte 128 Go avant montage/écriture |
| Cap LoRa-1262 | SX1262 ; NSS5, BUSY6, DIO1=4, RST3 ; SPI partagé avec la SD | Une seule gestionnaire de bus SPI partagé, sans initialiser/arrêter un bus concurrent |
| GNSS du Cap | ATGM336H documenté, UART1 RX15/TX13 à 115200 bauds ; expandeur I²C 0x43 | Conserver le buffer RX de 4096 octets configuré avant `begin()` ; vérifier la référence du module sur la documentation matérielle |
| USB-C | USB natif ESP32-S3, CDC au démarrage | Nécessaire pour le flash initial de migration et la récupération |

**Incohérence documentaire à corriger :** `firmware/HARDWARE.md` décrit l'ATGM336H, tandis qu'une ancienne section de la feuille de route mentionne AT6668. Le code GNSS et les commandes PCAS doivent être rapprochés de la référence exacte de la carte/du Cap fourni.

### 2.3 Applications et fonctions à préserver

Le fichier `firmware/docs/feature-inventory.md` annonce 40 applications / 85 écrans. Le menu actuellement déclaré dans `firmware/src/globals.cpp` contient 40 applications réparties en sept groupes ; cet état constitue la référence pour les prochains contrôles d'inventaire.

Le périmètre observé comprend notamment : horloge/calendrier, chronomètre/minuteur, IMU, gestionnaire de fichiers, lecteur de texte, spectre micro, calculatrice, convertisseur, lecteur WAV, BadApple, radio MP3, météo, astronomie, satellites, avions/ADS-B, séismes/typhons, FX, OKX, GitHub, ChatGPT, SSH, Wi-Fi, BLE, hotspot, outils réseau, IR, LoRa, GNSS/cartographie, wardrive, outils HID, Hash Oven et App Store.

L'audit de migration devra établir une liste exhaustive par écran et par fonctionnalité, puis utiliser cette liste comme test de non-régression. Les noms d'applications ou leur présentation pourront être harmonisés lors de la nouvelle interface, mais pas au prix de retirer une fonction existante.

### 2.4 Audio, vidéo et ressources volumineuses

- Le lecteur WAV lit du PCM 16 bits depuis la SD et utilise `M5.Speaker.playRaw()`.
- La radio utilise ESP8266Audio pour décoder le MP3, avec une sortie adaptée vers `M5.Speaker`. Elle ne doit pas être confondue avec le lecteur WAV.
- BadApple lit `/video/badapple.dat`, un format binaire propre au projet en 240×135, avec une image monochrome 1 bit par pixel. Ce n'est pas un lecteur MP4 généraliste.
- Les anciennes ressources de jeu et de narration locale ont été retirées du firmware actif. Le code applicatif ne doit plus embarquer de poids de modèle, de tokenizer ni de runtime associé.


## 3. Inventaire actuel et cibles stables candidates

Les liens ci-dessous sont des sources amont à la date de l'audit. Une version plus récente n'est pas automatiquement une version compatible : chaque adoption doit être épinglée et testée.

| Composant | Version actuelle dans Roza | Cible stable candidate au 24/11/2026 | Décision d'audit et risques |
|---|---:|---:|---|
| PlatformIO Core | 6.1.6 dans la CI | 6.2.0 | Mettre à niveau l'outil de build ; vérifier la résolution identique en local et en CI. Python 3.9+ requis par cette version. |
| Plateforme Espressif PlatformIO | espressif32 7.0.1 | pioarduino 61.04.00 pour Arduino 4.0.0 ; comparer aussi l'officielle espressif32 8.0.0 | Le dépôt officiel 8.0.0 utilise Arduino 3.3.12 / IDF 5.5.5. La plateforme pioarduino 61.04.00 annonce Arduino 4.0.0 / IDF 6.1.1, mais c'est une plateforme communautaire : épinglage et reproductibilité à contrôler. |
| Arduino-ESP32 / ESP-IDF | 2.0.17 / 4.4.7 | Arduino 4.0.0 / ESP-IDF 6.1.1 | Migration majeure : APIs ESP-IDF supprimées/modifiées, drivers, Wi-Fi, Bluetooth, TLS, SPI/I²C, audio et fonctions FreeRTOS à recompiler et adapter. Ne pas porter tout le projet avant un test minimal du socle. |
| M5Unified | 0.2.20 | 0.3.0 | À mettre à niveau en même temps que M5GFX 0.3.0. Plusieurs fonctions changent de retour `void` à `bool`. |
| M5GFX | 0.2.27 | 0.3.0 | Rupture de rotation : sur Cardputer/Cardputer ADV, l'ancien `setRotation(1)` devient `setRotation(0)` (ou l'appel peut être supprimé si la rotation de démarrage convient). Contrôler également la fluidité et le rendu BadApple. |
| RadioLib | 7.7.1 | 8.0.0 | Rupture d'API confirmée : méthodes `begin` à arguments séparés supprimées, paramètres de fréquence/bande passés en entiers, TCXO enum, callbacks IRQ harmonisés. Reprise du pilote SX1262 nécessaire. |
| FastLED | 3.10.3 | 3.10.10 | Les régressions de taille observées en 3.10.5 ont été réduites dans les versions ultérieures, notamment 3.10.7. Ne pas reprendre l'ancienne interdiction comme vérité actuelle : mesurer Flash/RAM, vérifier RMT GPIO21 et tester avec audio/Wi-Fi actifs. |
| ESP8266Audio | 1.9.9 | 2.4.3 | La pile actuelle était épinglée pour IDF4. La cible IDF6 lève le motif historique de blocage, mais le code de sortie `RadioOut` et le correctif local `SafeMP3` doivent être recompilés et testés avant toute suppression. |
| ArduinoJson | 7.4.3 | 7.4.3 | Dernière version stable publiée identifiée dans le dépôt amont ; conserver la version exacte si les tests passent. |
| TinyGPSPlus | 1.1.0 (paquet PlatformIO) | 1.1.0 provisoirement | Le dépôt amont GitHub affiche le tag 1.0.3a comme dernière release ; l'écart de numérotation avec le paquet PlatformIO doit être expliqué par la provenance du paquet avant tout changement. Pas de mise à jour supposée. |
| LibSSH-ESP32 | 5.9.0 | Non déterminée | Aucune release amont récente fiable n'a été confirmée pendant cet audit. C'est un point bloquant à tester sous IDF6 ; si la bibliothèque est abandonnée ou incompatible, comparer un remplacement maintenu plutôt que garder une dépendance obsolète par facilité. |

### Décision sur le socle

La cible prioritaire à éprouver en étape 2 est **pioarduino 61.04.00 + Arduino-ESP32 4.0.0 + ESP-IDF 6.1.1**, car elle correspond à la génération Arduino stable la plus récente observée. Ce n'est pas encore une décision de production : la plateforme est communautaire et les bibliothèques tierces, en particulier LibSSH-ESP32 et ESP8266Audio, peuvent bloquer la migration.

Le test initial doit comparer un environnement PlatformIO propre, épinglé, qui compile la carte et initialise M5Unified, sans encore modifier les 80+ unités de compilation. Si cette plateforme n'est pas reproductible ou présente un blocage amont non résolu, l'alternative officielle espressif32 8.0.0 / Arduino 3.3.12 / IDF5.5.5 devra être évaluée explicitement. Il ne faut pas confondre « dernière version officielle de la plateforme PlatformIO » et « dernière version du framework Arduino ».

## 4. Partition Flash et faisabilité de l'OTA A/B

### 4.1 État présent

La table active `firmware/partitions_8MB.csv` contient NVS, PHY et une seule application :

```csv
nvs,      data, nvs,     0x9000,   0x5000,
phy_init, data, phy,     0xe000,   0x1000,
factory,  app,  factory, 0x10000,  0x7E0000,
```

Le paquet `ROZA_UPDATE.bin` est une image applicative à écrire par USB à `0x10000`. Ce n'est **pas** une mise à jour OTA : elle nécessite toujours le mode téléchargement USB et ne dispose pas d'un ancien slot applicatif pour revenir en arrière.

Le paquetage actuel produit des empreintes SHA-256 et vérifie la structure, l'en-tête ESP32-S3, les limites de partition et les octets écrits dans le paquet. Ces contrôles protègent contre un fichier tronqué ou mal empaqueté, mais **ne prouvent pas l'identité de l'émetteur** si l'image et son manifeste sont remplacés ensemble. Il n'existe pas de moteur OTA applicatif trouvé dans `firmware/src/`.

### 4.2 Estimation de taille pour une architecture A/B

La dernière taille documentée dans `firmware/UPSTREAMS.md` est un binaire application de **3 680 448 octets**, issu du workflow CI #39 du 4 novembre 2026 sur un ancien commit. Ce chiffre n'est pas une mesure du commit actuel.

Les anciennes mesures de taille citées dans cet audit précèdent le retrait des ressources de jeu et ne doivent pas servir de référence au firmware actuel. La taille de l'application doit être remesurée sur un build complet du commit courant.

Une disposition candidate à vérifier pourrait réserver :
- deux slots applicatifs OTA de `0x340000` octets chacun (3 407 872 octets par slot) ;
- aucune partition Flash dédiée à un jeu ou à un modèle de narration n'est désormais nécessaire ; la table OTA candidate doit être redimensionnée à partir de la taille réelle du firmware actuel.
- NVS, PHY et `otadata`, avec alignement et zones de boot réservées.

Sur ces seules valeurs historiques, le code aurait environ 1 008 194 octets de marge par slot. **Ce calcul ne valide pas le partitionnement** : la nouvelle fondation peut modifier fortement la taille, les données et les alignements. Une compilation réelle et la génération/inspection de la table seront indispensables.

Le dimensionnement OTA doit désormais être fondé sur le binaire actuel, les partitions nécessaires au démarrage et les marges de sécurité. Aucune hypothèse de stockage d'un modèle de narration embarqué ne doit être réintroduite.

### 4.3 Premier flash de migration

La table actuelle ne peut pas être transformée en A/B par une simple OTA : le bootloader en place et le partitionnement ne prévoient pas les deux slots. Le premier passage vers la nouvelle table devra se faire par une **image complète via USB-C**, avec sauvegarde préalable des réglages NVS (réseaux Wi-Fi enregistrés, préférences, fuseau, paramètres locaux). La documentation de flash actuelle précise qu'une image fusionnée écrite depuis 0x0000 réinitialise la NVS. La microSD est une mémoire distincte, mais aucune opération ne doit la formater ou la nettoyer.

Les futures mises à jour devront vérifier une signature numérique du manifeste avec une clé publique intégrée, puis vérifier SHA-256, modèle exact de carte, version minimale du bootloader/partitionnement, taille et version du firmware. SHA-256 seul n'est pas une signature. La validation de démarrage et le rollback devront utiliser le mécanisme OTA ESP-IDF/Arduino compatible avec le nouveau socle. L'activation irréversible de Secure Boot/eFuses ne doit pas être faite dans cette migration sans plan de clés, récupération et décision distincte.

## 5. Réseau, microSD et comportement hors ligne

### Wi-Fi

Les règles `AGENTS.md` disent que Wi-Fi et Bluetooth doivent rester inactifs par défaut, mais le code actuel lance `bootWifiStart()` et `wifiKeeperUpdate()` : il tente automatiquement de se connecter à un réseau enregistré et de maintenir la connexion. Il existe donc une incohérence entre règle documentaire et comportement logiciel.

Pour la nouvelle base, le comportement cible est :
- usage de ROZA pleinement disponible sans Internet ;
- recherche de mise à jour manuelle depuis l'interface ;
- connexion uniquement à un réseau Wi-Fi déjà enregistré ;
- mise à jour automatique désactivée par défaut et activable explicitement dans les paramètres ;
- échec réseau non bloquant, sans empêcher le démarrage ni les applications hors ligne ;
- HTTPS avec validation TLS, sans `setInsecure()`.

### microSD

Le pilote actuel appelle `SD.begin(12, SPI, 20000000)` et monte la carte en lecture/écriture. Il n'existe pas de garantie de montage réellement protégé en lecture seule. Le guide de flash avertit également que le pilote courant ne garantit pas la prise en charge exFAT des cartes SDXC 128 Go.

Avant d'ajouter l'installation firmware depuis SD, il faudra identifier le système de fichiers réellement présent, confirmer la prise en charge de FAT32/exFAT selon la solution retenue, définir une lecture de paquet non destructive et tester le partage SPI avec le SX1262. La carte personnelle ne doit pas être formatée ni utilisée pour un test d'écriture sans sauvegarde et validation explicite. Les premiers essais de paquet SD doivent utiliser une carte de test.

### Contenu des paquets

Le magasin `firmware/data/apps/catalog.json` installe aujourd'hui des mini-applications déclaratives JSON sur la SD ; elles ne contiennent pas de code natif et n'installent pas un firmware. Le nouvel installateur doit séparer clairement :
- applications déclaratives ;
- ressources volumineuses (cartes, musique, vidéo et données) ;
- paquets firmware signés destinés au slot OTA inactif.

## 6. Risques de migration à traiter

| Priorité | Risque | Mesure nécessaire avant de fermer le point |
|---|---|---|
| Critique | Nouveau core IDF6 casse des API de réseau, TLS, Bluetooth, USB, audio ou FreeRTOS | Inventaire des callsites et build isolé, puis migration par lots |
| Critique | Capacité réelle des deux slots A/B OTA non démontrée sur le firmware actuel | Nouvelle compilation, mesure du binaire et vérification de la table de partitions avant d'activer l'OTA |
| Critique | Mauvaise table de partitions / flash initiale | Paquet fusionné contrôlé, sauvegarde NVS et procédure USB-C de récupération |
| Élevée | LibSSH-ESP32 5.9.0 sans nouvelle release fiable | Prouver la compilation et le SSH réel, ou choisir un remplacement maintenu |
| Élevée | API RadioLib 8.0 rompue | Porter le pilote SX1262, vérifier réception, IRQ, TCXO, réglages, et laisser TX désactivé |
| Élevée | M5GFX change la rotation et peut changer les timings | Vérifier rotation 240×135, rafraîchissement, clavier et écran sur ADV |
| Élevée | Bus SPI SD/SX1262 et I²C interne partagés | Gestionnaires existants conservés ; tests de coexistence et de sortie d'applications |
| Élevée | microSD 128 Go en exFAT potentiellement non supportée | Identifier le format réel ; aucun montage destructif ni formatage |
| Élevée | Wi-Fi se connecte au démarrage contrairement à la règle hors ligne | Aligner politique et code avant la mise à jour automatique |
| Moyenne | Inventaire applicatif divergent (40 annoncées contre 39 totalisées par groupes) | Refaire le catalogue depuis les points d'entrée de `main.cpp` |
| Moyenne | FastLED et audio peuvent consommer ressources/temps CPU | Mesurer Flash/RAM, RMT, latence et cohabitation audio |
| Moyenne | Modifications TLS/certificats sous IDF6 | Tester HTTPS public, certificats, erreurs de date, reprise et absence de clé API |

## 7. Plan de validation pour la suite

### Sortie attendue de l'étape 1 — audit
- [x] Dépôt, branche et commit examinés.
- [x] Chaîne de build et plateforme réelle identifiées.
- [x] Dépendances actuelles et releases stables candidates relevées.
- [x] Matériel, bus partagés, contraintes RAM/Flash documentés.
- [x] Inventaire fonctionnel existant localisé et écart de comptage signalé.
- [x] Absence de double slot OTA et absence d'un moteur OTA de production confirmées dans le code inspecté.
- [x] Risques microSD, Wi-Fi, TLS et migration de ressources recensés.
- [ ] Prototype technique OTA A/B et modèle mappé (étape 2/3, pas dans cet audit).
- [ ] Compilation sur la nouvelle base (étape 2).
- [ ] Tests physiques Cardputer ADV + Cap LoRa-1262 (étape 5).

### Porte de passage vers l'étape 2

L'étape 2 devra démarrer sur une branche de travail autorisée par les règles du dépôt (la politique actuelle n'autorise que `main`, sans création de branche implicite) et commencer par un build minimal reproductible de la plateforme candidate. Puis les modules seront portés et mesurés sans supprimer de fonctionnalité. Les versions finales devront être verrouillées dans `platformio.ini` et dans la CI.

### Les cinq étapes du projet

1. **Audit et préparation** — ce document.
2. **Nouvelle fondation système** — migration du core, BSP et bibliothèques ; adaptation des APIs et préservation de toutes les fonctions.
3. **Moteur de mise à jour** — paquets signés, installation Wi-Fi/SD, A/B OTA, vérification au démarrage et rollback.
4. **Nouvelle interface VHS/CRT** — identité visuelle originale analogique années 70/80, effets mesurés adaptés au 240×135 et au budget mémoire.
5. **Intégration finale** — compilation complète, tests logiciels et CI, vérification du paquet, puis essai du firmware par l'utilisateur sur le Cardputer ADV et le Cap LoRa-1262.

## 8. Sources techniques principales

### Dépôt Roza
- [Configuration PlatformIO active](../firmware/platformio.ini)
- [Référence matérielle Cardputer ADV](../firmware/HARDWARE.md)
- [Versions de dépendances et anciens benchmarks](../firmware/docs/dependency-pins.md)
- [Inventaire applicatif](../firmware/docs/feature-inventory.md)
- [Audit mémoire](../firmware/docs/memory-audit.md)
- [Sécurité du paquet de flash](../firmware/docs/UPDATE_SAFETY.md)
- [Guide de flash et limites du partitionnement](../firmware/docs/FLASHING.md)
- [Provenance de la base et mesures CI historiques](../firmware/UPSTREAMS.md)
- [Workflow firmware](../.github/workflows/roza_esp32s3.yml)

### Releases amont examinées au 24/11/2026
- [Arduino-ESP32 4.0.0 / notes Espressif](https://github.com/espressif/arduino-esp32/releases/tag/4.0.0) et [notes de version officielles](https://release-notes.espressif.com/?project=Arduino)
- [pioarduino 61.04.00 — Arduino 4.0.0 / ESP-IDF 6.1.1](https://github.com/pioarduino/platform-espressif32/releases/tag/61.04.00)
- [PlatformIO espressif32 8.0.0](https://github.com/platformio/platform-espressif32/releases/tag/v8.0.0)
- [PlatformIO Core 6.2.0](https://github.com/platformio/platformio-core/releases/tag/v6.2.0)
- [M5Unified 0.3.0](https://github.com/m5stack/M5Unified/releases/tag/0.3.0)
- [M5GFX 0.3.0](https://github.com/m5stack/M5GFX/releases/tag/0.3.0)
- [RadioLib 8.0.0](https://github.com/jgromes/RadioLib/releases/tag/8.0.0)
- [FastLED 3.10.10](https://github.com/FastLED/FastLED/releases/tag/3.10.10)
- [FastLED 3.10.7 — correction de régressions mémoire](https://github.com/FastLED/FastLED/releases/tag/3.10.7)
- [ESP8266Audio 2.4.3](https://github.com/earlephilhower/ESP8266Audio/releases/tag/2.4.3)
- [ArduinoJson 7.4.3](https://github.com/bblanchon/ArduinoJson/releases/tag/v7.4.3)
- [Releases TinyGPSPlus](https://github.com/mikalhart/TinyGPSPlus/releases)

**État final : étape 1/5 — audit documentaire terminé.** Aucun code, aucune dépendance, aucune partition et aucune interface du firmware n'ont été modifiés pendant cet audit.
