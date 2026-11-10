#!/usr/bin/env python3
"""Fail-closed validation of the ESP-IDF merged flash preview for ROZA."""
from __future__ import annotations

import argparse
import hashlib
import json
import shlex
from pathlib import Path

FLASH_BYTES = 8 * 1024 * 1024
EXPECTED_FILES = {
    "bootloader/bootloader.bin": 0x000000,
    "partition_table/partition-table.bin": 0x008000,
    "ota_data_initial.bin": 0x00F000,
    "roza_nucleoos_launcher_bridge.bin": 0x020000,
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def fail(message: str) -> None:
    raise SystemExit("Validation image flash ROZA : ECHEC — " + message)


def resolve_flash_file(build: Path, raw_path: str) -> Path:
    candidate = Path(raw_path)
    attempts = [candidate] if candidate.is_absolute() else [build / candidate]
    if not candidate.is_absolute() and candidate.parts and candidate.parts[0] == build.name:
        attempts.append(build.parent / candidate)
    for attempt in attempts:
        resolved = attempt.resolve()
        try:
            resolved.relative_to(build)
        except ValueError:
            continue
        if resolved.is_file():
            return resolved
    fail("fichier référence par flash_args absent ou hors du build : " + raw_path)


def parse_flash_args(path: Path, build: Path) -> tuple[dict[str, tuple[int, Path]], str]:
    tokens = shlex.split(path.read_text(encoding="utf-8"))
    entries: dict[str, tuple[int, Path]] = {}
    flash_size = ""
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if token.startswith("--"):
            option = token.replace("_", "-").lower()
            if index + 1 < len(tokens) and not tokens[index + 1].startswith("--"):
                value = tokens[index + 1]
                if option == "--flash-size":
                    flash_size = value
                index += 2
            else:
                index += 1
            continue
        try:
            offset = int(token, 0)
        except ValueError:
            fail("argument inattendu dans flash_args : " + token)
        if index + 1 >= len(tokens):
            fail("offset sans fichier dans flash_args : " + token)
        file_path = resolve_flash_file(build, tokens[index + 1])
        relative = file_path.relative_to(build).as_posix()
        if relative in entries:
            fail("fichier flash dupliqué : " + relative)
        entries[relative] = (offset, file_path)
        index += 2
    return entries, flash_size


def read_partitions(path: Path) -> dict[str, tuple[int, int]]:
    rows: dict[str, tuple[int, int]] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line or line.lower().startswith("name,"):
            continue
        cols = [item.strip() for item in line.split(",")]
        if len(cols) < 5:
            fail("ligne de partition invalide : " + raw)
        try:
            rows[cols[0]] = (int(cols[3], 0), int(cols[4], 0))
        except ValueError:
            fail("offset/taille de partition invalide : " + raw)
    return rows


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--image", type=Path, required=True,
                        help="Nom du fichier fusionné relatif au dossier build.")
    parser.add_argument("--flash-args", type=Path, required=True)
    parser.add_argument("--partition-csv", type=Path, required=True)
    args = parser.parse_args()

    build = args.build_dir.resolve()
    image_path = (build / args.image).resolve()
    if not image_path.is_file():
        fail("image fusionnée introuvable : " + str(image_path))
    if image_path.stat().st_size <= 0 or image_path.stat().st_size > FLASH_BYTES:
        fail("taille de l'image hors limite 8 Mio")
    flash_args = args.flash_args.resolve()
    if not flash_args.is_file():
        fail("flash_args absent")

    entries, flash_size = parse_flash_args(flash_args, build)
    if flash_size.upper() != "8MB":
        fail("flash_args ne cible pas explicitement 8MB : " + (flash_size or "absent"))
    expected = set(EXPECTED_FILES)
    if set(entries) != expected:
        fail("fichiers/offsets flash inattendus : " + repr(entries.keys()))

    partitions = read_partitions(args.partition_csv.resolve())
    for label in ("ota_0", "ota_1"):
        if label not in partitions:
            fail("slot OTA manquant : " + label)
    app_offset, app_size = partitions["ota_0"]
    if app_offset != EXPECTED_FILES["roza_nucleoos_launcher_bridge.bin"]:
        fail("l'application n'est pas placée au début de ota_0")
    if partitions["ota_1"][1] != app_size:
        fail("les slots OTA n'ont pas la même capacité")

    max_end = 0
    verified = []
    image_bytes = image_path.read_bytes()
    for relative, expected_offset in EXPECTED_FILES.items():
        actual_offset, source_path = entries[relative]
        if actual_offset != expected_offset:
            fail(f"{relative}: offset {actual_offset:#x}, attendu {expected_offset:#x}")
        payload = source_path.read_bytes()
        end = actual_offset + len(payload)
        if end > FLASH_BYTES:
            fail(relative + " dépasse la flash de 8 Mio")
        if relative == "roza_nucleoos_launcher_bridge.bin" and len(payload) > app_size:
            fail("l'application dépasse ota_0")
        if end > len(image_bytes):
            fail("image fusionnée tronquée avant " + relative)
        if image_bytes[actual_offset:end] != payload:
            fail("contenu fusionné différent de la source : " + relative)
        max_end = max(max_end, end)
        verified.append({
            "file": relative,
            "offset": f"0x{actual_offset:X}",
            "size_bytes": len(payload),
            "sha256": sha256(source_path),
        })

    if len(image_bytes) != max_end:
        fail(f"taille fusionnée {len(image_bytes)} différente de la fin réelle {max_end}")
    print(json.dumps({
        "status": "PASS",
        "chip": "esp32s3",
        "flash_bytes": FLASH_BYTES,
        "merged_image": image_path.name,
        "merged_size_bytes": len(image_bytes),
        "merged_sha256": sha256(image_path),
        "write_offset": "0x0",
        "verified_components": verified,
        "safe_to_flash": False,
        "reason": "la migration de partitions et la validation matérielle ne sont pas approuvées",
    }, ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
