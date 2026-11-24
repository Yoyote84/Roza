**English** | [简体中文](dependency-pins.zh-CN.md)

# Verrouillage des versions des dépendances (mise à jour du 24/11/2026)

    Summary up front: **The platform, Arduino core, and all libraries are strictly pinned to exact versions**.
    Any upgrade must be performed and tested individually following the procedure at the end of this document.
    This policy was instituted when a fresh build on a clean server saw Flash usage jump from 43.7% to 48.5%
    and static RAM increase by 6KB without changing a single line of code—simply because version ranges like
    `^3.7` resolved to newer library releases.

---

## Migration du socle — décision du 24/11/2026

La base active visée pour le Cardputer ADV est désormais :

- PlatformIO Espressif32 **8.0.0** ;
- Arduino-ESP32 **3.3.12**, basé sur ESP-IDF **5.5.5** ;
- M5Unified **0.3.0** et M5GFX **0.3.0**, obligatoirement synchronisés ;
- NimBLE-Arduino **2.5.2** pour la pile BLE et le scan étendu 1M/Coded PHY ;
- PlatformIO Core **6.2.0** dans la CI.
- Les fonctions de hachage MD5/SHA-256 utilisent une couche de compatibilité Mbed TLS 2/3 ; les codes de retour sont contrôlés.
- Le header `HTTPClient.h` est inclus avant `M5Unified` dans `gnss.cpp` afin que M5GFX 0.3.0 déclare ses surcharges `drawJpg(Stream*)` ; le décodage des JPEG reste en flux, sans charger toute la tuile en RAM.
- La sélection du sous-domaine des tuiles gère explicitement une liste vide, configuration hors ligne par défaut.
- Le scan BLE étendu configure les événements HCI à **257 octets**, requis par NimBLE 2.5.2 pour EXT_ADV ; la valeur générique de 70 octets du SDK est harmonisée avant la compilation des bibliothèques.

La sélection 8.0.0 reste volontairement plus conservatrice que la branche Arduino 4.0 / ESP-IDF 6.1.1. Les dépendances non indispensables au portage restent épinglées à leurs versions précédentes : elles seront mises à niveau séparément uniquement si un conflit de compilation ou un besoin concret l'exige.

**Attention :** les mesures RAM/Flash de la section benchmark ci-dessous ont été obtenues sous Arduino 2.0.17 / ESP-IDF 4.4.7. Elles sont un point de comparaison historique, pas une mesure de la nouvelle base. La compilation et la CI doivent fournir de nouvelles mesures avant de déclarer la migration validée.

M5Unified/M5GFX 0.3.0 change la sémantique des rotations absolues : sur Cardputer / Cardputer ADV, l'ancien `setRotation(1)` devient `setRotation(0)` pour conserver le même paysage. Les fonctions dont le type de retour change de `void` à `bool` doivent être adaptées à leurs usages. Les tests matériels d'écran, micro, haut-parleur, SPI, clavier ADV et IMU restent obligatoires.

---

## Versions actuellement épinglées après migration

| Dependency | Version | Notes |
|---|---|---|
| platform `espressif32` | 8.0.0 | Arduino 3.3.12 / IDF 5.5.5 |
| `framework-arduinoespressif32` | Résolu par la plateforme 8.0.0 | Arduino 3.3.12 / ESP-IDF 5.5.5 |
| M5Unified | 0.3.0 | À synchroniser avec M5GFX 0.3.0 |
| M5GFX | 0.3.0 | Épinglé explicitement avec M5Unified 0.3.0 |
| NimBLE-Arduino | 2.5.2 | Nouvelle pile BLE sans Bluedroid direct; BLE scan/HID, annonces étendues et PHY Coded |
| FastLED | 3.10.3 | Maintenu pour isoler cette migration ; revalider la version récente sous IDF 5.5.5 |
| ArduinoJson | 7.4.3 | |
| RadioLib | 7.7.1 | |
| TinyGPSPlus | 1.1.0 | |
| LibSSH-ESP32 | 5.9.0 | |
| ESP8266Audio | 1.9.9 (git tag) | Conservé pour isoler la migration ; compatibilité IDF 5.5.5 à confirmer par la CI |

---

## Why "Newer is Not Always Better"

This board lacks PSRAM, and the 64.8KB full-screen canvas resides permanently in internal RAM. Features like TLS handshakes and GNSS mapping already operate near available memory thresholds. When dependencies update, an increase of just a few kilobytes in static RAM can turn occasional edge-case failures into permanent out-of-memory crashes—often manifesting with symptoms completely detached from the upgraded library, making root cause analysis difficult.

Furthermore, several firmware subsystems rely on **specific driver behaviors**: direct register writes to the ES8311 (pop suppression on the spectrum page), GPIO38 serving dual roles for LCD backlight and RGB LED power (`led.cpp`), and frequent dynamic transitions between microphone and speaker. Internal modifications in upstream libraries can compile cleanly yet fail subtly at runtime on physical hardware.

Unpinned versions also introduce cross-environment inconsistency: builds produced locally, in CI, or across different workstations differ, obscuring whether an issue stems from code changes or shifting library dependencies.

---

