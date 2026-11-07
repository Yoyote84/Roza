# ROZA — Arduino / ESP-IDF compatibility smoke test

This is a **compile/link prototype**, not a flashable firmware and not the final ROZA architecture.

It builds the real `launcher/src/idf/launcher_platform.cpp` from the pinned Launcher 3.0.0 submodule as an ESP-IDF component. It adds Espressif Arduino-ESP32 **3.2.1** as an IDF-managed component and uses the same ESP-IDF **5.4.4** release pinned by NucleoOS.

Why these versions:

- ROZA's current Launcher PlatformIO platform is `pioarduino/platform-espressif32 55.03.39`, which is based on Arduino-ESP32 3.3.9 / ESP-IDF 5.5.4.
- NucleoOS currently pins ESP-IDF 5.4.4 in `firmware/dependencies.lock`.
- Arduino-ESP32 3.2.1 declares support for ESP-IDF `>=5.3,<5.5`, so it is the compatible candidate for this isolated test.

The test's `app_main()` explicitly calls `initArduino()`, then invokes the actual Launcher platform wrappers for serial output, time, delay, random and safe no-op GPIO calls. The automatic Arduino `setup()/loop()` startup remains disabled because a unified ROZA firmware must keep one owner for `app_main()`.

## Run in CI

The workflow `.github/workflows/roza-arduino-bridge.yml` builds this project with ESP-IDF v5.4.4 for `esp32s3`.

## Important limits

A green result proves only that this Arduino component and this real Launcher platform source can compile and link together under the NucleoOS ESP-IDF version. It does **not** prove the entire Launcher source tree, its display/input drivers, all PlatformIO libraries, NucleoOS runtime coexistence, memory headroom, device boot, or any physical Cardputer ADV behavior.

No device is flashed by this test. Do not use the generated build output as a firmware release.
