#!/usr/bin/env python3
"""Offline tests for ECDSA OTA signatures, anti-replay metadata and A/B partition layout."""
from __future__ import annotations
import base64, subprocess, tempfile, unittest
from pathlib import Path
import create_ota_manifest as signer
import package_flash as package

ROOT = Path(__file__).resolve().parents[1]

def keypair(directory: Path):
    directory.mkdir(parents=True, exist_ok=True)
    private = directory / "private.pem"
    public = directory / "public.pem"
    header = directory / "ota_public_key.h"
    subprocess.run(["openssl", "genpkey", "-algorithm", "EC",
                    "-pkeyopt", "ec_paramgen_curve:P-256", "-out", str(private)],
                   check=True, capture_output=True)
    subprocess.run(["openssl", "pkey", "-in", str(private), "-pubout", "-out", str(public)],
                   check=True, capture_output=True)
    pem_lines = public.read_text(encoding="ascii").splitlines()
    header.write_text(
        "#pragma once\nstatic const char ROZA_OTA_PUBLIC_KEY_PEM[] =\n"
        + "".join('    "' + line + r'\n"' + "\n" for line in pem_lines)
        + ";\n",
        encoding="ascii",
    )
    return private, public, header

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
    def test_signing_key_must_match_embedded_public_key(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            private_a, _public_a, _header_a = keypair(root / "a")
            _private_b, _public_b, header_b = keypair(root / "b")
            with self.assertRaises(ValueError):
                signer.verify_signing_key_matches_public(private_a, header_b)
    def test_signature_and_hash_verify(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); private, public, header = keypair(root)
            firmware = root / "firmware.bin"; firmware.write_bytes(b"ROZA test firmware image" * 512)
            m = signer.make_manifest(firmware, private, 222, "a" * 40, public_key_header=header)
            self.assertEqual(m["sha256"], signer.hashlib.sha256(firmware.read_bytes()).hexdigest())
            payload = signer.canonical_payload(m["build"], m["commit"], m["size"], m["sha256"], m["url"])
            data = root / "manifest.txt"; signature = root / "signature.der"
            data.write_bytes(payload); signature.write_bytes(base64.b64decode(m["signature"]))
            subprocess.run(["openssl", "dgst", "-sha256", "-verify", str(public),
                            "-signature", str(signature), str(data)], check=True, capture_output=True)

    def test_tampered_build_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); private, public, header = keypair(root)
            firmware = root / "firmware.bin"; firmware.write_bytes(b"ROZA test firmware image" * 512)
            m = signer.make_manifest(firmware, private, 101, "b" * 40, public_key_header=header)
            changed = signer.canonical_payload(m["build"] + 1, m["commit"], m["size"], m["sha256"], m["url"])
            data = root / "changed.txt"; signature = root / "signature.der"
            data.write_bytes(changed); signature.write_bytes(base64.b64decode(m["signature"]))
            result = subprocess.run(["openssl", "dgst", "-sha256", "-verify", str(public),
                                     "-signature", str(signature), str(data)], capture_output=True)
            self.assertNotEqual(result.returncode, 0)

    def test_zero_build_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp); private, _public, header = keypair(root)
            firmware = root / "firmware.bin"; firmware.write_bytes(b"x" * 4096)
            with self.assertRaises(ValueError):
                signer.make_manifest(firmware, private, 0, "c" * 40, public_key_header=header)

    def test_partition_layout_keeps_nvs_and_has_two_slots(self):
        csv = (ROOT / "partitions_8MB.csv").read_text()
        self.assertIn("nvs,       data, nvs,     0x9000,   0x5000", csv)
        self.assertIn("otadata,   data, ota,     0xf000,   0x2000", csv)
        self.assertIn("ota_0,     app,  ota_0,   0x20000,  0x3f0000", csv)
        self.assertIn("ota_1,     app,  ota_1,  0x410000, 0x3f0000", csv)
        self.assertEqual(package.validate_partition_table(partition_table()), 0x3F0000)

    def test_embedded_public_key_is_valid_pem(self):
        import re
        source = (ROOT / "src" / "ota_public_key.h").read_text()
        block = re.search(
            r"ROZA_OTA_PUBLIC_KEY_PEM\[\]\s*=\s*(.*?);",
            source, re.S
        )
        self.assertIsNotNone(block)
        fragments = re.findall(r'"((?:\\.|[^"\\])*)"', block.group(1))
        pem = "".join(bytes(item, "utf-8").decode("unicode_escape") for item in fragments)
        with tempfile.TemporaryDirectory() as temp:
            key_path = Path(temp) / "public.pem"
            key_path.write_text(pem)
            subprocess.run(
                ["openssl", "pkey", "-pubin", "-in", str(key_path), "-noout"],
                check=True, capture_output=True
            )

    def test_custom_sdkconfig_uses_separate_source_file(self):
        ini = (ROOT / "platformio.ini").read_text(encoding="utf-8")
        self.assertIn("custom_sdkconfig = file://sdkconfig.roza.defaults", ini)
        self.assertNotIn("custom_sdkconfig = file://sdkconfig.defaults", ini)
        source = (ROOT / "sdkconfig.roza.defaults").read_text(encoding="utf-8")
        self.assertIn("CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y", source)
        gitignore = (ROOT.parent / ".gitignore").read_text(encoding="utf-8")
        self.assertIn("/firmware/sdkconfig.defaults", gitignore)

    def test_bootloader_rollback_is_configured(self):
        self.assertIn("CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y",
                      (ROOT / "sdkconfig.roza.defaults").read_text())

    def test_manifest_parser_fails_closed_and_pins_binary_url(self):
        source = (ROOT / "src" / "ota_update.cpp").read_text()
        self.assertIn('strcmp(url,FIRMWARE_URL)!=0', source)
        self.assertIn('!verifySignature(m)', source)
        self.assertIn('m.size>0x3F0000UL', source)
        self.assertIn('m.size<4096', source)
        self.assertIn('hexValue(commit,40)', source)
        self.assertIn('hexValue(digest,64)', source)

    def test_installer_revalidates_manifest_before_writing(self):
        source = (ROOT / "src" / "ota_update.cpp").read_text()
        self.assertIn('fetchManifest(fresh,err,sizeof(err))', source)
        self.assertIn('fresh.build!=manifest.build', source)
        self.assertIn('strcmp(fresh.sha256,manifest.sha256)', source)
        self.assertIn('Update.setSHA256(fresh.sha256)', source)
        self.assertIn('Update.abort()', source)
        self.assertIn('Update.end(false)', source)

    def test_update_requires_https_and_certificate_validation(self):
        source = (ROOT / "src" / "ota_update.cpp").read_text()
        self.assertIn('MANIFEST_URL[]="https://', source)
        self.assertIn('FIRMWARE_URL[]="https://', source)
        self.assertIn('tlsUseCaBundle(client)', source)
        self.assertNotIn('setInsecure()', source)
        self.assertIn("length<=0", source.replace(" ", ""))
        self.assertIn("MANIFEST_MAX", source)

    def test_migration_never_chip_erases_and_requires_confirmation(self):
        script = (ROOT / "tools" / "migrate_to_ota.sh").read_text()
        self.assertIn('MIGRATE-ROZA', script)
        self.assertIn('SHA256SUMS-migration.txt', script)
        self.assertIn('hashlib.sha256', script)
        self.assertIn('erase-region 0xF000 0x2000', script)
        commands = "\n".join(
            line for line in script.splitlines()
            if line.lstrip().startswith("python -m esptool")
        )
        self.assertIn("erase-region 0xF000 0x2000", commands)
        self.assertNotRegex(commands, r"\b(?:erase-flash|erase_flash|chip\s+erase)\b")
        self.assertLess(script.index("hashlib.sha256"), script.index('read -r -p "Type MIGRATE-ROZA'))

    def test_migration_preserves_nvs_phy_and_sd(self):
        script = (ROOT / "tools" / "migrate_to_ota.sh").read_text()
        doc = (ROOT / "docs" / "UPDATE_SAFETY.md").read_text()
        self.assertIn('0xF000 0x2000', script)
        self.assertIn('NVS', doc)
        self.assertIn('PHY', doc)
        self.assertIn('microSD', doc)

    def test_build_number_is_monotonic_and_confirmation_records_it(self):
        source = (ROOT / "src" / "ota_update.cpp").read_text()
        self.assertIn('m.build>(stored>current?stored:current)', source)
        self.assertIn('saveUInt("ota","highest",current)', source)
        self.assertIn('esp_ota_mark_app_valid_cancel_rollback()', source)
        self.assertIn('esp_ota_mark_app_invalid_rollback_and_reboot()', source)

if __name__ == "__main__":
    unittest.main(verbosity=2)
