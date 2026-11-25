#!/usr/bin/env bash
# One-time USB migration from the old factory layout to A/B OTA.
set -euo pipefail
PORT="$1"
PACKAGE_DIR="."
if [[ "$#" -ge 2 ]]; then PACKAGE_DIR="$2"; fi
if [[ -z "$PORT" ]]; then echo "Usage: $0 <serial-port> [package-directory]" >&2; exit 2; fi

for file in bootloader.bin partitions.bin firmware.bin; do
  if [[ ! -s "$PACKAGE_DIR/$file" ]]; then echo "Missing $PACKAGE_DIR/$file" >&2; exit 2; fi
done

echo "ROZA A/B migration: Cardputer ADV ESP32-S3, 8 MiB"
echo "NVS (0x9000..0xDFFF) and PHY (0xE000..0xEFFF) are preserved."
echo "Only otadata (0xF000..0x10FFF) is erased; it overlaps the old factory app."
echo "Do not unplug USB or remove power during flashing."
read -r -p "Type MIGRATE-ROZA to continue: " confirmation
if [[ "$confirmation" != "MIGRATE-ROZA" ]]; then echo "Cancelled; no flash command was run."; exit 1; fi

# Deliberately no chip erase: existing NVS settings remain intact.
python -m esptool --chip esp32s3 --port "$PORT" erase-region 0xF000 0x2000
python -m esptool --chip esp32s3 --port "$PORT" write-flash \
  0x0000 "$PACKAGE_DIR/bootloader.bin" \
  0x8000 "$PACKAGE_DIR/partitions.bin" \
  0x20000 "$PACKAGE_DIR/firmware.bin"
echo "Migration finished. Reboot and verify normal ROZA startup."
