#!/usr/bin/env python3
"""Offline tests for ECDSA OTA signatures, anti-replay metadata and A/B partition layout."""
from __future__ import annotations
import base64, subprocess, tempfile, unittest
from pathlib import Path
import create_ota_manifest as signer
import package_flash as package

ROOT = Path(__file__).resolve().parents[1]

def keypair(directory: Path):
    private = directory / "private.pem"
    public = directory / "public.pem"
    subprocess.run(["openssl", "genpkey", "-algorithm", "EC",
                    "-pkeyopt", "ec_paramgen_curve:P-256", "-out", str(private)],
                   check=True, capture_output=True)
    subprocess.run(["openssl", "pkey", "-in", str(private), "-pubout", "-out", str(public)],
                   check=True, capture_output=True)
    return private, public

def partition_table() -> bytes:
    import struct
    rows = [
        (1, 2, 0x9000, 0x5000, "nvs"),
        (1, 1, 0xE000, 0x1000, "phy_init"),
        (1, 0, 0xF000, 0x2000, "otadata"),
        (0, 0x10, 0x20000, 0x3F0000, "ota_0"),
        (0, 0x11, 0x410000, 0x3F0000, "ota_1"),
    ]
    return b"".join(struct.pack("<HBBII16sI", 0x50AA, t, st, off, size,
                                name.encode().ljust(16, b"\0"), 0)
                    for t, st, off, size, name in rows) + b"\xff" * (0x1000 - 160)

class OtaSecurityTests(unittest.TestCase):
    def test_signature_and_hash_verify(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); private, public = keypair(root)
            firmware = root / "firmware.bin"; firmware.write_bytes(b"ROZA test firmware image" * 512)
            m = signer.make_manifest(firmware, private, 222, "a" * 40)
            self.assertEqual(m["sha256"], signer.hashlib.sha256(firmware.read_bytes()).hexdigest())
            payload = signer.canonical_payload(m["build"], m["commit"], m["size"], m["sha256"], m["url"])
            data = root / "manifest.txt"; signature = root / "signature.der"
            data.write_bytes(payload); signature.write_bytes(base64.b64decode(m["signature"]))
            subprocess.run(["openssl", "dgst", "-sha256", "-verify", str(public),
                            "-signature", str(signature), str(data)], check=True, capture_output=True)

    def test_tampered_build_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); private, public = keypair(root)
            firmware = root / "firmware.bin"; firmware.write_bytes(b"ROZA test firmware image" * 512)
            m = signer.make_manifest(firmware, private, 101, "b" * 40)
            changed = signer.canonical_payload(m["build"] + 1, m["commit"], m["size"], m["sha256"], m["url"])
            data = root / "changed.txt"; signature = root / "signature.der"
            data.write_bytes(changed); signature.write_bytes(base64.b64decode(m["signature"]))
            result = subprocess.run(["openssl", "dgst", "-sha256", "-verify", str(public),
                                     "-signature", str(signature), str(data)], capture_output=True)
            self.assertNotEqual(result.returncode, 0)

    def test_zero_build_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); private, _ = keypair(root)
            firmware = root / "firmware.bin"; firmware.write_bytes(b"x" * 4096)
            with self.assertRaises(ValueError):
                signer.make_manifest(firmware, private, 0, "c" * 40)

    def test_partition_layout_keeps_nvs_and_has_two_slots(self):
        csv = (ROOT / "partitions_8MB.csv").read_text()
        self.assertIn("nvs,       data, nvs,     0x9000,   0x5000", csv)
        self.assertIn("otadata,   data, ota,     0xf000,   0x2000", csv)
        self.assertIn("ota_0,     app,  ota_0,   0x20000,  0x3f0000", csv)
        self.assertIn("ota_1,     app,  ota_1,  0x410000, 0x3f0000", csv)
        self.assertEqual(package.validate_partition_table(partition_table()), 0x3F0000)

    def test_bootloader_rollback_is_configured(self):
        self.assertIn("CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y",
                      (ROOT / "sdkconfig.defaults").read_text())

if __name__ == "__main__":
    unittest.main(verbosity=2)
