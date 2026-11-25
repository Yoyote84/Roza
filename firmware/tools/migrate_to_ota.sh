#!/usr/bin/env bash
# One-time USB migration from the old factory layout to A/B OTA.
set -euo pipefail
PORT="${1:-}"
PACKAGE_DIR="."
if [[ "$#" -ge 2 ]]; then PACKAGE_DIR="$2"; fi
if [[ -z "$PORT" ]]; then echo "Usage: $0 <serial-port> [package-directory]" >&2; exit 2; fi

for file in bootloader.bin partitions.bin firmware.bin; do
  if [[ ! -s "$PACKAGE_DIR/$file" ]]; then echo "Missing $PACKAGE_DIR/$file" >&2; exit 2; fi
done

if [[ ! -s "$PACKAGE_DIR/SHA256SUMS-migration.txt" ]]; then
  echo "Missing SHA256SUMS-migration.txt; refusing to flash an unverified package." >&2
  exit 2
fi
# Verify the exact migration kit before asking for confirmation or erasing flash.
python - "$PACKAGE_DIR" <<'PY'
import hashlib
import re
import sys
from pathlib import Path

root = Path(sys.argv[1]).resolve()
manifest = root / "SHA256SUMS-migration.txt"
expected = {}
for raw in manifest.read_text(encoding="ascii").splitlines():
    match = re.fullmatch(r"([0-9a-fA-F]{64})  ([A-Za-z0-9._-]+)", raw)
    if not match:
        raise SystemExit("Invalid migration checksum manifest")
    digest, name = match.groups()
    if name in expected:
        raise SystemExit("Duplicate migration checksum entry: " + name)
    expected[name] = digest.lower()
required = {"bootloader.bin", "partitions.bin", "firmware.bin", "migrate_to_ota.sh"}
if set(expected) != required:
    raise SystemExit("Migration checksum manifest does not cover the exact required files")
for name, digest in expected.items():
    path = root / name
    if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        raise SystemExit("Migration package checksum failed: " + name)
print("Migration package SHA-256 checks passed.")
PY

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
