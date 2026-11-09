# ROZA — NucleoOS + Launcher ESP-IDF integration prototype

This project is a **compile/link integration test**, not a release firmware.

## What it builds

- NucleoOS's pinned, real \`firmware/main/main.c\` as the one and only \`app_main()\`, after applying the idempotent ROZA overlay to its default theme and boot identity.
- The actual NucleoOS ESP-IDF component tree from the pinned \`nucleoos\` submodule.
- Launcher 3.0.0's real \`launcher/src/idf/launcher_platform.cpp\`, through the same IDF component used by the first bridge test; \`WHOLE_ARCHIVE\` forces that object into the final link so unresolved Arduino/platform dependencies cannot remain hidden in an unused static archive.
- The first user-facing Launcher feature adapted to the NucleoOS native-app lifecycle: a ROZA **Partitions** app, registered in the same launcher menu. It enumerates the active ESP-IDF partition table and displays label, type/subtype, flash offset, size and encryption status. It uses the NucleoOS display/keyboard owner and only reads the partition metadata.
- Arduino-ESP32 3.2.1 as an ESP-IDF-managed component, with ESP-IDF 5.4.4 (the version locked by NucleoOS). Arduino autostart is explicitly disabled; NucleoOS remains the sole \`app_main()\` owner. Arduino-ESP32 3.2.1 requires \`CONFIG_FREERTOS_HZ=1000\`, so the ROZA bridge profile selects a 1 ms FreeRTOS tick without changing the pinned NucleoOS repository defaults. Its manifest also brings in RainMaker provisioning sources on ESP32-S3; this bridge profile enables the real upstream mbedTLS PEM-writing API those sources compile against, while keeping CSR parsing disabled. The bridge profile also enables IPv6 because Arduino-ESP32 3.2.1's upstream AsyncUDP implementation accesses \`ip_addr_t.u_addr.ip4\`, a member not present in ESP-IDF's IPv4-only \`ip_addr_t\` type. This IPv6 setting is isolated to the integration harness; it does not change NucleoOS defaults and its additional RAM/flash cost must be measured before production use on the PSRAM-less Cardputer ADV.

The new ROZA app is a **read-only first slice**, not the complete Launcher partition manager. It intentionally offers no partition-table writes, erase, resize, migration, backup/restore or OTA-slot changes. Those operations need their own data-preservation design and review.

No substitute implementation is introduced. The main component directly compiles the upstream NucleoOS \`main.c\`; the Launcher platform component directly compiles the upstream Launcher source. The read-only app is a native ROZA adaptation using ESP-IDF's partition iterator, not a claim that Launcher's Arduino UI has been ported wholesale.

## Shared USB Mass Storage ownership

NucleoOS enables \`CONFIG_TINYUSB_MSC_ENABLED\` for its real \`esp_tinyusb\` driver. Arduino-ESP32 3.2.1 sees the same Kconfig symbol and otherwise compiles \`cores/esp32/USBMSC.cpp\`, even though that source also requires Arduino's separate \`CONFIG_TINYUSB_ENABLED\` HAL. This caused missing declarations such as \`tinyusb_add_string_descriptor\`, \`TUD_MSC_DESCRIPTOR\` and \`tud_msc_set_sense\`.

The build now runs \`idf.py reconfigure\`, then \`scripts/prepare-roza-arduino-usb.py\`, then \`idf.py build\`. The guard adds a CMake condition to the downloaded Arduino component so its own \`USBMSC.cpp\` is built only when the Arduino TinyUSB HAL is enabled. In this bridge, NucleoOS remains the sole real USB-MSC implementation through \`nucleo_usbmsc\` + \`esp_tinyusb\`; no NucleoOS USB source or configuration is disabled. Before applying the guard, the script verifies the NucleoOS MSC implementation and checks that Launcher sources do not reference Arduino's \`USBMSC\` class. It fails closed if those assumptions change. This is a build-only integration guard, not a physical USB validation.

## Arduino FatFs compatibility (fixed 512-byte sectors)

The combined CI next reached Arduino's real SD.cpp and SD_MMC.cpp, where both referenced FATFS.ssize and failed because the NucleoOS bridge builds FatFs in fixed-sector mode. In ESP-IDF 5.4.4, FATFS.ssize exists only when FF_MAX_SS differs from FF_MIN_SS. This profile has FF_SS_SDCARD=512 and explicitly pins CONFIG_WL_SECTOR_SIZE_512=y, so the minimum and maximum sector sizes are both 512 and that member is intentionally absent.

The preparation script now adds the legacy Arduino compile definition _MAX_SS=512 only to the downloaded Arduino component target, and fails closed if the fixed 512-byte wear-levelling setting is no longer enabled. This makes Arduino 3.2.1 select the fixed-sector branch (multiplication by 512) rather than referencing the nonexistent ssize field. It does not alter FatFs, NucleoOS source, or the SD card's actual sector size. A passing compile is still required to validate the compatibility change.

## Pinned sources

- NucleoOS: \`indecenti/NucleoOs\`, commit \`cb0f1a5356ddd6f96ec6b31bbcba5d55f52d8123\`.
- Launcher: \`bmorcelli/Launcher\`, commit \`4b06de691df15b0fe6a9f09f56d1d107bbf32b12\`.

The NucleoOS submodule retains its upstream \`LICENSE\` (PolyForm Noncommercial 1.0.0) and notices. Keep those terms and every third-party license when redistributing or modifying the integrated firmware.

## Build in CI or locally

Use ESP-IDF v5.4.4 with the ESP32-S3 target.

\`\`\`sh
git clone --recurse-submodules https://github.com/Yoyote84/Roza.git
cd Roza
git checkout migration/roza-chanina-launcher
git submodule update --init --recursive
cd integration/nucleoos-bridge
idf.py set-target esp32s3
idf.py reconfigure
python3 ../../scripts/prepare-roza-arduino-usb.py
idf.py build
\`\`\`

The harness carries copies of NucleoOS's \`dependencies.lock\`, \`sdkconfig.defaults\` and \`partitions.csv\` from the pinned source revision, with the bridge-only 1 kHz tick requirement recorded explicitly in its own sdkconfig. The CI sparse checkout includes both \`firmware/\` and \`sdk/native/\`, because NucleoOS's \`nucleo_native\` component includes the native SDK header from outside \`firmware/\`. It does not flash the device.

## What a green build proves — and what it does not

A green build proves that the current NucleoOS firmware component graph, the selected real Launcher platform component and the new native ROZA Partitions app compile and link together under the same ESP-IDF project and dependency resolver, with one NucleoOS \`app_main()\`.

It does **not** yet mean that Launcher 3.0.0's complete application/UI, catalogue, OTA, SD installer, WebUI, USB SD mode, writable partition manager and all third-party libraries have been merged into ROZA. It also does not prove runtime display/keyboard behavior, available heap/flash headroom, safe partition migration, first-boot behavior, or physical Cardputer ADV operation. Those remain separate integration and hardware-validation gates.

Never use this test build as a release image. Do not flash it to a device or erase flash as part of this workflow.


## ROZA firmware update safety gate

The pinned NucleoOS updater targets NucleoOS releases, not the unified ROZA firmware. The ROZA overlay therefore disables that upstream firmware-update path until a dedicated signed ROZA release channel exists. Automatic/manual checks do not contact the NucleoOS release feed, stale update tags are ignored, a stale pending-install flag is consumed, and the install worker fails before any OTA-slot erase, download or boot-partition change. The UI identifies the ROZA update channel as pending. The native app Store catalog refresh remains available; only firmware OTA is gated.

This is not yet a working ROZA OTA implementation. A release channel must provide a complete signed ROZA image and matching metadata, and must pass partition-layout, SHA-256/signature, rollback and device compatibility checks before being enabled. Do not flash this integration prototype.
