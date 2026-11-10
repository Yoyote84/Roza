#!/usr/bin/env bash
set -euo pipefail

IMAGE="${1:-ROZA-Cardputer-ADV-FULL.bin}"
MANIFEST="${MANIFEST:-ROZA-BUILD-INFO.json}"
PORT="${PORT:-}"

[[ -f "$IMAGE" ]] || { echo "Image introuvable : $IMAGE" >&2; exit 1; }
[[ -f "$MANIFEST" ]] || { echo "Manifeste introuvable : $MANIFEST" >&2; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "Python 3 est requis." >&2; exit 1; }
python3 -m esptool --help >/dev/null 2>&1 || {
  echo "Installez esptool : python3 -m pip install esptool" >&2
  exit 1
}

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
if release.get("installation", {}).get("legacy_nvs_backup_required") is not True:
    raise SystemExit("Manifeste sans procédure de sauvegarde NVS.")
if release.get("installation", {}).get("legacy_nvs_restored_after_contiguous_merge_bin_write") is not True:
    raise SystemExit("Manifeste sans garantie de restauration NVS après image fusionnée.")
if release.get("safe_to_flash") is not False or release.get("hardware_tested") is not False:
    raise SystemExit("Le statut de validation matériel du manifeste est incohérent.")
flash = manifest.get("flash_image", {})
if flash.get("write_offset") != "0x0":
    raise SystemExit("Offset d'écriture inattendu.")
expected = flash.get("sha256")
actual = hashlib.sha256(image.read_bytes()).hexdigest()
if not expected or actual.lower() != expected.lower():
    raise SystemExit("SHA-256 de l'image invalide : aucun flash n'a été effectué.")
if release.get("installation", {}).get("legacy_nvs_backup_required") is not True:
    raise SystemExit("Manifeste sans procédure de sauvegarde NVS.")
print("SHA-256 image vérifié :", actual)
PY

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
