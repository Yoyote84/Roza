# Audit pré-flash complet — ROZA / Cardputer ADV

Date : 4 novembre 2026  
Dépôt : `Yoyote84/Roza`  
Branche autorisée : `main` uniquement  
Base de code examinée : `79463ee7c1506439e24c0483c941036c3495c01b`  
Cible : M5Stack Cardputer ADV — ESP32-S3FN8, 8 Mio de Flash, sans PSRAM.

## 1. Périmètre et architecture

L'inventaire GitHub recense 373 entrées suivies dans 39 dossiers, dont 335 entrées sous `firmware/`.

- Projet actif : `firmware/`, PlatformIO + Arduino-ESP32, environnement `cardputer-adv`.
- Sources de production : `firmware/src/`.
- Dépendances : versions exactes épinglées dans `firmware/platformio.ini`.
- Outils de test et simulateur : `firmware/tools/`.
- Certificats racines : `firmware/data/cert/`.
- Les `stubs` sont confinés aux tests et au simulateur hôte ; ils ne sont pas des pilotes de production.
- Les fichiers racine `CMakeLists.txt`, `main/`, `partitions.csv` et `sdkconfig.defaults` sont historiques. Ils ne constituent pas la chaîne de compilation active.
- Aucun fichier `secrets.h` ou `config_local.h` réel n'est présent dans Git. Ces fichiers sont ignorés par `.gitignore`.

## 2. Chaîne de compilation et mémoire Flash

La compilation PlatformIO du commit précédent a réussi. Le workflow actuel a été renforcé pour exécuter les suites de tests hôte avant de publier l'image flash ; son résultat doit être vérifié sur le SHA final avant flash.

Configuration observée :

- ESP32-S3FN8, Flash configurée à 8 Mio.
- Table de partitions active : NVS à `0x9000`, PHY à `0xE000`, application factory à `0x10000`.
- Une seule partition applicative : pas de double slot OTA.
- Image fusionnée : bootloader à `0x0000`, table à `0x8000`, application à `0x10000`.
- La compilation antérieure produisait une application d'environ 3,68 Mo, qui tient largement dans la partition factory d'environ 7,88 Mo.
- L'image fusionnée remplit les intervalles non occupés avec `0xFF`. La zone NVS est donc réinitialisée à chaque flash de cette image : préférences, fuseau, identifiants Wi-Fi et mot de passe hotspot sont perdus puis recréés.
- La microSD est indépendante de la Flash interne et n'est pas modifiée par le flash USB.
- Les mises à jour sont manuelles par USB ; aucun mécanisme OTA n'est configuré.

## 3. Défauts corrigés pendant cet audit

- Fuseau horaire initial : Pékin remplacé par Europe centrale.
- Serveurs NTP et cible réseau de référence : valeurs par défaut chinoises remplacées par des valeurs internationales adaptées à une configuration française.
- Hotspot : suppression du mot de passe public fixe `cardputer123`. Un mot de passe unique est généré avec la source d'entropie du ESP32-S3 puis enregistré dans NVS ; une valeur explicite dans `config_local.h` reste possible.
- Ajout de `firmware/src/config_local.h.example`, fichier modèle manquant auquel `config.h` faisait déjà référence.
- Contrôles du paquet flash renforcés : chevauchements bootloader/table/app vérifiés.
- Documentation du flash : réinitialisation NVS explicitée.
- CI : ajout des tests logiciels hôte existants au workflow firmware.

Aucun fichier de dessin, de rendu, de thème ou de disposition de l'interface n'a été modifié.

## 4. Risques et limites encore présents

### microSD — important

- `sdInit()` monte actuellement la carte en lecture/écriture ; aucun mode lecture seule n'est garanti.
- Aucun formatage automatique n'est lancé au démarrage.
- La fonction `formatSD()` ne fait pas un vrai formatage : elle supprime récursivement les fichiers et dossiers. L'action est déclenchée uniquement après deux confirmations, mais elle reste destructive.
- Wardrive et certaines fonctions de journalisation peuvent écrire sur la microSD quand l'application est explicitement utilisée.
- Une carte SDXC de 128 Go formatée en exFAT peut ne pas être reconnue par le pilote Arduino SD utilisé. Ne pas tester sur une carte contenant des données non sauvegardées.
- Première validation recommandée : carte de test vide, format connu, aucune opération d'écriture ni de nettoyage.

### LoRa / radio

- Le Cardputer ADV seul ne contient pas le SX1262. LoRa et GNSS nécessitent le M5Stack Cap LoRa-1262 et son antenne adaptée.
- Le démarrage de l'application LoRa sélectionne la réception ; aucune émission automatique n'est déclenchée.
- L'application permet toutefois une émission manuelle. Les préréglages contiennent aussi US915 et CN470, et le code n'impose pas encore de garde régionale ni de budget de duty-cycle.
- Le chat LoRa ROZA est un protocole privé simple, non chiffré et non compatible Meshtastic. Ne pas y transmettre de données privées.
- Ne pas tester l'émission en France avant une validation réglementaire et matérielle. Les premiers essais radio doivent rester en réception uniquement, antenne installée.

