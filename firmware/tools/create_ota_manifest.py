#!/usr/bin/env python3
"""Create a signed ROZA OTA manifest (ECDSA P-256 / SHA-256)."""
from __future__ import annotations
import argparse, base64, hashlib, json, re, subprocess, tempfile
from pathlib import Path

DEFAULT_PUBLIC_KEY_HEADER = Path(__file__).resolve().parents[1] / "src" / "ota_public_key.h"

OTA_URL = "https://yoyote84.github.io/Roza/downloads/ROZA-OTA.bin"

def canonical_payload(build: int, commit: str, size: int, digest: str, url: str = OTA_URL) -> bytes:
    return ("ROZA-OTA-MANIFEST-V1\n"
            f"cardputer-adv\n{build}\n{commit}\n{size}\n{digest}\n{url}\n").encode("ascii")

def embedded_public_key_pem(header: Path) -> str:
    """Extract the exact PEM embedded in ota_public_key.h."""
    source = header.read_text(encoding="utf-8")
    block = re.search(r"ROZA_OTA_PUBLIC_KEY_PEM\s*\[\s*\]\s*=\s*(.*?);", source, re.S)
    if block is None:
        raise ValueError("embedded OTA public key declaration is missing")
    fragments = re.findall(r'"((?:\\.|[^"\\])*)"', block.group(1))
    if not fragments:
        raise ValueError("embedded OTA public key PEM is empty")
    return "".join(bytes(item, "utf-8").decode("unicode_escape") for item in fragments)


def verify_signing_key_matches_public(private_key: Path, public_key_header: Path) -> None:
    """Fail before publishing when CI's private key is not the firmware's key pair."""
    pem = embedded_public_key_pem(public_key_header)
    with tempfile.TemporaryDirectory(prefix="roza-ota-key-check-") as temp:
        root = Path(temp)
        embedded = root / "embedded-public.pem"
        derived_der = root / "derived-public.der"
        embedded_der = root / "embedded-public.der"
        embedded.write_text(pem, encoding="ascii")
        subprocess.run(["openssl", "pkey", "-in", str(private_key), "-pubout",
                        "-outform", "DER", "-out", str(derived_der)],
                       check=True, capture_output=True, text=True)
        subprocess.run(["openssl", "pkey", "-pubin", "-in", str(embedded),
                        "-outform", "DER", "-out", str(embedded_der)],
                       check=True, capture_output=True, text=True)
        if derived_der.read_bytes() != embedded_der.read_bytes():
            raise ValueError("La cle privee OTA ne correspond pas a la cle publique embarquee")

def make_manifest(firmware: Path, private_key: Path, build: int, commit: str,
                  public_key_header: Path = DEFAULT_PUBLIC_KEY_HEADER) -> dict:
    if build < 1:
        raise ValueError("build number must be positive")
    if not re.fullmatch(r"[0-9a-fA-F]{40}", commit):
        raise ValueError("commit must be a 40-character Git SHA")
    if not firmware.is_file() or firmware.stat().st_size < 4096:
        raise ValueError("firmware is missing or too small")
    if not private_key.is_file():
        raise ValueError("private signing key is missing")
    verify_signing_key_matches_public(private_key, public_key_header)
    digest = hashlib.sha256(firmware.read_bytes()).hexdigest()
    size = firmware.stat().st_size
    payload = canonical_payload(build, commit.lower(), size, digest)
    with tempfile.TemporaryDirectory(prefix="roza-ota-sign-") as temp:
        source = Path(temp) / "manifest.payload"
        signature = Path(temp) / "manifest.der"
        source.write_bytes(payload)
        subprocess.run(
            ["openssl", "dgst", "-sha256", "-sign", str(private_key),
             "-out", str(signature), str(source)],
            check=True, capture_output=True, text=True,
        )
        encoded = base64.b64encode(signature.read_bytes()).decode("ascii")
    return {
        "schema": 1, "board": "cardputer-adv", "build": build,
        "commit": commit.lower(), "size": size, "sha256": digest,
        "url": OTA_URL, "signature": encoded,
    }

def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--firmware", required=True, type=Path)
    parser.add_argument("--private-key", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--build-number", required=True, type=int)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--public-key-header", type=Path, default=DEFAULT_PUBLIC_KEY_HEADER)
    args = parser.parse_args()
    manifest = make_manifest(args.firmware, args.private_key, args.build_number, args.commit,
                             public_key_header=args.public_key_header)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(manifest, ensure_ascii=True, indent=2) + "\n", encoding="utf-8")
    print("Signed OTA manifest created for build %d" % args.build_number)

if __name__ == "__main__":
    main()