## Benchmark: The Cost of Upgrading Each Library

Measured via `pio run` in an isolated test environment, upgrading each library individually while keeping all others pinned:

| Configuration | RAM (bytes) | Flash (bytes) | Delta vs Baseline |
|---|---|---|---|
| Baseline (current pinned versions) | 110,980 | 3,631,405 | — |
| Only M5GFX → 0.2.30 | 110,972 | 3,638,853 | Flash +7.4KB |
| Only M5Unified → 0.2.23 | — | — | Build failed: requires M5GFX ≥ 0.2.30 |
| M5Unified 0.2.23 + M5GFX 0.2.30 | 111,108 | 3,644,389 | RAM +128B, Flash +13KB |
| Only FastLED → 3.10.5 | 117,308 | 4,006,213 | **RAM +6.3KB, Flash +375KB** |
| Upgrade all dependencies | 117,444 | 4,019,265 | RAM +6.5KB, Flash +388KB |

### FastLED 3.10.5 : benchmark historique sous Arduino 2.0.17 / IDF 4.4.7

- Newly added audio/FFT compilation units (`fl.audio+.cpp.o`, unused by this project) introduce references to `std::ios_base::Init`. The linker pulls in libstdc++'s complete iostream and locale machinery (`std::locale::_Impl`, `time_get`/`money_get`/`num_get`, wide character formatting, etc.). Memory impact: `.flash.rodata` +227KB, `.flash.text` +147KB, `.dram0.bss` +6KB, IRAM +1KB.
- Driver architecture changed: 3.10.3 uses RMT hardware for GPIO21 (`ClocklessController<21, SK6812>`), whereas 3.10.5 falls back to `ClocklessBlockingGeneric` on this IDF 4 core, toggling pins via busy-waiting CPU cycles. LED effect modes update every 30ms, which would contend for CPU cycles against I2S audio and Wi-Fi processing.
- The release contains no functional improvements for a single SK6812: 3.10.4 release notes explicitly stated "No FastLED source code changes", and 3.10.5's 122 commits focus primarily on LPC/RP2040/C2 platforms, containerization, test fixtures, and CI.

Upgrades should be deferred until upstream removes the iostream dependency and retains hardware RMT support on IDF 4.

### Historique — M5Unified 0.2.23 + M5GFX 0.2.30 (versions remplacées)

M5Unified 0.2.23 mandates M5GFX ≥ 0.2.30; both must be updated concurrently.

Benefits:

- Microphone `begin()`/`end()`/`record()` calls are serialized, providing a post-start callback required by ES8311 configurations after the I2S clock starts running. The spectrum page frequently stops and restarts the microphone.
- Speaker driver ensures partial audio buffers finish playback, eliminating premature track truncation in music players.
- Introduces `setBufferReleaseCallback`, providing a clean replacement for custom `audioWaitQueueSpace` queue polling.
- Resolves race conditions on shared I2C/SPI buses.

Trade-offs:

- **Screen refresh rate is halved.** Starting with M5GFX 0.2.28 (PR #260), an issue where ESP32-S3 Arduino SPI operated at double the requested frequency was corrected. Cardputer requests 40MHz, so the display has historically run at 80MHz. Upgrading drops this to a true 40MHz, increasing full-screen `pushSprite` (64.8KB) render times from ~6.5ms to ~13ms. All pages are affected, most noticeably direct-push animations like BadApple.
- Microphone recording buffer migrated from double-buffered flipping to a single `rec_info` struct; spectrum page reliance on `record()`/`isRecording()` requires thorough hardware verification.
- Board auto-detection logic was rewritten, specifically altering the internal bus containing BMI270 and ES8311.
- Flash +13KB, RAM negligible change.

### Historique — décision PlatformIO antérieure au 24/11/2026 (remplacée)

Cette conclusion décrivait la base 7.1.x antérieure. Elle est remplacée par PlatformIO 8.0.0, qui fournit Arduino-ESP32 3.3.12 / ESP-IDF 5.5.5. ESP8266Audio 1.9.9 reste conservé uniquement pour isoler les changements et doit être vérifié sous ce nouveau core.

---

## Procedure for Upgrading Dependencies in the Future

1. Sur `main`, modifier une seule dépendance à la fois dans `platformio.ini` (M5Unified et M5GFX doivent être testés ensemble), conformément à `AGENTS.md`.
2. Delete `.pio/libdeps` to force a clean fetch, and verify the resolved version in the Dependency Graph emitted by `pio run`.
3. Record RAM and Flash usage against the baseline above. If static RAM increases by more than 1KB, diagnose root causes before proceeding.
4. Review changelogs and commits between the two versions, paying close attention to display, audio, I2C/SPI, RMT, and power management changes.
5. Après compilation CI, effectuer sur Cardputer ADV les tests de non-régression suivants : boot sequence, spectrum analyzer (microphone), player/radio (speaker), RGB LED modes, Chat (TLS handshake, highest heap demand), GNSS map (48KB tile cache), and BadApple (frame rate). Monitor serial `STAT` for regressions in `heap`, `largest`, and `minEver`.
6. Merge once verified, and update the version table and benchmark data in this document.
