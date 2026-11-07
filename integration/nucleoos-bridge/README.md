# ROZA — NucleoOS + Launcher ESP-IDF integration prototype

This project is a **compile/link integration test**, not a release firmware.

## What it builds

- NucleoOS's pinned, real `firmware/main/main.c` as the one and only `app_main()`.
- The actual NucleoOS ESP-IDF component tree from the pinned `nucleoos` submodule.
- Launcher 3.0.0's real `launcher/src/idf/launcher_platform.cpp`, through the same IDF component used by the first bridge test.
- Arduino-ESP32 3.2.1 as an ESP-IDF-managed component, with ESP-IDF 5.4.4 (the version locked by NucleoOS).

No substitute implementation is introduced. The main component directly compiles the upstream NucleoOS `main.c`; the Launcher component directly compiles the upstream Launcher source.

## Pinned sources

- NucleoOS: `indecenti/NucleoOs`, commit `cb0f1a5356ddd6f96ec6b31bbcba5d55f52d8123`.
- Launcher: `bmorcelli/Launcher`, commit `4b06de691df15b0fe6a9f09f56d1d107bbf32b12`.

The NucleoOS submodule retains its upstream `LICENSE` (PolyForm Noncommercial 1.0.0) and notices. Keep those terms and every third-party license when redistributing or modifying the integrated firmware.

## Build in CI or locally

Use ESP-IDF v5.4.4 with the ESP32-S3 target.

```sh
git clone --recurse-submodules https://github.com/Yoyote84/Roza.git
cd Roza
git checkout migration/roza-chanina-launcher
git submodule update --init --recursive
cd integration/nucleoos-bridge
idf.py set-target esp32s3
idf.py build
```

The harness carries copies of NucleoOS's `dependencies.lock`, `sdkconfig.defaults` and `partitions.csv` from the pinned source revision. It does not flash the device.

## What a green build proves — and what it does not

A green build proves that the current NucleoOS firmware component graph and the selected real Launcher platform component can compile and link together under the same ESP-IDF project and dependency resolver, with one NucleoOS `app_main()`.

It does **not** yet mean that Launcher 3.0.0's complete application/UI, catalogue, OTA, SD installer, WebUI, USB SD mode, partition manager and all third-party libraries have been merged into ROZA. It also does not prove runtime coexistence, available heap/flash headroom, safe partition migration, first-boot behavior, or physical Cardputer ADV operation. Those remain separate integration and hardware-validation gates.

Never use this test build as a release image. Do not flash it to a device or erase flash as part of this workflow.
