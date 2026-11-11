#!/usr/bin/env bash
set -euo pipefail

IMAGE="${1:-ROZA-Cardputer-ADV-FULL.bin}"
MANIFEST="${MANIFEST:-ROZA-BUILD-INFO.json}"
PORT="${PORT:-}"

[[ -f "$IMAGE" ]] || { echo "Image introuvable : $IMAGE" >&2; exit 1; }
[[ -f "$MANIFEST" ]] || { echo "Manifeste introuvable : $MANIFEST" >&2; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "Python 3 est requis." >&2; exit 1; }

python3 - "$IMAGE" "$MANIFEST" <<'PY'
import hashlib, json, pathlib, sys
image = pathlib.Path(sys.argv[1])
manifest = json.loads(pathlib.Path(sys.argv[2]).read_text(encoding="utf-8"))
if manifest.get("hardware", {}).get("chip") != "ESP32-S3FN8":
    raise SystemExit("Manifeste incompatible : cible différente de ESP32-S3FN8.")
if manifest.get("hardware", {}).get("flash_bytes") != 8 * 1024 * 1024:
    raise SystemExit("La capacité flash du manifeste n'est pas 8 Mio.")
release = manifest.get("release", {})
if release.get("class") != "complete-software-device-test-candidate":
    raise SystemExit("Ce paquet n'est pas un candidat ROZA complet de test appareil.")
if release.get("hardware_tested") is not True:
    raise SystemExit("REFUS DE FLASH : le manifeste confirme que le matériel n'a pas été validé.")
if release.get("safe_to_flash") is not True:
    raise SystemExit("REFUS DE FLASH : le manifeste n'autorise pas l'écriture de cette image.")
install = release.get("installation", {})
if install.get("whole_flash_erase") is not False:
    raise SystemExit("Refus : le manifeste n'interdit pas explicitement l'effacement global.")
if install.get("legacy_nvs_backup_required") is not True:
    raise SystemExit("Manifeste sans procédure de sauvegarde NVS.")
if install.get("legacy_nvs_restored_after_contiguous_merge_bin_write") is not True:
    raise SystemExit("Manifeste sans garantie de restauration NVS après image fusionnée.")
if install.get("legacy_nvs_restored_exactly_after_merged_image_write") is not True:
    raise SystemExit("Manifeste sans garantie de restauration exacte de la NVS historique.")
if install.get("legacy_nvs_readback_compared_byte_for_byte") is not True:
    raise SystemExit("Manifeste sans vérification octet par octet de la NVS restaurée.")
if install.get("new_nvs_initialized_empty") is not True:
    raise SystemExit("Manifeste sans garantie d'initialisation de la nouvelle NVS.")
if install.get("otadata_reset") is not True:
    raise SystemExit("Manifeste sans politique explicite de sélection du slot ota_0.")
if install.get("micro_sd_contents_erased") is not False:
    raise SystemExit("Refus : le manifeste n'interdit pas explicitement l'effacement de la microSD.")
expected_erase_regions = ["0x11000-0x11FFF", "0x13000-0x18FFF", "0x19000-0x19FFF"]
if install.get("erase_regions") != expected_erase_regions:
    raise SystemExit("Plages d'effacement du manifeste incompatibles avec l'installateur.")
flash = manifest.get("flash_image", {})
if flash.get("write_offset") != "0x0":
    raise SystemExit("Offset d'écriture inattendu.")
expected = flash.get("sha256")
actual = hashlib.sha256(image.read_bytes()).hexdigest()
if not expected or actual.lower() != expected.lower():
    raise SystemExit("SHA-256 de l'image invalide : aucun flash n'a été effectué.")
print("SHA-256 image vérifié :", actual)
PY

python3 -m esptool --help >/dev/null 2>&1 || {
  echo "Installez esptool : python3 -m pip install esptool" >&2
  exit 1
}

if [[ -z "$PORT" ]]; then
  read -r -p "Port série du Cardputer (ex. /dev/ttyACM0) : " PORT
fi
[[ -n "$PORT" ]] || { echo "Aucun port série indiqué." >&2; exit 1; }

echo
echo "ROZA / РO3A — installation initiale de test sur Cardputer ADV"
echo "Ce firmware n'a pas encore été validé sur un appareil physique."
echo "Sauvegardez aussi le contenu de la microSD sur un autre support."
echo "L'antenne RP-SMA doit être vissée au Cap LoRa-1262 AVANT la mise sous tension."
echo
echo "Plan mémoire :"
echo " - ancienne NVS Launcher : sauvegardée, restaurée après l'image et vérifiée"
echo " - nouvelle NVS NucleoOS : initialisée à neuf"
echo " - otadata : réinitialisée pour démarrer ota_0"
echo " - aucune commande erase_flash globale"
echo
read -r -p "Tapez ROZA-TEST pour confirmer que vous acceptez ce premier essai : " CONFIRM
[[ "$CONFIRM" == "ROZA-TEST" ]] || { echo "Installation annulée."; exit 1; }

BACKUP="ROZA-LAUNCHER-NVS-$(date +%Y%m%d-%H%M%S).bin"
python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 \
  read-flash 0x9000 0x5000 "$BACKUP"
sha256sum "$BACKUP" > "$BACKUP.sha256"
echo "Ancienne NVS sauvegardée : $BACKUP"
cat "$BACKUP.sha256"

# Erase only the sectors that will become new ROZA/NucleoOS data.
# The legacy Launcher NVS range is restored from the backup after merge-bin writes gaps.
python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 erase-region 0x11000 0x1000
python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 erase-region 0x13000 0x6000
python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 erase-region 0x19000 0x1000

python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 write-flash \
  --flash-mode dio --flash-size 8MB --flash-freq 80m 0x0 "$IMAGE"

# merge-bin fills address gaps with 0xFF; restore old Launcher NVS afterward.
python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 write-flash \
  --flash-mode dio --flash-size 8MB --flash-freq 80m 0x9000 "$BACKUP"

VERIFY="$BACKUP.verify"
python3 -m esptool --chip esp32s3 --port "$PORT" --baud 460800 \
  read-flash 0x9000 0x5000 "$VERIFY"
if ! cmp -s "$BACKUP" "$VERIFY"; then
  echo "ERREUR : la NVS historique n'a pas ete restauree a l'identique." >&2
  echo "Sauvegarde PC conservee : $BACKUP" >&2
  exit 1
fi
rm -f "$VERIFY"
echo "NVS historique restauree et verifiee octet par octet."

echo
echo "Écriture terminée. Le premier démarrage est un essai matériel."
echo "Conservez $BACKUP et son .sha256 jusqu'à validation complète."
