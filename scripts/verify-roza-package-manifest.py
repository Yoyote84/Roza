#!/usr/bin/env python3
"""Verify that every manifest artifact reference resolves inside the assembled ROZA ZIP tree."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath

EXPECTED_PATHS = {
    "firmware_candidate": "ROZA-Cardputer-ADV-FULL.bin",
    "merged_image_audit": "audit/ROZA-ESP32S3-FLASH-PREVIEW.bin",
    "application": "audit/roza_nucleoos_launcher_bridge.bin",
    "elf": "audit/roza_nucleoos_launcher_bridge.elf",
    "bootloader": "audit/bootloader.bin",
    "partition_table": "audit/partition-table.bin",
    "ota_data_initial": "audit/ota_data_initial.bin",
    "flash_args": "flash_args",
    "partition_csv": "audit/partitions.csv",
    "installer": "flash.sh",
    "install_guide": "INSTALL.md",
    "launcher_license": "LICENSE-Launcher-MIT.txt",
    "nucleoos_license": "LICENSE-NucleoOS.txt",
    "radiolib_license": "LICENSE-RadioLib-MIT.txt",
}


def fail(message: str) -> None:
    raise SystemExit("Manifeste ROZA invalide : " + message)


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def package_file(root: Path, relative: object, label: str) -> Path:
    if not isinstance(relative, str) or not relative or "\\" in relative:
        fail(f"chemin absent ou invalide pour {label}")
    rel = PurePosixPath(relative)
    if rel.is_absolute() or ".." in rel.parts or "." in rel.parts:
        fail(f"chemin non relatif au paquet pour {label}: {relative}")
    path = (root / Path(*rel.parts)).resolve()
    try:
        path.relative_to(root)
    except ValueError:
        fail(f"chemin hors du paquet pour {label}: {relative}")
    if not path.is_file():
        fail(f"fichier absent du paquet pour {label}: {relative}")
    return path


def verify_package(package_dir: Path) -> dict:
    root = package_dir.resolve()
    manifest_path = root / "ROZA-BUILD-INFO.json"
    if not manifest_path.is_file():
        fail("ROZA-BUILD-INFO.json absent")
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        fail("JSON illisible: " + str(exc))

    records = manifest.get("artifacts")
    if not isinstance(records, dict) or set(records) != set(EXPECTED_PATHS):
        fail("liste des artefacts différente du contrat du paquet")
    resolved = {}
    for name, expected_path in EXPECTED_PATHS.items():
        record = records.get(name)
        if not isinstance(record, dict):
            fail(f"enregistrement absent: {name}")
        if record.get("path") != expected_path:
            fail(f"chemin de paquet inattendu pour {name}: {record.get('path')!r}")
        path = package_file(root, record.get("path"), name)
        if path.stat().st_size != record.get("size_bytes"):
            fail(f"taille incorrecte pour {name}")
        actual = digest(path)
        if actual.lower() != str(record.get("sha256", "")).lower():
            fail(f"SHA-256 incorrect pour {name}")
        resolved[name] = path

    nested = (
        (manifest.get("flash_image", {}), "artifact", "firmware_candidate"),
        (manifest.get("flash_image", {}), "flash_args", "flash_args"),
        (manifest.get("partition_layout", {}), "partition_csv", "partition_csv"),
    )
    for section, field, record_name in nested:
        if section.get(field) != records[record_name]:
            fail(f"référence imbriquée {field} incohérente avec artifacts.{record_name}")

    image_sha = records["firmware_candidate"]["sha256"]
    if image_sha != records["merged_image_audit"]["sha256"]:
        fail("image publiée différente de la copie d'audit")
    if manifest.get("flash_image", {}).get("sha256") != image_sha:
        fail("SHA-256 de flash_image incohérent")

    release = manifest.get("release", {})
    if release.get("class") != "complete-software-device-test-candidate":
        fail("classe de paquet inattendue")
    for flag in ("release_ready", "safe_to_flash", "hardware_tested"):
        if release.get(flag) is not False:
            fail(f"{flag} doit rester false pour ce candidat non validé")

    return {
        "verified_artifacts": len(resolved),
        "image_bytes": records["firmware_candidate"]["size_bytes"],
        "image_sha256": image_sha,
        "release_ready": False,
        "safe_to_flash": False,
        "hardware_tested": False,
    }


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("package_dir", type=Path)
    args = ap.parse_args()
    print(json.dumps(verify_package(args.package_dir), ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
