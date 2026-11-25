# ROZA — Étape 2/5 : modernisation et intégration

**État de clôture logiciel :** 25 novembre 2026  
**Branche :** `main`  
**Commit validé par CI :** `3f41feb3a01c05b0034cb2508f55f3dc56e8bc47`  
**Workflow :** [run 194](https://github.com/Yoyote84/Roza/actions/runs/45676271094)

> Cette mise à jour remplace la matrice préparatoire devenue obsolète. Elle consigne les changements déjà présents, la validation automatisée et les limites qui relèvent des essais matériels (étape 5). Elle ne prétend pas que les essais physiques ont été faits.

## 1. Palier technique intégré

| Composant | Version verrouillée dans main | Statut |
|---|---|---|
| PlatformIO Core (CI) | 6.2.0 | Validé par workflow |
| pioarduino platform | 55.03.313 | Intégrée |
| Arduino-ESP32 | 3.3.13 | Intégrée |
| ESP-IDF | 5.5.6 | Intégrée via core |
| M5Unified / M5GFX | 0.3.0 / 0.3.0 | Intégrées, essais écran/audio sur appareil à faire |
| NimBLE-Arduino | 2.5.2 | Intégrée |
| RadioLib | 8.0.0 | Intégrée, SX1262 à tester sur matériel |
| FastLED | 3.10.3 | Conservée volontairement; 3.10.10 non adoptée sans tests |
| ArduinoJson | 7.4.3 | Intégrée |
| TinyGPSPlus | 1.1.0 | Intégrée |
| ESP8266Audio | 2.4.3 | Intégrée; tests audio physiques à faire |
| LibSSH-ESP32 | 5.9.0 | Conservée |
| SD | Arduino SD | Conservée; aucun formatage ou effacement automatique |

## 2. Validation automatisée du commit exact

Le workflow 194 est terminé avec succès (25 novembre 2026) :
- Compilation firmware Cardputer ADV : succès.
- Suite de régression logiciel : succès.
- Assemblage des images flashables : succès.
- Upload des artefacts : succès.
- Rendu des écrans : succès.
- Assemblage du viewer et du firmware : succès.
- Déploiement GitHub Pages : succès.

La CI ne prouve pas le fonctionnement des périphériques physiques.

## 3. Modernisations applicatives et règles de conservation

- La migration du core, des bibliothèques principales et du pilote radio a été intégrée.
- Les appels réseau utilisent les helpers communs de résolution, HTTP/JSON et TLS lorsqu'ils sont applicables. Le contrôle de certificat reste requis; ne pas réintroduire `setInsecure()` dans la livraison.
- Weather et qualité de l'air : source Open-Meteo, sans clé, HTTPS.
- GeoLoc : priorité aux données GNSS/cachées; fallback IP HTTPS.
- Sats : N2YO reste optionnel (clé API); ne pas afficher une erreur de clé comme si le service était indisponible.
- OKX : source primaire et fallback doivent rester explicites; une panne du premier fournisseur ne doit pas bloquer l'écran.
- BadApple : lecteur local sur SD; fichier attendu `/video/badapple.dat`, créé par l'outil de conversion; aucun fichier vidéo ne peut être livré dans le firmware sans tenir compte du budget Flash.
- Les apps et fonctions existantes doivent être conservées; aucune suppression fonctionnelle, aucune simulation/stub à la place d'un vrai service.
- Le jeu Hawkins retiré ne doit être réintroduit ni dans le firmware, ni dans les packages, ni dans les tests.

## 4. Pourquoi Arduino-ESP32 4.0.0 n'est pas adopté immédiatement

Arduino-ESP32 4.0.0 / ESP-IDF 6.1.1 est sorti le 19 novembre 2026. Le guide de migration officiel documente des changements incompatibles dans plusieurs API ESP-IDF et des paramètres de configuration. ROZA n'a pas encore une matrice de compatibilité démontrée sur toutes ses dépendances/peripherals pour ce saut majeur.

**Décision :** garder 3.3.13 / IDF 5.5.6 comme palier de livraison validé en CI; faire le saut 4.0.0 dans une branche/lignée de test distincte, et ne l'adopter que si le build, les régressions, les ressources et les essais Cardputer ADV sont concluants. Une version plus récente ne vaut pas une version stable pour ce matériel.

Références primaires :
- Arduino ESP32 4.0 migration guide : https://docs.espressif.com/projects/arduino-esp32/en/latest/migration_guides/3.x_to_4.0.html
- Arduino ESP32 4.0.0 / IDF 6.1.1 release : https://github.com/espressif/arduino-esp32/releases/tag/4.0.0
- M5Unified 0.3.0 : https://github.com/m5stack/M5Unified/releases
- M5GFX 0.3.0 : https://github.com/m5stack/M5GFX/releases

## 5. Conditions de clôture de l'étape 2

- [x] Core et dépendances principales migrés au palier choisi et versions verrouillées.
- [x] Compilation CI réussie sur le SHA exact.
- [x] Tests logiciels de régression passés.
- [x] Images flashables assemblées et artefacts publiés.
- [x] Viewer web généré et déployé.
- [x] Ancien jeu Hawkins absent de la vérification CI.
- [x] Pas d'effacement/formatage automatique de la carte SD.
- [x] Pas de faux statut d'essai matériel.
- [ ] Essais écran, clavier, audio, SD, GNSS, IMU, LoRa/SX1262 et comportement batterie sur Cardputer ADV physique — **étape 5**.

**Conclusion :** l'étape 2/5 est clôturable côté intégration logicielle/CI au palier retenu. Les tests de fonctionnement sur l'appareil restent explicitement réservés à l'étape 5. L'étape 3/5 peut commencer sans prétendre avoir validé le matériel.
