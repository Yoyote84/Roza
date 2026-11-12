# ROZA — contrat d'intégration des services matériels

Ce document fixe le propriétaire unique de chaque périphérique et le contrat à respecter quand les fonctions de Launcher 3.0.0 sont portées dans le firmware ESP-IDF de NucleoOS. Il évite de faire fonctionner deux pilotes sur le même écran, le même bus I2C, la même carte SD ou le même contrôleur Wi-Fi.

## Règle d'architecture

**NucleoOS est le noyau matériel et le propriétaire de l'unique démarrage. Launcher fournit des applications et des fonctions qui doivent être adaptées pour utiliser les services NucleoOS.** Il ne faut pas lancer le `setup()/loop()` Arduino en parallèle de `app_main()` et `nucleo_app_run()`.

Le test de compatibilité Arduino/ESP-IDF qui compile `launcher/src/idf/launcher_platform.cpp` est vert. Le prototype NucleoOS compile le vrai `main.c`, son graphe de composants et le vrai composant de plateforme Launcher avec Arduino autostart désactivé. Cela valide le socle de compilation sélectionné, pas encore l'ensemble des applications Launcher ni le fonctionnement matériel conjoint.

## Propriétaire et adaptateur requis

| Domaine | Propriétaire ROZA unique | Source Launcher à adapter | Contrat d'intégration |
|---|---|---|---|
| Démarrage / ordonnanceur | `nucleoos/firmware/main/main.c`, `nucleo_app_run()` | `launcher/src/main.cpp` | Ne pas lancer `setup()`, `loop()` ou une deuxième tâche de menu Launcher. Enregistrer les fonctions portées dans le registre d'applications Nucleo. |
| Écran Cardputer ADV | `nucleo_ui`, objet M5GFX `d`, cible active `nucleo_app_gfx()` | `launcher/lib/DisplayDrivers/src/backends/m5gfx_hal.cpp` | Le backend Launcher utilise actuellement `M5.Display` via M5Unified et appelle `M5.begin()`. Le port ROZA doit cibler le `LovyanGFX*` fourni par `nucleo_app_gfx()`, ne pas réinitialiser le panneau et ne pas allouer un deuxième canvas plein écran. Les sprites temporaires doivent utiliser le même périphérique parent et respecter le chemin canvas/direct de Nucleo. |
| Clavier / bus I2C | `nucleo_kbd`, API `nucleo_kbd_read()`, bus ADV possédé par le clavier | `boards/m5stack-cardputer/interface.cpp` | Ne pas compiler le backend TCA8418 de Launcher sur ROZA ADV : il appelle `Wire.begin(8,9)`, crée un autre `Adafruit_TCA8418` et active sa propre IRQ. Ajouter un traducteur des événements `NK_*`, caractères et modificateurs vers les événements clavier attendus par les applications Launcher, sans réinitialiser le bus. |
| Batterie | `nucleo_power_battery_pct()` / `nucleo_power_battery_mv()` | `getBattery()` dans le port Cardputer | Réutiliser la mesure ADC mise en cache de Nucleo. Propager `-1` quand la mesure est indisponible ; ne pas inventer un pourcentage. Cette jauge reste une estimation tension-vers-pourcentage, pas une jauge de charge matérielle dédiée. |
| Luminosité | `nucleo_ui_set_brightness()` | `_setBrightness()` | Transmettre le niveau au service d'écran Nucleo ; ne pas ajouter un second PWM ou une autre gestion de rétroéclairage. |
| Wi-Fi STA/AP et événements réseau | `nucleo_setup_*` et pile ESP-IDF initialisée au démarrage Nucleo | `launcher/src/idf/idf_wifi.cpp` | Ne pas initialiser une deuxième fois `esp_wifi`, les netifs ou les handlers. Fournir une façade Launcher qui délègue scan, join, AP, statut et IP aux fonctions Nucleo, avec traduction exacte des états/erreurs. |
| microSD / système de fichiers | `nucleo_storage`, montage unique de la carte | `launcher/src/sd_functions.cpp` | Ne pas appeler `SD.begin`, `SD_MMC.begin`, ni démarrer un bus SPI concurrent. Porter les opérations fichier vers le volume Nucleo monté ; conserver les répertoires `/sd` et `/cfg` et définir une migration explicite des paramètres Launcher, sans formatage ni effacement implicite. |
| Catalogue / applications | `nucleo_registry`, `nucleo_app_register()`, store Nucleo | `launcher/src/app_registry.cpp`, catalogue/OTA | Convertir les entrées Launcher vers le registre/app framework Nucleo. Un seul accueil et une seule recherche/favoris/récents ; aucune seconde boucle de menu. Préserver les actions/capacités non redondantes dans les applications ROZA. |
| Redémarrage / OTA | `esp_restart()`, `esp_ota_* `, politique de validation/rollback Nucleo | mises à jour Launcher | Conserver le mécanisme A/B, l'état pending-verify et la confirmation différée Nucleo. Les fonctions de catalogue Launcher peuvent demander une mise à jour au service commun ; elles ne doivent pas écrire directement dans une partition en contournant la politique. |
| Interface Web / réseau local | serveur HTTP, PWA et authentification Nucleo | WebUI / services web Launcher | Un seul serveur et un seul modèle d'authentification. Fusionner les routes et capacités utiles ; pas de deuxième serveur qui tente de réserver le même port. |
| Partitionnement / sauvegarde | table de partition ROZA approuvée et outils de récupération à définir | `partitioner.cpp`, sauvegarde/restauration Launcher | Aucune fonction d'écriture de table de partitions ne doit être activée avant une conception et une vérification de l'ensemble 8 MB, OTA A/B, données, NVS et récupération. |

