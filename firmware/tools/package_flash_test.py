#!/usr/bin/env python3
"""Host-side tests for the ROZA A/B OTA partition and flash-package safety."""
import struct
import subprocess
import tempfile
import unittest
import zipfile
from pathlib import Path
import package_flash as package

def entry(ptype,subtype,address,size,label):
 return struct.pack("<HBBII16sI",0x50AA,ptype,subtype,address,size,label.encode("ascii").ljust(16,b"\0"),0)

def valid_table():
 rows=[
  entry(1,2,0x9000,0x5000,"nvs"),
  entry(1,1,0xE000,0x1000,"phy_init"),
  entry(1,0,0xF000,0x2000,"otadata"),
  entry(0,0x10,0x20000,0x3F0000,"ota_0"),
  entry(0,0x11,0x410000,0x3F0000,"ota_1"),
 ]
 return b"".join(rows)+b"\xff"*(0x1000-32*len(rows))

def esp_image(chip_id=9,magic=0xE9):
 image=bytearray(24);image[0]=magic;image[1]=1
 struct.pack_into("<H",image,12,chip_id)
 return bytes(image)

class PackageSafetyTests(unittest.TestCase):
 def test_exact_ota_layout(self):
  self.assertEqual(package.validate_partition_table(valid_table()),0x3F0000)
  self.assertEqual(package.APP_OFFSET,0x20000)

 def test_reject_changed_slot_offset(self):
  bad=valid_table().replace(entry(0,0x10,0x20000,0x3F0000,"ota_0"),
                            entry(0,0x10,0x30000,0x3E0000,"ota_0"))
  with self.assertRaises(SystemExit):package.validate_partition_table(bad)

 def test_reject_partition_overlap(self):
  rows=[
   entry(1,2,0x9000,0x7000,"nvs"),entry(1,1,0xE000,0x1000,"phy_init"),
   entry(1,0,0xF000,0x2000,"otadata"),entry(0,0x10,0x20000,0x3F0000,"ota_0"),
   entry(0,0x11,0x410000,0x3F0000,"ota_1")]
  with self.assertRaises(SystemExit):package.validate_partition_table(b"".join(rows))

 def test_nvs_unchanged(self):
  nvs=next(p for p in package.EXPECTED_PARTITIONS if p[4]=="nvs")
  self.assertEqual(nvs[2:4],(0x9000,0x5000))

 def test_checksum_detects_tampering_after_zip(self):
  with tempfile.TemporaryDirectory() as tmp:
   root=Path(tmp);d=root/"package";d.mkdir()
   binary=d/"firmware.bin";binary.write_bytes(b"ROZA signed A/B image")
   manifest=d/"SHA256SUMS.txt";package.write_checksum_manifest(manifest,[binary])
   archive=root/"package.zip"
   with zipfile.ZipFile(archive,"w") as z:z.write(binary,binary.name);z.write(manifest,manifest.name)
   out=root/"extracted";out.mkdir()
   with zipfile.ZipFile(archive) as z:z.extractall(out)
   subprocess.run(["sha256sum","-c",manifest.name],cwd=out,check=True,capture_output=True)
   (out/binary.name).write_bytes(b"tampered")
   failed=subprocess.run(["sha256sum","-c",manifest.name],cwd=out,capture_output=True)
   self.assertNotEqual(failed.returncode,0)

 def test_accepts_esp32s3_image(self):package.validate_esp32s3_image(esp_image())
 def test_rejects_wrong_chip(self):
  with self.assertRaises(SystemExit):package.validate_esp32s3_image(esp_image(chip_id=2))
 def test_rejects_bad_magic(self):
  with self.assertRaises(SystemExit):package.validate_esp32s3_image(esp_image(magic=0))
 def test_rejects_truncated_image(self):
  with self.assertRaises(SystemExit):package.validate_esp32s3_image(b"\xe9")

if __name__=="__main__":unittest.main(verbosity=2)
