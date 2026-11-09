#!/usr/bin/env python3
"""Create an audit-only manifest for a ROZA ESP-IDF bridge build."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
import subprocess
from pathlib import Path

PINNED = {
    "launcher": "4b06de691df15b0fe6a9f09f56d1d107bbf32b12",
    "nucleoos": "cb0f1a5356ddd6f96ec6b31bbcba5d55f52d8123",
}
FLASH_BYTES = 8 * 1024 * 1024

def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()

def git_head(path: Path) -> str:
    p = subprocess.run(["git", "-C", str(path), "rev-parse", "HEAD"],
                       capture_output=True, text=True)
    if p.returncode:
        raise SystemExit("Impossible de vérifier la révision de " + str(path))
    return p.stdout.strip()

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--build-dir", type=Path, required=True)
    ap.add_argument("--partition-csv", type=Path, required=True)
    ap.add_argument("--output", type=Path, required=True)
    args = ap.parse_args()
    root = Path.cwd().resolve()
    build = args.build_dir.resolve()
    files = {
        "application": build / "roza_nucleoos_launcher_bridge.bin",
        "elf": build / "roza_nucleoos_launcher_bridge.elf",
        "bootloader": build / "bootloader/bootloader.bin",
        "partition_table": build / "partition_table/partition-table.bin",
        "ota_data_initial": build / "ota_data_initial.bin",
        "partition_csv": args.partition_csv.resolve(),
    }
    records = {}
    for name, path in files.items():
        if not path.is_file():
            raise SystemExit("Artefact manquant : " + str(path))
        records[name] = {
            "path": path.relative_to(root).as_posix(),
            "size_bytes": path.stat().st_size,
            "sha256": digest(path),
        }
    rows = []
    for raw in files["partition_csv"].read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line or line.lower().startswith("name,"):
            continue
        cols = [x.strip() for x in line.split(",")]
        if len(cols) < 5:
            raise SystemExit("Ligne de partition invalide : " + raw)
        try:
            offset, size = int(cols[3], 0), int(cols[4], 0)
        except ValueError:
            raise SystemExit("Offset/taille invalide : " + raw)
        if offset < 0 or size <= 0 or offset + size > FLASH_BYTES:
            raise SystemExit("Partition hors des 8 Mio : " + raw)
        rows.append({"name": cols[0], "type": cols[1], "subtype": cols[2],
                     "offset": offset, "size": size, "end": offset + size})
    ordered = sorted(rows, key=lambda x: x["offset"])
    for a, b in zip(ordered, ordered[1:]):
        if a["end"] > b["offset"]:
            raise SystemExit("Chevauchement de partitions : " + a["name"] + "/" + b["name"])
    if not rows or max(x["end"] for x in rows) != FLASH_BYTES:
        raise SystemExit("La table ne couvre pas exactement les 8 Mio.")
    apps = [x for x in rows if x["type"] == "app"]
    if {x["name"] for x in apps} != {"ota_0", "ota_1"}:
        raise SystemExit("Il faut exactement les deux slots OTA attendus.")
    for name, expected in PINNED.items():
        actual = git_head(root / name)
        if actual != expected:
            raise SystemExit(f"Révision {name} inattendue : {actual}")
    capacity = min(x["size"] for x in apps)
    used = records["application"]["size_bytes"]
    if used > capacity:
        raise SystemExit(f"Image ({used}) trop grande pour un slot ({capacity}).")
    manifest = {
        "schema": "roza-engineering-build-manifest/v1",
        "product": "ROZA CHANINA",
        "build_scope": "NucleoOS + surcouche ROZA + deux apps de diagnostic + lien objet Launcher IDF",
        "hardware": {"board": "M5Stack Cardputer ADV", "chip": "ESP32-S3FN8",
                     "flash_bytes": FLASH_BYTES, "psram": False},
        "source": {"roza_commit": os.getenv("GITHUB_SHA", "local"),
                   "roza_ref": os.getenv("GITHUB_REF", "local"),
                   "launcher_commit": git_head(root / "launcher"),
                   "nucleoos_commit": git_head(root / "nucleoos"),
                   "esp_idf": "v5.4.4"},
        "partition_layout": {"rows": ordered, "slot_capacity_bytes": capacity,
                             "application_bytes": used,
                             "free_bytes": capacity-used,
                             "free_percent": round((capacity-used)*100/capacity, 2),
                             "csv": records["partition_csv"]},
        "artifacts": records,
        "release": {"release_ready": False, "safe_to_flash": False,
                    "class": "engineering-diagnostic-only",
                    "blockers": ["port Launcher incomplet",
                                 "migration de partition non validée",
                                 "démarrage/récupération physique non testés",
                                 "canal ROZA signé non configuré"]},
    }
    out = args.output.resolve()
    out.write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+"\n",
                   encoding="utf-8")
    print(json.dumps({"output": str(out), "application_bytes": used,
                      "slot_bytes": capacity, "free_bytes": capacity-used,
                      "free_percent": manifest["partition_layout"]["free_percent"],
                      "sha256": records["application"]["sha256"],
                      "safe_to_flash": False}, ensure_ascii=False, indent=2))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
