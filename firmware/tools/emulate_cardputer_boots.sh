#!/usr/bin/env bash
set -uo pipefail

OUT_DIR="${1:-emulator-results}"
IMAGE_104="${2:?usage: emulate_cardputer_boots.sh OUT_DIR IMAGE_104 IMAGE_118}"
IMAGE_118="${3:?usage: emulate_cardputer_boots.sh OUT_DIR IMAGE_104 IMAGE_118}"

mkdir -p "$OUT_DIR"
SUMMARY="$OUT_DIR/summary.txt"
: > "$SUMMARY"
FOUND_PANIC=0

run_one() {
  local label="$1"
  local image="$2"
  local log="$OUT_DIR/$label.log"
  local rc=0
  local state="UNKNOWN"

  if [[ ! -s "$image" ]]; then
    printf '%s: missing/empty image: %s\n' "$label" "$image" | tee -a "$SUMMARY"
    return 0
  fi

  {
    echo "=== $label ==="
    echo "Image: $image"
    echo "Bytes: $(wc -c < "$image")"
    echo "Target: ESP32-S3 / 8 MiB flash / PSRAM disabled"
    echo "I2C register-file devices: 0x34 (TCA8418), 0x18 (ES8311), 0x68 (BMI270)"
    echo
  } > "$log"

  set +e
  RUST_LOG=info esp-emu \
    --chip esp32s3 \
    --firmware "$image" \
    --psram-size 0 \
    --i2c-dev 0:0x34,fill=0x00 \
    --i2c-dev 0:0x18,fill=0x00 \
    --i2c-dev 0:0x68,fill=0x00 \
    --timeout 25s \
    >> "$log" 2>&1
  rc=$?
  set -e

  if grep -Eqi 'Guru Meditation|panic(ed)?|abort\(\)|assert failed|LoadProhibited|StoreProhibited|InstrFetchProhibited|Stack canary|Unhandled exception|CPU exception' "$log"; then
    state="PANIC_OR_EXCEPTION"
    FOUND_PANIC=1
  elif grep -Fq '[boot-diag] post-complete' "$log"; then
    state="POST_COMPLETE"
  elif grep -Fq '[boot-diag] boot-animation-done' "$log"; then
    state="BOOT_ANIMATION_COMPLETE_POST_NOT_CONFIRMED"
  elif grep -Fq '[boot-diag] m5-begin-done' "$log"; then
    state="M5_BEGIN_REACHED"
  elif grep -Fq '[boot-diag] setup-enter' "$log"; then
    state="EARLY_SETUP_REACHED"
  elif grep -Fq '[boot] gnss io=' "$log"; then
    state="REACHED_GNSS_INIT_NO_PANIC_OBSERVED"
  elif grep -Fq '[boot] sd mounted=' "$log"; then
    state="REACHED_SD_INIT_NO_PANIC_OBSERVED"
  elif [[ "$rc" -ne 0 ]]; then
    state="EMULATOR_OR_IMAGE_ERROR"
  else
    state="NO_SERIAL_MARKER_OBSERVED"
  fi

  {
    echo "$label: $state (esp-emu exit=$rc)"
    echo "  boot-diag markers:"
    grep -oE '\[boot-diag\] [^|]+' "$log" | sed 's/^/    /' || true
    echo "  ROM/app/error lines:"
    grep -Ei 'ESP-ROM|rst:|boot:|panic|Guru Meditation|assert failed|exception|error:|error ' "$log" | tail -n 20 | sed 's/^/    /' || true
  } | tee -a "$SUMMARY"
}

set -e
run_one "firmware-104" "$IMAGE_104"
run_one "firmware-118" "$IMAGE_118"

echo
echo "Interpretation limits:" | tee -a "$SUMMARY"
echo "- This is a CPU/boot smoke test, not a rendered LCD simulation." | tee -a "$SUMMARY"
echo "- I2C devices are ACK/register-file stand-ins, not faithful TCA8418/ES8311/BMI270 models." | tee -a "$SUMMARY"
echo "- SD card, keyboard events, audio output, battery ADC and ST7789 pixels are not physically reproduced." | tee -a "$SUMMARY"
echo "- A clean emulator run cannot prove the real Cardputer display/backlight works." | tee -a "$SUMMARY"

if [[ "$FOUND_PANIC" -ne 0 ]]; then
  exit 1
fi
