# ROZA Cardputer ADV — firmware boot emulation

This diagnostic branch runs the exact full-flash artifacts produced by GitHub Actions for:

- Known-good reference: `dbc9e79c22bfa6757cd26243d1a7f1dae3dc29c5` (build #104).
- Black-screen report: `f1159af46f1d0eafe6b4976e67f745f00161c8b3` (build #118).

It uses Espressif's `esp-emu` for the ESP32-S3, with external PSRAM explicitly disabled to match the Cardputer ADV.

## What is simulated

- ESP32-S3 CPU, ROM/bootloader path, memory and flash image.
- The firmware's code execution and serial diagnostics.
- I2C transactions can be allowed to ACK using simple register-file devices at the Cardputer ADV addresses used by the keyboard/audio/IMU stack.

## What is not simulated

This is **not** a complete virtual Cardputer ADV and does not render the ST7789V2 screen. The I2C devices are generic ACK/read-back stand-ins, not accurate chip models. The simulator does not reproduce the keyboard's real key matrix, speaker/audio codec behaviour, SD card contents, battery ADC, backlight brightness or LCD pixels.

The test can expose CPU faults, bootloader/app startup failures and the last serial boot stage reached. It cannot certify that the physical screen lights up or that its SPI wiring/timing is correct.

## Reproducible run

Workflow: `.github/workflows/cardputer-emulator.yml`

The workflow downloads the archived full-flash images from the original #104 and #118 Actions runs and executes both with:

- `--chip esp32s3`
- `--psram-size 0`
- I2C stand-ins: port 0, addresses `0x34`, `0x18`, `0x68`
- 25-second emulated runtime per image

Both full-flash images include the bootloader, partition table and application. Their packaging notes state that the NVS area is erased in the image. The simulator therefore starts with a clean NVS state; it does not contain the private settings from the user's physical Cardputer.

Logs and a stage summary are uploaded as the `cardputer-emulator-results` workflow artifact.

## Safety

The workflow only downloads and executes firmware images in the emulator. It never flashes a physical device, never modifies user NVS, and does not change the firmware source on `main`.