### GNSS et cartographie

- Le GNSS dépend du Cap ; sans ce module, l'initialisation série ne prouve aucune réception satellite.
- La source de tuiles par défaut est Autonavi en GCJ-02, adaptée principalement à la Chine. La carte détaillée en Europe nécessite une source WGS-84 autorisée et compatible avec le client HTTP actuel.
- Le code sait afficher des états sans fix ; la qualité réelle, le temps au premier fix et la stabilité doivent être testés dehors sur matériel.

### Autres périphériques

- Infrarouge : émetteur uniquement, pas de récepteur ; apprentissage de télécommandes impossible sur le matériel intégré.
- Ducky : exécute des scripts de frappe via BLE HID (et non USB HID). Ne lancer que des scripts connus et contrôlés : ils peuvent saisir des commandes sur l'appareil jumelé.
- Batterie : estimation logicielle non calibrée sur plusieurs cycles de la carte réelle.
- LED RGB : son alimentation partage la ligne du rétroéclairage ; quand la LED est active, le firmware force le rétroéclairage à pleine luminosité.
- Audio, clavier ADV TCA8418, BMI270, écran, veille/réveil et USB natif compilent, mais nécessitent des essais physiques.
- Bluetooth : la pile BLE garde une part importante de la RAM et le firmware la suspend/libère selon les applications. L'initialisation BLE coupe le Wi-Fi ; le gestionnaire réseau attend la libération BLE avant de relancer une connexion. Le chemin de démontage manuel dans `bt.cpp` est complexe et les commentaires historiques décrivent des blocages possibles dans l'ancienne séquence de deinit. Il faut tester plusieurs cycles scan/HID → sortie → retour Wi-Fi sur la carte réelle.
- Sans PSRAM, les fonctions lourdes (TLS/Chat, carte GNSS, BLE, audio et gros flux) peuvent manquer de mémoire contiguë. Les mécanismes de libération et de repli du code doivent être éprouvés en utilisation réelle.

### Réseau et services externes

- Wi-Fi et Bluetooth sont arrêtés au démarrage et doivent être activés volontairement.
- Les clés API et paramètres privés sont vides par défaut. ChatGPT, N2YO/Sats et Router/Clash nécessitent une configuration valide ; leur présence dans le menu ne signifie pas qu'ils sont prêts sans compte/serveur.
- La cible VPN NetProbe par défaut est une adresse de documentation non routable : elle doit être remplacée par l'utilisateur.
- Les applications dépendant d'Internet ne fonctionneront pas hors réseau ; les fonctions locales restent indépendantes.

## 5. Tests logiciels disponibles et limites de simulation

- Les suites hôte couvrent notamment les calculs GNSS, le protocole LoRa de flux, le décodage Remote ID, l'encodage IR, la batterie simulée, l'horloge, les calculs astronomiques, le JSON en flux, la calculatrice, Hash Oven et les structures Router/RID.
- Le simulateur `uisim` compile le vrai code de dessin et rend des PNG ; il utilise des stubs pour l'écran, le réseau et les périphériques. Ce n'est pas un émulateur complet de l'ESP32 ni une preuve de fonctionnement matériel.
- Aucun projet/configuration Wokwi n'est présent dans le dépôt à la date de cet audit.
- Les tests hôte ne valident pas les broches, le câblage, l'alimentation, la réception RF, l'ADC, l'audio réel ni les performances thermiques.

## 6. Plan d'essais physiques après le premier flash

1. Démarrer sans microSD et sans Cap pour isoler la carte de base.
2. Vérifier écran, orientation, luminosité, 56 touches, Fn/modificateurs, bouton G0 et veille/réveil.
3. Vérifier son, microphone, sortie casque, IMU et lecture batterie/charge.
4. Tester ensuite une microSD de test vide au format connu ; commencer par la détection et la lecture seulement.
5. Activer volontairement le Wi-Fi, tester connexion/déconnexion et vérifier qu'il reste arrêté après redémarrage ; faire de même pour BLE.
6. Si le Cap est présent : vérifier d'abord l'expandeur I²C, puis GNSS en extérieur et LoRa en réception avec antenne adaptée.
7. Ne pas émettre en LoRa avant validation réglementaire et ajout d'une protection régionale/duty-cycle.
8. Tester IR uniquement vers un appareil cible non sensible.
9. Refaire les applications gourmandes une par une en observant heap libre, plus grand bloc, minimum historique, redémarrages et température.
10. Consigner séparément chaque résultat physique ; ne jamais assimiler une compilation verte à un test matériel réussi.

## Conclusion

La chaîne de compilation et la création d'une image USB unique sont en place. Les corrections de configuration et de sécurité ci-dessus ne modifient pas l'interface. Le firmware peut passer à une première installation contrôlée, mais il ne faut pas déclarer toutes les fonctions validées avant les essais physiques. Les points les plus sensibles avant usage normal restent la microSD, la réglementation/émission LoRa, la cartographie européenne et le budget mémoire sans PSRAM.
