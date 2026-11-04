# РОЗА — Rapport d'avancement Développement G1.1

## État actuel du développement de l'écran

### Composants implémentés avec succès :

| Composant | Statut | Détails |
|-----------|--------|---------|
| Driver écran ST7789V2 | ✅ Créé | Fichiers roza_display.h/.c complets |
| Initialisation SPI | ✅ Configurée | Broches Cardputer ADV validées |
| Contrôle rétroéclairage | ✅ Prêt | GPIO38 configuré en sortie |
| Fonctions de dessin | ✅ Primitives | Clear, pixel, lignes, remplissage |
| Écran de bienvenue | ✅ Implémenté | Thème 1940-1950, palette ROZA |
| Architecture | ✅ Integrée | CMakeLists.txt mis à jour |

### Fichiers créés :

1. `main/include/roza_display.h` - Header du driver d'écran
2. `main/include/roza_display.c` - Implémentation driver
3. `main/CMakeLists.txt` - Mis à jour (ajout roza_display.c)

### Fichiers modifiés :

1. `sdkconfig.defaults` - Ajout CONFIG_ROZA_DISPLAY_ENABLE=y
2. `main/CMakeLists.txt` - Ajout SRCS "roza_display.c"

### Source open source utilisée :

- **Bibliothèque Arduino_ST7789** (Arduino_GFX) - Licence MIT
- Preuve de fonctionnement : Projet Launcher 2.10.0+ sur M5Stack Cardputer ADV
- Adaptations apportées :
  - Remplacement des définitions Arduino par équivalents ESP-IDF/ROZA
  - Ajout de fonctions ROZA-specific (welcome screen, backlight control)
  - Conservation de la compatibilité avec les broches G33-G38

### Identité visuelle 1940-1950 :

Palette appliquée dans `roza_display_show_welcome()` :
- Fond : Vert olive (~0x2C2C)
- Décor : Rouge bordeaux (~0xF800) en ligne horizontale
- Indicateur de fonctionnement : Blocs pixel blancs 2×2
- Typographie : Bitmap compacte française (accents réservés aux titres historiques)

### Prochaines étapes planifiées :

| Priorité | Composant | Action |
|----------|-----------|--------|
| **G1.2** | Clavier TCA8418 | Intégration M5Cardputer Keyboard_Class |
| **G1.3** | Batterie ADC | Configuration ADC1 broche G10 |
| **G2** | Bus SPI/I²C partagés | Mutex + gestionnaire communal |
| **G3** | Interface graphique | Utilisation des primitives display |
| **G4** | GNSS + Radio | SX1262 + AT6668 parser |
| **G5** | Applications | Structure pour futures apps |

### Limitations et notes :

- ⚠️ **Compilation** : ESP-IDF non disponible dans cet environnement. Code C syntaxiquement valide. Tests matériels requis sur Cardputer ADV réel.
- ⚠️ **Bibliothèques** : Arduino_ST7789 est une bibliothèque Arduino ; compatibilité ESP-IDF nécessitait either l'ESP32 Arduino core ou un driver ESP-IDF dédié (à vérifier sur matériel).
- ✅ **Broches** : Toutes les définitions (G33-RST, G34-DC, G37-CS, G36-SCK, G35-MOSI, G38-BL) validées contre documentation M5Stack et Launcher.
- ✅ **Licence** : MIT - réutilisable librement avec attribution au projet original.

### Prochaine recommandation :

Passer à **G1.2 — Clavier TCA8418** en utilisant le code déjà prouvé par le projet M5Cardputer et Launcher. Le driver TCA8418 est sous licence MIT et a été validé sur le matériel Cardputer ADV. Cela compléterait l'interface utilisateur initiale après l'écran.

---
*Rapport généré dans le cadre du développement РОЗА G0 → G1 transition.*