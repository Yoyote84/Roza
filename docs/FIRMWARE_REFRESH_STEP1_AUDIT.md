# РO3A — Audit de référence de la grande mise à jour VHS (étape 1/5)

**Date de l'audit :** 25 novembre 2026  
**Dépôt :** https://github.com/Yoyote84/Roza  
**Branche examinée :** \`main\` uniquement  
**Commit de référence :** \`71cd1ff0216ec426f8d073153c838373181da5b9\`  
**Matériel cible :** M5Stack Cardputer ADV + Cap LoRa-1262  
**Périmètre :** firmware actif de \`firmware/\`, ses applications, ses dépendances, ses services et sa chaîne de livraison.

> Ce dossier est l'état de référence documentaire de l'étape 1. Il distingue les faits observés dans le dépôt, les informations amont officielles, les recommandations de migration et les validations qui ne peuvent être réalisées qu'aux étapes suivantes. Il ne prétend ni que le firmware compile actuellement, ni que les périphériques ont été testés sur la carte physique.

---

## 1. Décision et résultat de l'étape 1

L'étape 1 est **complète sur son périmètre d'audit et de préparation**. Le dépôt, le commit, la chaîne de compilation, le catalogue réel, les dépendances, les possibilités de remplacement, les contraintes du matériel, les risques de mémoire, les problèmes réseau, le partitionnement et les priorités de migration ont été relevés et consignés.

La modernisation doit conserver les **40 entrées du menu actuel** et leur rôle. Aucune application ne doit être retirée pour simplifier une mise à niveau. Une technologie peut être remplacée lorsque la nouvelle solution apporte un gain démontré et reprend les fonctions utiles, les formats de données et les cas d'usage existants.

La cible proposée est un socle Arduino-ESP32 moderne, sans basculer le firmware actif vers un projet ESP-IDF natif. La version actuelle du dépôt est déjà plus récente que les anciens audits : PlatformIO Espressif32 8.0.0, Arduino-ESP32 3.3.12 / ESP-IDF 5.5.5, M5Unified 0.3.0 et M5GFX 0.3.0. Les versions 4.0.0 / IDF 6.1.1 et les bibliothèques les plus récentes doivent désormais être évaluées comme migrations depuis cette base, et non comme un premier portage depuis Arduino 2.0.17.

**À ne pas confondre :** la CI actuelle échoue encore sur une erreur de compilation dans Wardrive. Cette correction appartient à l'étape 2/5. Elle n'annule pas l'audit documentaire de l'étape 1, mais interdit de déclarer le firmware compilable ou de mesurer son binaire actuel.

## 2. État réel du dépôt au commit de référence

### 2.1 Dépôt et règles de travail

- Le firmware actif est exclusivement dans \`firmware/\`.
- La compilation de référence est PlatformIO, environnement \`cardputer-adv\`.
- La commande de référence est \`cd firmware && pio run -e cardputer-adv\`.
- Les fichiers racine \`CMakeLists.txt\`, \`main/\`, \`partitions.csv\` et \`sdkconfig.defaults\` sont historiques. Ils ne définissent pas la compilation active.
- Le projet actif utilise la branche \`main\`. Les branches de travail et les pull requests non intégrées ne constituent pas l'état livré.
- Deux pull requests restent ouvertes au moment de l'audit : la PR #3 sur le découplage du jeu séparé et la PR #1 de test de mise à jour 105. Elles ne sont pas considérées comme intégrées à \`main\` et ne doivent pas être fusionnées sans examen de leur différence avec l'état courant.
- Le test CI de nettoyage des anciens composants du jeu retiré passe sur le commit examiné. Aucun fichier ou symbole Hawkins n'a été observé dans le menu actif de \`globals.cpp\`. Le jeu Hawkins ne fait donc pas partie du périmètre de ROZA à reconstruire ou à réintroduire.

### 2.2 Chaîne de compilation active

| Élément | État réel dans le dépôt |
|---|---|
| Plateforme PlatformIO | \`espressif32@8.0.0\` |
| Framework | Arduino-ESP32 3.3.12 |
| Base ESP-IDF du framework | 5.5.5 |
| PlatformIO Core en CI | 6.2.0 |
| Carte PlatformIO | \`esp32-s3-devkitc-1\`, configurée pour 8 Mo |
| Environnement | \`cardputer-adv\` |
| Table active | \`firmware/partitions_8MB.csv\` |
| Workflow CI | \`.github/workflows/roza_esp32s3.yml\` |

Le dépôt n'est pas un firmware ESP-IDF natif. Une éventuelle adoption d'un niveau ESP-IDF plus récent doit être effectuée au travers d'une plateforme PlatformIO compatible avec Arduino-ESP32, puis vérifiée sur le projet complet.

### 2.3 Résultat CI exact

Exécution associée au commit de référence :

- Workflow : ROZA Web Screen Viewer + Cardputer ADV firmware
- Run : https://github.com/Yoyote84/Roza/actions/runs/45518114744
- Job de compilation : https://github.com/Yoyote84/Roza/actions/runs/45518114744/job/138882744067
- Conclusion : échec.
- Étape fautive : compilation de \`src/wardrive.cpp\`.
- Erreur : \`STA_STARTED_BIT was not declared in this scope\`, à l'appel \`WiFiGenericClass::waitStatusBits(STA_STARTED_BIT, 300)\`.
- La production des images de flash, la génération de l'artefact Web et les étapes qui suivent la compilation ont été ignorées.

Le correctif du point d'accès dans \`hotspot.cpp\` n'est plus l'erreur bloquante de ce commit. Il utilise déjà l'API publique ESP-NETIF \`esp_netif_dhcps_get_clients_by_mac\`. Les anciens rapports qui désignent encore \`hotspot.cpp\` comme erreur courante sont obsolètes.

Avertissements secondaires observés dans la même sortie :
- \`skyview.cpp\` : possible troncature du texte formaté dans un tampon de six octets ;
- \`typhoon.cpp\` : référence potentiellement pendante vers un \`JsonArray\` temporaire ;
- \`wardrive.cpp\` : format \`%u\` utilisé avec un \`uint32_t\` qui correspond à \`%lu\` sur cette cible.

La CI actuelle exécute aussi les suites IR, LoRa, Drone ID, GNSS, JSON, Hash Oven, batterie, RAM, Astro, calculatrice, horloge, tests routeur/RID, balayage mémoire, test statique NimBLE, tests de paquetage et création des images. Le test de nettoyage des anciens composants Hawkins est réussi sur ce commit. La compilation échouant avant l'édition de liens, aucun nouveau rapport RAM/Flash ni binaire complet n'est disponible pour le SHA de référence.

**Conséquence pour la suite :** réparer l'API Wi-Fi privée dans Wardrive, traiter les avertissements qui révèlent un risque réel, compiler et mesurer le binaire relèvent de l'étape 2/5. Ne pas masquer cet échec en modifiant la documentation ou en sautant le contrôle CI.

### 2.4 Inventaire exact du menu

Le tableau \`APPS[]\` de \`firmware/src/globals.cpp\` contient **40 entrées**, réparties en **7 groupes**. Les contrôles statiques du même fichier vérifient que les plages des groupes se suivent, couvrent toutes les entrées et ne dépassent pas quatre colonnes sur deux lignes.

| Groupe | Entrées visibles dans le menu | Nombre |
|---|---|---:|
| TOOL | Time, IMU, Files, Spectrum, Calc, Converter, Player, Hash Oven | 8 |
| SCAN | WiFi Chan, Sniffer, Wardrive, Drone ID, BLE Scan | 5 |
| NET | Hotspot, LAN Scan, NetProbe, Router, SSH, ChatGPT | 6 |
| HID | BT Keys, BT Media, Ducky | 3 |
| SIG | LoRa, GNSS, Map, IR | 4 |
| SKY | Astro, Weather, Planes, Sats, Typhoon, Quake | 6 |
| MORE | GitHub, Radio, FX, OKX, Reader, BadApple, Apps, Settings | 8 |
| **Total** | **40 entrées / 7 groupes** | **40** |

Le fichier historique \`firmware/docs/feature-inventory.md\` décrit encore une organisation amont différente et annonce 34 applications. Il ne doit plus servir de référence au nombre d'entrées du menu ROZA. La table \`APPS[]\` de \`globals.cpp\` est la source de vérité pour le menu courant. Les écrans et sous-modes d'une entrée ne doivent pas être comptés comme des applications supplémentaires.

Le menu Paramètres contient actuellement 15 choix : Wi-Fi, PC MODE, GNSS Config, Brightness, Volume, Boot sound, LED, Theme, Screen off, Date/heure, Units, Battery, Debug bar, Format SD et About.

## 3. Matériel et compatibilité

### 3.1 M5Stack Cardputer ADV

La documentation officielle décrit notamment :
- SoC ESP32-S3FN8 ;
- Flash 8 Mo ;
- écran ST7789V2 de 240 × 135 pixels ;
- clavier 56 touches, matrice 4 × 14 pilotée par TCA8418 ;
- IMU BMI270 ;
- codec audio ES8311, amplificateur NS4150B et haut-parleur 1 W ;
- microphone MEMS ;
- émetteur infrarouge ;
- lecteur microSD ;
- batterie annoncée à 1 750 mAh.

La configuration ROZA considère la carte sans PSRAM. Aucune fonctionnalité de la modernisation ne doit supposer une PSRAM disponible. L'écran, le clavier ADV, l'IMU, l'audio, la batterie et la microSD doivent continuer à utiliser les pilotes ou le BSP déjà présents tant qu'un défaut démontré ne justifie pas une extension.

Broches importantes du Cardputer ADV, recoupées avec le code :
- I²C interne : GPIO8 SDA et GPIO9 SCL ;
- clavier TCA8418 : adresse I²C 0x34 et interruption GPIO11 ;
- IMU BMI270 : bus I²C interne ;
- écran : GPIO33 reset, GPIO34 RS/DC, GPIO35 données, GPIO36 horloge, GPIO37 CS, GPIO38 rétroéclairage / alimentation LED ;
- microSD : GPIO12 CS, GPIO14 MOSI, GPIO40 SCK, GPIO39 MISO ;
- infrarouge : GPIO44 ;
- batterie : mesure ADC sur GPIO10.

### 3.2 Cap LoRa-1262 et GNSS

La documentation officielle M5Stack du Cap LoRa-1262 confirme :
- transceiver Semtech SX1262 ;
- bande matérielle indiquée 868–923 MHz ;
- GPS ATGM336H-6N / AT6668 ;
- bus SPI partagé avec la microSD ;
- expandeur d'E/S PI4IOE5V6408 ;
- interrupteur RF d'antenne à activer par P0 de l'expandeur ;
- GNSS sur UART : GPIO13 côté émission du Cardputer vers GPS-RX et GPIO15 côté réception depuis GPS-TX ;
- LoRa : NSS GPIO5, RESET GPIO3, IRQ GPIO4, BUSY GPIO6, SCK GPIO40, MOSI GPIO14 et MISO GPIO39 ;
- I²C du Cap sur GPIO8/GPIO9, adresse d'expandeur utilisée par le code : 0x43.

Le code actuel de \`lora.cpp\` réutilise le même objet SPI global que \`sd_files.cpp\`. Cette réutilisation est nécessaire : la microSD et le SX1262 partagent physiquement SCK/MISO/MOSI et ont deux lignes de sélection distinctes (SD GPIO12, LoRa GPIO5). La coexistence simultanée reste un essai matériel à réaliser ; une déclaration de partage dans le code ne prouve pas à elle seule l'absence de collision de transactions, d'accès concurrents ou de sélection CS incorrecte.

Le firmware doit conserver l'écoute LoRa comme comportement par défaut. Aucune émission automatique ne doit être introduite. Les profils hors bande ou incompatibles avec l'antenne du Cap ne doivent pas être présentés comme des profils d'émission utilisables. Toute émission doit être explicitement demandée, avec région, fréquence, puissance, duty-cycle, antenne et réglementation France/UE vérifiés.

### 3.3 microSD de 128 Go : risque matériel et logiciel à traiter

L'identification du système de fichiers de la carte réelle n'est pas consignée. Une carte de 128 Go ne doit pas être supposée FAT32 ; une carte SDXC peut notamment être formatée en exFAT.

Constats dans le code actif :
- \`sdInit()\` appelle \`SPI.begin(40, 39, 14, 12)\`, puis \`SD.begin(12, SPI, 20000000)\` ;
- le montage Arduino SD est en lecture/écriture ; le code ne prouve pas l'existence d'un montage protégé contre les écritures ;
- \`sdInit()\` est appelé au démarrage dans le firmware actuel, alors que la règle du dépôt demande de ne pas monter automatiquement la carte de 128 Go avant identification de son format ;
- \`formatSD()\` ne recrée pas un système de fichiers : elle supprime récursivement les fichiers et dossiers de la carte via \`SD.remove\` et \`SD.rmdir\`. C'est une opération destructive malgré son nom affiché ;
- le partage SPI avec le SX1262 doit être testé avec une carte de test et des procédures non destructives.

Décision de préparation : ne pas effectuer de test d'écriture, de suppression ou de formatage sur la carte utilisateur. Étudier une détection non destructive du type de partition et du système de fichiers, puis un pilote assurant la prise en charge du format constaté. SdFat 2.3.1 est un candidat à évaluer pour FAT/exFAT et SPI, mais il ne doit pas remplacer Arduino SD sans test de lecture des données existantes, mesures mémoire et contrôle du bus partagé. Si aucun montage réellement sûr ne peut être garanti, laisser le stockage désactivé plutôt que d'afficher une protection fictive.

## 4. Budget de mémoire et de performance

Les contraintes structurantes sont les suivantes :

| Élément | Donnée ou conséquence |
|---|---|
| Flash | 8 Mo au total |
| PSRAM | aucune supposée par la configuration ROZA |
| Écran | 240 × 135 pixels |
| Canvas RGB565 plein écran | 240 × 135 × 2 = 64 800 octets, soit environ 63,3 Kio |
| Allocation du canvas | alignement spécifique dans \`globals.cpp\` pour éviter la fragmentation du heap |
| Cache de tuile cartographique | environ 48 Kio documentés dans le plan des dépendances |
| Bundle de certificats racines | 121 racines, environ 56 Kio compilés en Flash selon la configuration actuelle |
| TLS | allocations temporaires importantes et besoin de blocs contigus ; les accès HTTPS lourds libèrent temporairement le canvas |
| Radio / audio | buffers et tâches concurrentes ; la lecture radio libère le canvas afin de réserver de la RAM au décodeur et au flux |

Les mesures historiques de RAM/Flash dans \`firmware/docs/dependency-pins.md\` ont été obtenues sur une base Arduino 2.0.17 / ESP-IDF 4.4.7. Elles ne sont pas des mesures de la base actuelle Arduino 3.3.12 / IDF 5.5.5 et encore moins une mesure de la branche 4.0.0. Elles doivent être conservées comme archives comparatives, pas comme budget courant.

La CI du commit de référence échoue avant l'édition de liens. Il n'existe donc pas de mesure fiable du binaire lié courant pour ce SHA. À l'étape 2, relever au minimum :
- RAM statique et Flash à l'édition de liens ;
- heap libre, plus grand bloc contigu et minimum historique au démarrage ;
- mêmes mesures pendant TLS, GNSS/cartographie, BLE, audio/lecture radio et lecture/écriture SD ;
- temps de rendu et fluidité BadApple après mise à jour de M5GFX ;
- consommation en veille et lors des fonctions réseau/radio sur matériel réel à l'étape 5.

Aucun chiffre d'un ancien build ne doit être réutilisé comme mesure du nouveau binaire.

## 5. Versions amont : état réel et candidats stables

Les versions ci-dessous ont été recoupées dans les dépôts de publication officiels au 25 novembre 2026. « Candidat » signifie qu'une version stable existe en amont ; cela ne signifie pas qu'elle compile déjà avec ROZA.

| Composant | Version ROZA actuelle | Dernière version stable vérifiée | Décision de préparation |
|---|---:|---:|---|
| PlatformIO Core | 6.2.0 | 6.2.0 | Garder ; version cohérente avec la CI |
| Plateforme PlatformIO Espressif32 | 8.0.0 | 8.0.0 dans la base active ; branche communautaire pioarduino 61.04.00 disponible | Comparer les chaînes sans confondre la plateforme officielle et le fork pioarduino |
| Arduino-ESP32 | 3.3.12 | 4.0.0 | Cible majeure à évaluer |
| ESP-IDF sous Arduino | 5.5.5 | 6.1.1 | Suit le core Arduino sélectionné ; vérifier le guide de migration |
| M5Unified | 0.3.0 | 0.3.0 | Déjà à jour ; ne pas remplacer |
| M5GFX | 0.3.0 | 0.3.0 | Déjà à jour ; ne pas remplacer |
| NimBLE-Arduino | 2.5.2 | 2.5.2 | Déjà à jour ; conserver les tests BLE |
| RadioLib | 7.7.1 | 8.0.0 | Mise à niveau utile mais API majeure incompatible |
| FastLED | 3.10.3 | 3.10.10 | Candidat ; mesurer Flash/RAM et tester le pilote RMT LED |
| ArduinoJson | 7.4.3 | 7.4.3 | Déjà à jour ; conserver les correctifs et les tests JSON |
| TinyGPSPlus | 1.1.0 via la dépendance PlatformIO | dernier tag public GitHub observé v1.0.3a | Clarifier la provenance/version réellement résolue ; garder le parseur tant que couverture NMEA non équivalente |
| ESP8266Audio | tag Git 1.9.9 | 2.4.3 | Mise à niveau prioritaire à tester pour I²S, AAC, WAV, MIDI et flux |
| LibSSH-ESP32 | 5.9.0 | pas de version GitHub Release publiée | Compatibilité du port à confirmer ; son README ne cite que les cores Arduino 2.0.17 et 3.0.7 comme testés |
| wolfSSH | absent | 1.6.0 stable | Alternative à étudier pour SSH ; ce n'est pas un remplacement direct de LibSSH |
| SdFat | absent | 2.3.1 | Alternative candidate pour FAT/exFAT, sous réserve de tests Cardputer et bus SPI |

### 5.1 Cœur système : cible recommandée

Deux cibles amont plus récentes existent :
- Arduino-ESP32 3.3.13 / ESP-IDF 5.5.6, distribués par la plateforme communautaire pioarduino 55.03.313 ; c'est le saut incrémental qui permet d'isoler la migration de core ;
- Arduino-ESP32 4.0.0 / ESP-IDF 6.1.1, distribués par pioarduino 61.04.00 ; c'est la cible majeure la plus récente vérifiée.

Recommandation : conserver la base 8.0.0 comme point de comparaison reproductible, puis tester d'abord le saut incrémental 3.3.13/5.5.6 avant le saut majeur 4.0.0/6.1.1. La cible de livraison doit être la version stable la plus récente qui passe le projet complet, les tests, les mesures mémoire et les contrôles matériels. Il ne faut pas adopter le core 4.0 uniquement parce qu'il est plus récent si une bibliothèque essentielle, l'audio ou le BSP Cardputer ADV régresse. Le fork pioarduino doit être identifié comme tel et ne doit pas être présenté comme une version officielle PlatformIO ou Espressif.

### 5.2 Ruptures et risques amont majeurs

- M5Unified et M5GFX 0.3.0 sont déjà synchronisés. Leur mise à niveau a changé la rotation absolue : sur Cardputer / Cardputer ADV, l'ancien \`setRotation(1)\` correspond maintenant à \`setRotation(0)\`. Certaines fonctions ont également changé leur type de retour de \`void\` à \`bool\`. Les tests écran, microphone, haut-parleur, DMA, IMU et clavier ADV restent obligatoires.
- RadioLib 8.0.0 est une rupture d'API : les anciens appels \`begin(...)\` à paramètres individuels sont remplacés par des structures de configuration, de nombreux paramètres flottants deviennent des entiers, les callbacks d'interruption sont unifiés et des codes de retour changent. Il faut répertorier tous les appels SX1262 de \`lora.cpp\` avant la migration.
- FastLED 3.10.10 est plus récent que le 3.10.3 intégré ; les vieux benchmarks du fichier de pins comparaient surtout des versions sous IDF 4.4.7. Ils ne prouvent pas que 3.10.10 augmente ou réduit la mémoire sur la base actuelle. Mesurer le binaire et vérifier GPIO21 / SK6812 / RMT et la coexistence audio.
- ESP8266Audio 2.4.3 contient des corrections postérieures au tag 1.9.9, notamment autour des formats ID3v2.4, AAC corrompu et de la pile I²S ESP32. Le portage doit valider le lecteur local, les flux radio, les interruptions de lecture, la mémoire et les transitions microphone/haut-parleur.
- Le README de LibSSH-ESP32 dit que les cores 2.0.17 et 3.0.7 sont testés ; la version ROZA actuelle est 3.3.12. Un build réussi ne suffit pas à prouver la sûreté de SSH/TLS ou les allocations de mémoire. Comparer le port existant à wolfSSH sans perdre SSH/SCP et après examen des licences.
- TinyGPSPlus est central dans un parseur GNSS riche en champs NMEA personnalisés. Ne pas le remplacer par un parseur plus petit sans démontrer l'équivalence des constellations, GGA/GSV, qualité de fix, HDOP, âge des données et états d'antenne.

## 6. Audit des applications et modernisation sans suppression

Le menu réel contient 40 entrées. La ligne « modernisation prévue » décrit le travail à examiner à l'étape 2 et ne déclare pas déjà la fonction corrigée.

| Groupe | Application | Fichiers/technologie repérés | Modernisation à préserver ou à évaluer |
|---|---|---|---|
| TOOL | Time | horloge, date, chronomètre | Temps manuel et fonctionnement hors ligne ; NTP facultatif, fuseau et changement été/hiver |
| TOOL | IMU | M5Unified / BMI270 | Conserver cache partagé, état matériel indisponible et mesures réelles |
| TOOL | Files | Arduino SD / gestion de fichiers | Conserver les fichiers ; sécuriser le montage, les opérations destructives et exFAT |
| TOOL | Spectrum | ES8311 / audio / analyse | Tester micro, buffers audio, rendu et coexistence avec le haut-parleur |
| TOOL | Calc | calcul local | Garder l'application locale et ses tests de calcul |
| TOOL | Converter | conversions locales | Garder les conversions sans réseau ni clé |
| TOOL | Player | lecteur de fichiers WAV et audio | Conserver la lecture SD ; évaluer ESP8266Audio 2.4.3 et les formats sans dépasser la RAM |
| TOOL | Hash Oven | fonctions de hachage et crypto | Garder les fonctions, vérifier SHA-256, erreurs et vecteurs connus |
| SCAN | WiFi Chan | scan Wi-Fi passif | API Wi-Fi du core ; radio arrêtée hors activation explicite |
| SCAN | Sniffer | mode promiscuous Wi-Fi | Préserver suspension/reprise du Hotspot et les compteurs |
| SCAN | Wardrive | scan et journalisation géographique | Corriger l'API Wi-Fi interne retirée ; garantir journalisation sûre et SD non destructive |
| SCAN | Drone ID | Remote ID / découverte | Vérifier réception passive, NimBLE et parsing des trames |
| SCAN | BLE Scan | NimBLE-Arduino | Déjà épinglé sur la dernière version stable ; conserver EXT_ADV, PHY et limites HCI |
| NET | Hotspot | SoftAP, ESP-NETIF DHCP | Conserver gestion des clients et mot de passe aléatoire ; confirmer arrêt/reprise |
| NET | LAN Scan | Wi-Fi / découverte LAN | API du core, délais bornés et aucun blocage de l'interface |
| NET | NetProbe | TCP / DNS / HTTP | Distinguer les tests réseau explicites et les cibles contrôlées par l'utilisateur |
| NET | Router | statistiques et outils réseau | Tests logiciels existants, mesure mémoire et traitement non bloquant |
| NET | SSH | LibSSH-ESP32 | Préserver client SSH/SCP ; valider la compatibilité 3.3.12/4.0 et les alternatives |
| NET | ChatGPT | HTTPS, API configurable, clé facultative | TLS avec validation de certificat par défaut ; clé et fournisseur configurables, réseau optionnel |
| HID | BT Keys | NimBLE HID | Garder le profil clavier et les commandes utilisateur explicites |
| HID | BT Media | NimBLE HID / multimédia | Garder les contrôles multimédia et la gestion d'appairage |
| HID | Ducky | macros HID | Conserver l'exécution locale, la prévisualisation et les limites de sécurité |
| SIG | LoRa | RadioLib / SX1262 | Migrer RadioLib sans perte RX, scanner, presets, journaux ; TX manuel et régional |
| SIG | GNSS | TinyGPSPlus / UART1 | Conserver parseur NMEA, GPS hors ligne, constellations et réglages du Cap |
| SIG | Map | monde/tuiles et cache | Garder carte hors ligne ; cache en RAM borné et sources HTTPS vérifiées |
| SIG | IR | M5Stack / IR | Conserver envoi/réception compatibles avec le matériel et les codes existants |
| SKY | Astro | calculs locaux | Fonctionnement hors ligne, calculs et tests locaux |
| SKY | Weather | Open-Meteo / qualité de l'air | Passer les URL HTTP en HTTPS lorsque les endpoints le permettent ; cache et état réseau non bloquant |
| SKY | Planes | ADS-B.lol | HTTPS à évaluer ; garder le parsing en flux et les filtres mémoire |
| SKY | Sats | N2YO | Dépend actuellement d'une clé N2YO obligatoire ; étudier TLE publics CelesTrak + propagation orbitale locale, sans fausse clé |
| SKY | Typhoon | données JMA / JSON | Garder HTTPS avec CA, filtre JSON, gestion des données absentes |
| SKY | Quake | données sismiques | Vérifier source, HTTPS, fraîcheur et fonctionnement hors réseau |
| MORE | GitHub | API GitHub HTTPS | Préserver l'accès public sans token, gérer quotas, cache et dates de fraîcheur |
| MORE | Radio | ESP8266Audio / flux Web | Fonctionne actuellement uniquement avec URL HTTP ; évaluer HTTPS et la nouvelle pile audio |
| MORE | FX | Frankfurter / taux de change | Garder source publique, HTTPS, cache et date du taux |
| MORE | OKX | API OKX HTTPS | Conserver validation TLS et traiter réponse indisponible/quotas sans bloquer |
| MORE | Reader | fichiers locaux | Préserver les formats réellement présents et lecture sans réseau |
| MORE | BadApple | lecteur de ressource vidéo dédiée | Garder le format de ressource existant ; vérifier le fichier SD et le débit d'affichage ; un lecteur MP4 général est un projet séparé, non acquis |
| MORE | Apps | catalogue JSON et pages runtime | Le catalogue courant est limité à 8 apps, fichiers JSON et pages runtime bornés à 8 Kio ; ce n'est pas encore un chargeur sûr de modules natifs |
| MORE | Settings | paramètres système | Conserver les 15 réglages, les confirmations destructives et l'activation volontaire du réseau/radio |

### 6.1 Priorités transversales des applications

**P0 — fiabilité et intégrité**
1. Corriger l'API Wi-Fi obsolète dans Wardrive et retrouver une compilation CI verte.
2. Empêcher le montage automatique de la microSD tant que son format n'est pas identifié ; ne jamais laisser un test toucher la carte utilisateur.
3. Renommer ou réécrire l'action « Format SD » : elle supprime aujourd'hui récursivement les fichiers, elle ne crée pas de système de fichiers.
4. Conserver les modes hors ligne et les erreurs réseau non bloquantes.

**P1 — transport et confidentialité**
- Weather utilise des URL HTTP pour les prévisions et la qualité de l'air.
- Planes utilise HTTP pour la requête de position des avions.
- GeoLoc utilise HTTP vers ip-api.com lorsqu'aucune position GNSS/cache n'est disponible.
- Radio rejette explicitement les URL HTTPS et n'accepte que HTTP.
- Chat possède une option explicite \`CHAT_ALLOW_INSECURE\` qui appelle \`setInsecure()\`. Elle doit rester désactivée pour toute configuration normale ; les clés API ne doivent jamais être envoyées à travers une connexion dont le certificat n'est pas vérifié.
- GitHub, FX, OKX et Typhoon utilisent déjà des connexions TLS avec bundle ou certificat ; leurs chemins doivent être conservés et testés.
- Sats utilise HTTPS correctement vérifié, mais exige une clé N2YO intégrée à l'URL. Une source publique sans clé avec calcul local est préférable si la précision et les fonctionnalités sont équivalentes.
- Le catalogue Apps utilise HTTPS sur GitHub Raw et limite la source à ce domaine.

**P2 — disponibilité et autonomie**
- Ajouter cache local, horodatage et état « données périmées » aux services météo, avions, satellites, finance, radio et GitHub.
- Préserver les fonctions essentielles sans réseau. L'accès Internet ne doit pas être une condition de démarrage ou d'utilisation du Cardputer.
- Utiliser les réseaux Wi-Fi déjà enregistrés uniquement ; aucun démarrage Wi-Fi/Bluetooth automatique au démarrage.
- Pour les applications qui dépendent aujourd'hui d'une API, choisir des sources officielles/publiques et sans clé lorsque cela est possible, plutôt que d'inventer des identifiants.

## 7. Mises à jour, partitionnement et récupération

### 7.1 État actuel constaté

La table active \`firmware/partitions_8MB.csv\` contient :
- NVS ;
- PHY init ;
- une seule partition applicative \`factory\` à partir de 0x10000, de taille 0x7E0000.

Elle ne contient ni \`otadata\`, ni partitions \`ota_0\` et \`ota_1\`. Le fichier racine \`partitions.csv\` contient un ancien exemple avec deux slots, mais il est historique et interdit d'utilisation pour la compilation active par \`AGENTS.md\`.

Le système de livraison actuel produit :
- une image fusionnée USB à flasher à 0x0000, qui réécrit également la zone NVS et réinitialise donc les préférences ;
- un binaire application seul destiné à un firmware ROZA existant et à la partition courante, à flasher par USB à 0x10000 sans effacement global ;
- des SHA-256 de contrôle d'intégrité du fichier.

Ce deuxième binaire n'est pas un OTA : la carte doit être reliée en USB et placée en mode téléchargement. SHA-256 détecte une corruption ; sans signature et clé de confiance indépendante, il ne prouve pas l'identité de l'éditeur du paquet.

Il n'existe donc pas encore de moteur OTA de production, de mise à jour A/B ou de rollback de firmware. Le document \`docs/ARCHITECTURE.md\` a précédemment décrit deux slots comme s'ils étaient actifs ; cette description doit être corrigée pour refléter la table réellement utilisée.

### 7.2 Architecture à étudier à l'étape 3

1. Manifeste versionné signé couvrant la cible, la version, la taille, le hash SHA-256, le partitionnement minimum et le paquet.
2. Clé privée de signature hors du dépôt ; clé publique de vérification intégrée au firmware.
3. Vérification de signature avant l'installation, puis hash, taille, modèle et compatibilité.
4. Slots OTA A/B avec \`otadata\`, confirmation du premier démarrage et rollback bootloader activé.
5. Téléchargement par Wi-Fi uniquement après demande explicite, et installation locale depuis SD après validation de la carte.
6. Sauvegarde des préférences NVS avant migration ; récupération USB-C toujours disponible.
7. Étude précise du budget 8 Mo après compilation du binaire réellement modernisé. L'ancienne table A/B à la racine ne constitue pas une validation du dimensionnement courant.
8. Ne pas activer Secure Boot ou programmer des eFuses de façon irréversible sans procédure de provisionnement, récupération et essais distincts.

La documentation officielle ESP-IDF exige au moins deux partitions d'application OTA et une partition OTA Data pour le mode de mise à jour sécurisé ; le mécanisme de rollback exige en plus les options et l'auto-confirmation prévues au premier démarrage. Le partitionnement ROZA actuel ne remplit pas ces conditions.

## 8. Matrice de migration consolidée

| Domaine | État constaté | Cible de travail | Risque | Porte de validation |
|---|---|---|---|---|
| Core ESP32 | Arduino 3.3.12 / IDF 5.5.5 | tester 3.3.13/5.5.6 puis 4.0.0/6.1.1 | élevé | CI complète, API Wi-Fi/BLE/I²S/USB, mesures RAM/Flash |
| BSP écran | M5Unified/M5GFX 0.3.0 | conserver et auditer les appels | moyen | rotations, DMA, rendu, veille/réveil |
| Clavier ADV | TCA8418 sur I²C | conserver pilote existant | moyen | matrice complète 56 touches, Fn et modificateurs |
| IMU | BMI270 via M5Unified | conserver pilote existant | moyen | mesures, disponibilité et stabilité |
| Wi-Fi/BLE | API core + NimBLE 2.5.2 | conserver NimBLE ; remplacer les symboles internes | élevé | scans, hotspot, BLE HID/EXT_ADV, état radio |
| LoRa | RadioLib 7.7.1 + SX1262 | tester RadioLib 8.0.0 | élevé | RX, presets, IRQ, SPI partagé ; TX seulement après validation réglementaire |
| GNSS | TinyGPSPlus 1.1.0 de l'environnement PIO | vérifier provenance et garder couverture NMEA | moyen | constellation, fix, GSV/GGA, GPS offline |
| Audio | ESP8266Audio 1.9.9 | tester 2.4.3 | élevé | WAV/MP3/AAC/MIDI selon usages, flux, micro, haut-parleur, RAM |
| LED RGB | FastLED 3.10.3 | tester 3.10.10 | moyen | RMT GPIO21, timing et Flash/RAM |
| JSON | ArduinoJson 7.4.3 | conserver | faible | suites JSON et parsing flux |
| SSH | LibSSH-ESP32 5.9.0 | tester sur core moderne ; comparer wolfSSH | élevé | client, SCP, TLS, heap, licence |
| microSD | Arduino SD, montage RW | comparer SdFat 2.3.1 | critique | identification, exFAT/FAT, conservation des fichiers, bus partagé |
| Services Web | HTTP/HTTPS mélangés | HTTPS vérifié, cache, API sans clé | élevé | TLS avec heure/CA, erreurs réseau, mémoire |
| OTA | USB + SHA-256 | signature, A/B, rollback | critique | coupure d'alimentation, premier démarrage et récupération |
| Catalogue Apps | pages JSON et 8 éléments max | catalogue versionné sûr, sans faux chargeur natif | moyen | validation schéma, limites, intégrité et compatibilité |

## 9. Décisions recommandées

1. **Ne pas refaire l'interface maintenant.** La refonte VHS/CRT attend l'étape 4.
2. **Ne pas remplacer le BSP M5Stack déjà à jour.** Réutiliser M5Unified/M5GFX et les pilotes Cardputer ADV présents.
3. **Adopter le plus récent stable après preuve, pas par simple numéro de version.** Évaluer Arduino 4.0/IDF 6.1 comme cible prioritaire, avec 3.3.13/5.5.6 comme palier de comparaison.
4. **Mettre à niveau séparément les bibliothèques qui ont réellement évolué.** Priorités : ESP8266Audio, RadioLib et FastLED ; maintenir M5Unified, M5GFX, NimBLE et ArduinoJson tant qu'ils restent les dernières versions stables vérifiées.
5. **Ne pas changer de parseur GNSS sans matrice NMEA.** La couverture actuelle est plus importante que le remplacement nominal d'une dépendance.
6. **Moderniser les transports réseau.** HTTPS validé dès que disponible, aucune clé en HTTP, horodatage/caches, réseau facultatif et retour clair hors ligne.
7. **Traiter la microSD avant d'ajouter de grandes données ou des paquets.** Identifier le format, sécuriser le montage, vérifier l'accès exFAT et l'arbitrage SPI.
8. **Concevoir le vrai OTA comme une fonction de sécurité, pas comme un simple téléchargement.** Signature, double slot, rollback et récupération doivent être vérifiés ensemble.
9. **Conserver les 40 applications et leurs fonctions.** Un remplacement doit être démontré supérieur et reprendre les fonctions existantes ; aucun stub, aucune suppression de commodité.

## 10. Critères de sortie de l'étape 1

- [x] Dépôt, branche et commit exacts identifiés.
- [x] Chaîne de build réellement active et versions épinglées relevées.
- [x] CI du commit courant interrogée ; erreur de compilation exacte documentée.
- [x] Différence entre l'ancien rapport et l'état courant du dépôt clarifiée.
- [x] Nombre d'applications et groupes recalculé depuis le tableau de menu réel : 40 / 7.
- [x] Fonctions des applications rapprochées des fichiers sources actifs et plan de préservation établi.
- [x] Carte matérielle Cardputer ADV et Cap LoRa-1262, broches, bus partagés et adresse d'expandeur rapprochés des sources officielles et du code.
- [x] Limites Flash/RAM/absence de PSRAM, canvas, TLS, cartes, audio et radio documentées.
- [x] Versions stables amont et changements majeurs recherchés dans les dépôts officiels.
- [x] Matrice de migration avec risques et validations préparée.
- [x] HTTP en clair, dépendances API, secrets, contraintes microSD et absence d'OTA sécurisée recensés.
- [x] Incohérences documentaires du partitionnement et des anciennes versions de dépendances identifiées et corrigées dans les documents de référence.
- [x] Étapes 2 à 5 maintenues dans le bon ordre ; aucun changement du firmware ni refonte d'interface effectué au titre de cette étape.

**Conclusion de porte :** l'étape 1/5 est complète comme audit et préparation. La compilation échouée est enregistrée comme point de départ de l'étape 2, pas maquillée en succès. Les mesures du nouveau binaire et les essais physiques ne sont pas des preuves disponibles à ce stade et restent affectés aux portes de validation des étapes 2 et 5.

## 11. Sources primaires

### Dépôt ROZA
- Code courant : https://github.com/Yoyote84/Roza/tree/main
- Commit de référence : https://github.com/Yoyote84/Roza/commit/71cd1ff0216ec426f8d073153c838373181da5b9
- CI du commit : https://github.com/Yoyote84/Roza/actions/runs/45518114744
- Configuration PlatformIO : https://github.com/Yoyote84/Roza/blob/main/firmware/platformio.ini
- Verrouillage des dépendances : https://github.com/Yoyote84/Roza/blob/main/firmware/docs/dependency-pins.md

### Matériel et plateforme
- M5Stack Cardputer ADV : https://docs.m5stack.com/en/core/Cardputer-Adv
- M5Stack Cap LoRa-1262 : https://docs.m5stack.com/en/cap/Cap_LoRa-1262
- Arduino-ESP32 4.0.0 / IDF 6.1.1 : https://release-notes.espressif.com/?project=Arduino
- ESP-IDF 6.1.1 : https://github.com/espressif/esp-idf/releases/tag/v6.1.1
- pioarduino 61.04.00 : https://github.com/pioarduino/platform-espressif32/releases/tag/61.04.00
- pioarduino 55.03.313 : https://github.com/pioarduino/platform-espressif32/releases/tag/55.03.313

### Bibliothèques
- M5Unified : https://github.com/m5stack/M5Unified/releases
- M5GFX : https://github.com/m5stack/M5GFX/releases
- NimBLE-Arduino : https://github.com/h2zero/NimBLE-Arduino/releases
- RadioLib : https://github.com/jgromes/RadioLib/releases
- FastLED : https://github.com/FastLED/FastLED/releases
- ESP8266Audio : https://github.com/earlephilhower/ESP8266Audio/releases
- ArduinoJson : https://github.com/bblanchon/ArduinoJson/releases
- TinyGPSPlus : https://github.com/mikalhart/TinyGPSPlus
- LibSSH-ESP32 : https://github.com/ewpa/LibSSH-ESP32
- wolfSSH : https://github.com/wolfSSL/wolfssh/releases
- SdFat : https://github.com/greiman/SdFat/releases

### Services et mise à jour
- OTA et rollback ESP-IDF : https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/ota.html
- Open-Meteo : https://open-meteo.com/en/docs
- Frankfurter : https://frankfurter.dev/
- CelesTrak GP data : https://celestrak.org/NORAD/elements/gp.php
- ADS-B.lol API : https://www.adsb.lol/docs/open-data/api
- Radio Browser API : https://api.radio-browser.info/
- GitHub API limits : https://docs.github.com/en/rest/using-the-rest-api/rate-limits-for-the-rest-api
