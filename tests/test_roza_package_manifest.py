#!/usr/bin/env python3
"""Host tests for package-relative ROZA manifest paths and integrity checks."""
from __future__ import annotations

import copy
import hashlib
import importlib.util
import json
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise SystemExit(f"Impossible de charger {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


generator = load_module("roza_manifest_generator", ROOT / "scripts/create-roza-build-manifest.py")
verifier = load_module("roza_package_verifier", ROOT / "scripts/verify-roza-package-manifest.py")

EXPECTED = {
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
if generator.PACKAGE_PATHS != EXPECTED or verifier.EXPECTED_PATHS != EXPECTED:
    raise SystemExit("Le générateur, le vérificateur et les destinations du paquet divergent.")

with tempfile.TemporaryDirectory(prefix="roza-package-manifest-") as tmp_name:
    root = Path(tmp_name)
    records = {}
    for index, (name, relative) in enumerate(EXPECTED.items()):
        data = b"ROZA-MERGED-IMAGE" if name in {"firmware_candidate", "merged_image_audit"} else f"ROZA:{name}:{index}".encode()
        path = root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        records[name] = {
            "path": relative,
            "build_path": f"ci-workspace/{name}",
            "size_bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
        }

    manifest = {
        "artifacts": records,
        "flash_image": {
            "artifact": records["firmware_candidate"],
            "flash_args": records["flash_args"],
            "sha256": records["firmware_candidate"]["sha256"],
        },
        "partition_layout": {"partition_csv": records["partition_csv"]},
        "release": {
            "class": "complete-software-device-test-candidate",
            "release_ready": False,
            "safe_to_flash": False,
            "hardware_tested": False,
        },
    }
    manifest_path = root / "ROZA-BUILD-INFO.json"

    def write_manifest(value: dict) -> None:
        manifest_path.write_text(json.dumps(value), encoding="utf-8")

    write_manifest(manifest)
    result = verifier.verify_package(root)
    if result["verified_artifacts"] != len(EXPECTED):
        raise SystemExit("Tous les artefacts n'ont pas été vérifiés.")

    installer = root / EXPECTED["installer"]
    original = installer.read_bytes()
    installer.write_bytes(original + b"corruption")
    try:
        verifier.verify_package(root)
    except SystemExit:
        pass
    else:
        raise SystemExit("Un fichier corrompu a passé la vérification.")
    installer.write_bytes(original)

    bad_path = copy.deepcopy(manifest)
    bad_path["artifacts"]["installer"]["path"] = "../flash.sh"
    write_manifest(bad_path)
    try:
        verifier.verify_package(root)
    except SystemExit:
        pass
    else:
        raise SystemExit("Un chemin sortant du paquet a été accepté.")

    bad_release = copy.deepcopy(manifest)
    bad_release["release"]["safe_to_flash"] = True
    write_manifest(bad_release)
    try:
        verifier.verify_package(root)
    except SystemExit:
        pass
    else:
        raise SystemExit("Un candidat non validé a été marqué flashable.")

print("ROZA package manifest: PASS")
print(" - every manifest path resolves inside the ZIP layout")
print(" - file sizes and SHA-256 values are checked")
print(" - missing/corrupt files and path traversal are rejected")
print(" - release remains non-flashable until explicit validation")
