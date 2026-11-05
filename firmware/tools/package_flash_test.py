#!/usr/bin/env python3
"""Host-side tests for the firmware package safety checks."""
import struct
import unittest

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
