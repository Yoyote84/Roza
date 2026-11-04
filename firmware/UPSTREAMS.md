# Provenance des composants open source

Cette base privilégie les composants natifs ESP-IDF déjà maintenus pour le matériel Cardputer ADV. Aucun fichier de pilote du précédent dossier `main/include/` n'est réutilisé : ces pilotes ont été audités comme incorrects/incomplets.

## Composant utilisé directement

### ESPP — M5Stack Cardputer BSP

- Dépôt : https://github.com/esp-cpp/espp
- Composant : https://components.espressif.com/components/espp/m5stack-cardputer
- Version épinglée : **1.3.11**
- Licence : **MIT** (licence du dépôt ESPP et métadonnées du composant).
- Utilisation : abstraction matérielle Cardputer et Cardputer ADV, ST7789V2/LVGL, TCA8418, BMI270, batterie, microSD, SX1262 et GNSS du Cap.
- Dépendances transitives gérées par ESP Component Manager.
- La version exacte du composant est fixée dans `main/idf_component.yml`. Après le premier `idf.py build`, conserver `dependencies.lock` pour figer aussi les dépendances transitives.

Le composant documente explicitement le support du Cap LoRa + GPS (SX1262 + ATGM336H) et la gestion du bus SPI d'extension partagé avec la microSD.

## Référence firmware secondaire

### Lewis-Peter — cardputer-adv-firmware

- Dépôt : https://github.com/Lewis-Peter/cardputer-adv-firmware
- Licence du dépôt : MIT ; les fichiers tiers sous `lib/` conservent leurs propres licences.
- Utilisation dans cette base : **référence technique uniquement**, aucun fichier source copié.
- Intérêt : contraintes 8 Mo Flash / absence de PSRAM, détails matériels Cardputer ADV et Cap LoRa-1262, buffer UART GNSS.
- Son environnement est Arduino + M5Unified + PlatformIO ; il n'est donc pas mélangé à la base ESP-IDF native.

## Documentation matérielle officielle

- Cardputer ADV : https://docs.m5stack.com/en/core/Cardputer-Adv
- Cap LoRa-1262 : https://docs.m5stack.com/en/cap/Cap_LoRa-1262

La documentation officielle confirme notamment le clavier TCA8418 sur G8/G9/G11, le bus SPI partagé de la microSD et du Cap, le SX1262, et le GNSS ATGM336H-6N@AT6668.

## Règles de provenance

- Ne pas copier un fichier tiers sans vérifier sa licence et conserver son attribution.
- Ne pas présenter un dépôt Arduino comme un pilote ESP-IDF natif.
- Ne pas déclarer le matériel fonctionnel sur la seule base de la compilation.
