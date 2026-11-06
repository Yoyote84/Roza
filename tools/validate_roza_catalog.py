#!/usr/bin/env python3
"""Validate the public ROZA app catalog and every package hash."""
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "roza-app-catalog" / "catalog.json"
ID_RE = re.compile(r"^[a-z0-9_-]{1,24}$")
FILE_RE = re.compile(r"^[A-Za-z0-9_.-]{1,23}$")


def fail(message: str) -> None:
    raise SystemExit(f"ROZA catalog validation failed: {message}")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(8192), b""):
            digest.update(block)
    return digest.hexdigest()


data = json.loads(CATALOG.read_text(encoding="utf-8"))
if data.get("format") != "roza-app-catalog" or data.get("format_version") != 1:
    fail("unsupported catalog format")
apps = data.get("apps")
if not isinstance(apps, list) or not 1 <= len(apps) <= 12:
    fail("apps must contain between 1 and 12 entries")

seen = set()
for app in apps:
    app_id = app.get("id", "")
    entry = app.get("entry", "")
    if not ID_RE.fullmatch(app_id) or app_id in seen:
        fail(f"invalid or duplicate id: {app_id!r}")
    seen.add(app_id)
    if not FILE_RE.fullmatch(entry) or entry.startswith(".") or ".." in entry or entry == "manifest.json":
        fail(f"unsafe entry filename for {app_id}")
    if app.get("runtime") != "roza-lua":
        fail(f"unsupported runtime for {app_id}")

    app_dir = CATALOG.parent / "apps" / app_id
    manifest_path = app_dir / "manifest.json"
    entry_path = app_dir / entry
    if not manifest_path.is_file() or not entry_path.is_file():
        fail(f"package files missing for {app_id}")

    manifest_bytes = manifest_path.read_bytes()
    entry_bytes = entry_path.read_bytes()
    if len(manifest_bytes) != app.get("manifest_size"):
        fail(f"manifest size mismatch for {app_id}")
    if len(entry_bytes) != app.get("entry_size"):
        fail(f"entry size mismatch for {app_id}")
    if sha256(manifest_path) != app.get("manifest_sha256"):
        fail(f"manifest SHA-256 mismatch for {app_id}")
    if sha256(entry_path) != app.get("entry_sha256"):
        fail(f"entry SHA-256 mismatch for {app_id}")

    manifest_data = json.loads(manifest_bytes.decode("utf-8"))
    expected = {
        "format": "roza-app",
        "format_version": 1,
        "id": app.get("id"),
        "name": app.get("name"),
        "version": app.get("version"),
        "description": app.get("description"),
        "runtime": app.get("runtime"),
        "entry": app.get("entry"),
    }
    for key, value in expected.items():
        if manifest_data.get(key) != value:
            fail(f"manifest/catalog mismatch for {app_id}: {key}")

print(f"ROZA catalog OK: {len(apps)} package(s), hashes verified")
