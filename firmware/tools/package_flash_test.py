#!/usr/bin/env python3
"""Host-side tests for the firmware package safety checks."""
import struct
import subprocess
import tempfile
import unittest
import zipfile
from pathlib import Path

import package_flash as package


def entry(ptype, subtype, address, size, label):
    return struct.pack(
        "<HBBII16sI", 0x50AA, ptype, subtype, address, size,
        label.encode("ascii").ljust(16, b"\0"), 0
    )


def valid_table():
    rows = [
        entry(0x01, 0x02, 0x9000, 0x5000, "nvs"),
        entry(0x01, 0x01, 0xE000, 0x1000, "phy_init"),
        entry(0x00, 0x00, 0x10000, 0x7E0000, "factory"),
    ]
    return b"".join(rows) + b"\xff" * (0x1000 - 32 * len(rows))


def esp_image(chip_id=9, magic=0xE9):
    image = bytearray(24)
    image[0] = magic
    image[1] = 1
    struct.pack_into("<H", image, 12, chip_id)
    return bytes(image)


class PackageSafetyTests(unittest.TestCase):
    def test_checksum_manifests_verify_inside_each_download_archive(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            cases = (
                (
                    "flash",
                    "roza-cardputer-adv-8mb.bin",
                    b"full image",
                    "SHA256SUMS-flash.txt",
                ),
                (
                    "update",
                    "ROZA_UPDATE.bin",
                    b"app only",
                    "SHA256SUMS-update.txt",
                ),
            )

            for kind, filename, payload, manifest_name in cases:
                with self.subTest(artifact=kind):
                    package_dir = root / kind
                    package_dir.mkdir()
                    binary = package_dir / filename
                    manifest = package_dir / manifest_name
                    binary.write_bytes(payload)
                    package.write_checksum_manifest(manifest, [binary])

                    # GitHub Actions artifacts are downloaded as ZIP archives.
                    archive = root / f"{kind}.zip"
                    with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as zipped:
                        zipped.write(binary, binary.name)
                        zipped.write(manifest, manifest.name)

                    extracted = root / f"{kind}-extracted"
                    extracted.mkdir()
                    with zipfile.ZipFile(archive) as zipped:
                        zipped.extractall(extracted)

                    manifest_bytes = (extracted / manifest_name).read_bytes()
                    self.assertTrue(manifest_bytes.endswith(b"\n"))
                    self.assertEqual(len(manifest_bytes.splitlines()), 1)
                    result = subprocess.run(
                        ["sha256sum", "-c", manifest_name],
                        cwd=extracted,
                        check=True,
                        capture_output=True,
                        text=True,
                    )
                    self.assertIn(f"{filename}: OK", result.stdout)

                    # A changed payload must make the same archive check fail.
                    (extracted / filename).write_bytes(payload + b"tampered")
                    failed = subprocess.run(
                        ["sha256sum", "-c", manifest_name],
                        cwd=extracted,
                        capture_output=True,
                        text=True,
                    )
                    self.assertNotEqual(failed.returncode, 0)

    def test_accepts_exact_roza_layout(self):
        self.assertEqual(package.validate_partition_table(valid_table()), 0x7E0000)

    def test_rejects_changed_factory_offset(self):
        changed = valid_table().replace(
            entry(0, 0, 0x10000, 0x7E0000, "factory"),
            entry(0, 0, 0x20000, 0x7D0000, "factory"),
        )
        with self.assertRaises(SystemExit):
            package.validate_partition_table(changed)

    def test_rejects_overlapping_partitions(self):
        rows = [
            entry(1, 2, 0x9000, 0x7000, "nvs"),
            entry(1, 1, 0xE000, 0x1000, "phy_init"),
            entry(0, 0, 0x10000, 0x7E0000, "factory"),
        ]
        with self.assertRaises(SystemExit):
            package.validate_partition_table(b"".join(rows) + b"\xff" * (0x1000 - 96))

    def test_accepts_esp32s3_image(self):
        package.validate_esp32s3_image(esp_image())

    def test_rejects_wrong_chip(self):
        with self.assertRaises(SystemExit):
            package.validate_esp32s3_image(esp_image(chip_id=2))

    def test_rejects_bad_magic(self):
        with self.assertRaises(SystemExit):
            package.validate_esp32s3_image(esp_image(magic=0))

    def test_rejects_truncated_image(self):
        with self.assertRaises(SystemExit):
            package.validate_esp32s3_image(b"\xe9")


if __name__ == "__main__":
    unittest.main(verbosity=2)