## Points d'attention confirmés dans les sources épinglées

- Le port Launcher Cardputer démarre le clavier TCA8418 lui-même sur SDA 8 / SCL 9 / adresse 0x34. Nucleo ADV utilise déjà son propre service clavier/I2C : les deux initialisations ne sont pas compatibles.
- Le backend Launcher DisplayDrivers M5GFX est conçu autour de `M5.Display` et de `M5.begin()`. Nucleo expose un objet M5GFX distinct `d` et redirige le dessin des applications avec `nucleo_app_gfx()`. Un adaptateur ciblant la même cible graphique est nécessaire ; utiliser ce backend sans adaptation pourrait initialiser ou dessiner sur le mauvais objet.
- Nucleo expose déjà les API de clavier, batterie, luminosité, Wi-Fi, stockage, registre et applications mentionnées dans le tableau. Ce sont les points d'entrée à utiliser avant d'ajouter un autre pilote.
- La carte est sans PSRAM. Un canvas ou un tampon plein écran supplémentaire peut consommer une part importante du plus grand bloc RAM contigu. Le dessin doit réutiliser le tampon Nucleo et chaque gros tampon d'application doit être mesuré.

## Première couche d'adaptation livrée

Le composant `roza_launcher_hal` fournit maintenant un contrat commun aux futures applications portées :

- `roza_launcher_display_target()` retourne le pointeur courant de `nucleo_app_gfx()`. L'application doit le récupérer pendant chaque callback de dessin et ne pas le conserver entre deux frames : NucleoOS peut basculer entre canvas partagé et dessin direct.
- `roza_launcher_key_from_callback(key, ch, out)` traduit les événements reçus dans le callback natif `on_key(key, ch)`, avec le masque de modificateurs courant. Il ne lit pas la file clavier, car NucleoOS l'a déjà consommée avant l'appel de l'application.
- Les fonctions batterie, tension, état SD et luminosité délèguent aux API NucleoOS. Une batterie inconnue reste `-1`.
- L'application native **Diagnostic ROZA** est enregistrée dans le menu unique. Elle affiche les touches/modificateurs reçus et l'état batterie/SD pour guider les essais sur le matériel.
- Le traducteur clavier pur est testé côté hôte pour les flèches, Enter, Esc, Backspace, Tab, caractères et les cinq modificateurs.

Cette couche n'appelle jamais `M5.begin()`, `nucleo_ui_init()`, `nucleo_kbd_init()`, `Wire.begin()` ou un second montage SD. Le vrai backend amont `launcher/lib/DisplayDrivers/src/backends/m5gfx_hal.cpp` est maintenant compilé en mode `ROZA_NUCLEO_TARGET` : `M5_DRV` est redirigé vers `nucleo_app_gfx()` à chaque opération de dessin ; `init()` et `begin()` ne réinitialisent rien ; `display()` laisse le run loop Nucleo présenter l'image ; `setRotation()` ne remplace pas l'orientation Nucleo ; les sprites utilisent la cible Nucleo. L'application native `Affichage Launcher` vérifie ce chemin avec un sprite limité à 72×18 pixels. Elle active le dessin direct pendant sa durée de vie afin que le parent du sprite reste stable, puis libère son allocation à la sortie. Aucun second canvas plein écran n'est créé. Ce test de compilation et de cycle de vie ne valide pas encore le rendu physique ni le pic de RAM réel.

## Étapes suivantes

1. Porter une première fonction applicative Launcher réelle, en priorité parmi celles qui ne dupliquent pas Files, Store, Wi-Fi ou USB déjà natifs, dans le cycle de vie `on_enter/on_key/on_tick/on_draw/on_exit`. L'application `Affichage Launcher` est seulement une sonde du backend, pas une application Launcher complète.
2. Remplacer les accès directs Launcher au Wi-Fi, à la batterie, à la luminosité et à la SD par les façades Nucleo et compiler chaque groupe réel.
3. Porter les autres applications/fonctions Launcher dans le registre Nucleo, puis vérifier individuellement catalogue, OTA, installateur SD, WebUI, USB-MSC et outils de partition.
4. Mesurer taille ELF/flash, heap libre/minimal et plus grand bloc contigu ; confronter le résultat à la table de partitions 8 MB et aux deux banques OTA.
5. Effectuer ensuite les essais physiques Cardputer ADV (écran, clavier, Wi-Fi, SD, audio, capteurs, USB, OTA et récupération), après validation explicite de l'image et de la procédure de retour arrière.

Aucune fonction marquée « à adapter » ci-dessus n'est considérée comme fusionnée simplement parce qu'elle apparaît dans la liste cible. Aucun test de flash matériel n'a été réalisé.
