#!/usr/bin/env python3
"""Regression tests for the BadApple SD file header written by badapple_pack.py."""
import struct
import unittest
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from badapple_pack import H, W, build_header


class BadApplePackTests(unittest.TestCase):
    def test_header_matches_firmware_binary_layout(self):
        header = build_header(30, 1800)
        self.assertEqual(len(header), 16)
        magic, width, height, fps, reserved, frames = struct.unpack("<4sHHHHI", header)
        self.assertEqual((magic, width, height, fps, reserved, frames),
                         (b"BAP1", W, H, 30, 0, 1800))
        frame_bytes = (W // 8) * H
        self.assertEqual(frame_bytes, 4050)
        self.assertEqual(len(header) + frame_bytes * frames, 7_290_016)

    def test_rejects_invalid_fps_or_empty_video(self):
        for fps in (0, 121):
            with self.assertRaises(ValueError):
                build_header(fps, 10)
        with self.assertRaises(ValueError):
            build_header(30, 0)


if __name__ == "__main__":
    unittest.main(verbosity=2)
