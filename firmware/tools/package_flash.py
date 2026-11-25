#!/usr/bin/env python3
"""Assemble ESP32-S3 flash regions into a Cardputer ADV A/B migration package."""
from __future__ import annotations
import argparse
import hashlib
import shutil
import struct
from pathlib import Path

FLASH_SIZE=8*1024*1024
BOOTLOADER_OFFSET=0x0000
PARTITION_TABLE_OFFSET=0x8000
PARTITION_TABLE_SIZE=0x1000
APP_OFFSET=0x20000
APP_SLOT_SIZE=0x3F0000
ESP32S3_CHIP_ID=9
EXPECTED_PARTITIONS=(
 (0x01,0x02,0x9000,0x5000,"nvs"),
 (0x01,0x01,0xE000,0x1000,"phy_init"),
 (0x01,0x00,0xF000,0x2000,"otadata"),
 (0x00,0x10,0x20000,0x3F0000,"ota_0"),
 (0x00,0x11,0x410000,0x3F0000,"ota_1"),
)

def sha256(path:Path)->str:
 digest=hashlib.sha256()
 with path.open("rb") as stream:
  for block in iter(lambda:stream.read(1024*1024),b""):digest.update(block)
 return digest.hexdigest()

def write_checksum_manifest(path:Path,files:list[Path])->None:
 path.write_text("".join(f"{sha256(file)}  {file.name}\n" for file in files),encoding="utf-8")

def validate_partition_table(data:bytes)->int:
 if not data or len(data)>PARTITION_TABLE_SIZE or len(data)%32:
  raise SystemExit("Invalid partition-table length")
 entries=[]
 for offset in range(0,len(data)-31,32):
  magic=struct.unpack_from("<H",data,offset)[0]
  if magic==0xEBEB:break
  if data[offset:offset+32]==b"\xff"*32:break
  if magic!=0x50AA:raise SystemExit(f"Invalid partition-table entry at 0x{offset:x}")
  _magic,ptype,subtype,address,size,raw_label,_flags=struct.unpack_from("<HBBII16sI",data,offset)
  label=raw_label.split(b"\0",1)[0].decode("ascii",errors="strict")
  if size==0 or address+size>FLASH_SIZE:raise SystemExit(f"Invalid partition bounds for {label or 'unnamed'}")
  entries.append((ptype,subtype,address,size,label))
 if tuple(entries)!=EXPECTED_PARTITIONS:
  raise SystemExit("Unsupported layout: expected exact ROZA NVS/PHY/otadata/ota_0/ota_1 layout")
 ranges=sorted((a,a+z,n) for _t,_s,a,z,n in entries)
 for previous,current in zip(ranges,ranges[1:]):
  if previous[1]>current[0]:raise SystemExit(f"Partitions overlap: {previous[2]} and {current[2]}")
 return APP_SLOT_SIZE

def validate_esp32s3_image(data:bytes)->None:
 if len(data)<24 or data[0]!=0xE9 or not 1<=data[1]<=16:
  raise SystemExit("Invalid or truncated ESP firmware image")
 chip_id=struct.unpack_from("<H",data,12)[0]
 if chip_id!=ESP32S3_CHIP_ID:
  raise SystemExit(f"Wrong target chip_id={chip_id}, expected ESP32-S3 (9)")

def main()->None:
 parser=argparse.ArgumentParser()
 parser.add_argument("--build-dir",required=True,type=Path)
 parser.add_argument("--output-dir",required=True,type=Path)
 args=parser.parse_args()
 build,output=args.build_dir,args.output_dir
 sources={name:build/f"{name}.bin" for name in ("bootloader","partitions","firmware")}
 for name,path in sources.items():
  if not path.is_file() or path.stat().st_size==0:raise SystemExit(f"Missing or empty {name}: {path}")
 boot=sources["bootloader"].read_bytes()
 table=sources["partitions"].read_bytes()
 app=sources["firmware"].read_bytes()
 if len(boot)>PARTITION_TABLE_OFFSET:raise SystemExit("Bootloader overlaps partition table")
 if len(table)>PARTITION_TABLE_SIZE:raise SystemExit("Partition table exceeds 4 KiB")
 if PARTITION_TABLE_OFFSET+len(table)>APP_OFFSET:raise SystemExit("Partition table overlaps OTA_0")
 capacity=validate_partition_table(table)
 validate_esp32s3_image(app)
 if len(app)>capacity:raise SystemExit(f"Firmware {len(app)} exceeds OTA slot {capacity}")
 if APP_OFFSET+len(app)>FLASH_SIZE:raise SystemExit("Firmware exceeds 8 MiB Flash")

 output.mkdir(parents=True,exist_ok=True)
 image_path=output/"roza-cardputer-adv-8mb.bin"
 image_end=max(len(boot),PARTITION_TABLE_OFFSET+len(table),APP_OFFSET+len(app))
 image=bytearray(b"\xff"*image_end)
 image[BOOTLOADER_OFFSET:BOOTLOADER_OFFSET+len(boot)]=boot
 image[PARTITION_TABLE_OFFSET:PARTITION_TABLE_OFFSET+len(table)]=table
 image[APP_OFFSET:APP_OFFSET+len(app)]=app
 image_path.write_bytes(image)

 migration=[]
 for name,source in sources.items():
  target=output/f"{name}.bin"
  shutil.copyfile(source,target)
  migration.append(target)
 script=output/"migrate_to_ota.sh"
 shutil.copyfile(Path(__file__).with_name("migrate_to_ota.sh"),script)
 script.chmod(0o755)
 migration.append(script)
 write_checksum_manifest(output/"SHA256SUMS-flash.txt",[image_path])
 write_checksum_manifest(output/"SHA256SUMS-migration.txt",migration)
 (output/"MIGRATION-USB.txt").write_text(
  "ROZA — migration USB unique vers A/B OTA\n"
  "Cible : Cardputer ADV / ESP32-S3FN8 / 8 MiB / sans PSRAM\n"
  "L'ancienne table factory unique ne permet pas l'OTA : migration USB obligatoire une fois.\n"
  "Le script exige une confirmation et ne fait PAS de chip erase.\n"
  "Il efface uniquement otadata (0xF000..0x10FFF), puis écrit bootloader, table et OTA_0.\n"
  "NVS reste à 0x9000/0x5000 et PHY à 0xE000/0x1000 ; la microSD n'est jamais touchée.\n"
  f"OTA_0 : 0x{APP_OFFSET:05x}, taille 0x{capacity:x}; OTA_1 : 0x410000, taille 0x3f0000.\n"
  f"Firmware : {len(app)} octets; SHA-256 : {sha256(sources['firmware'])}.\n",encoding="utf-8")
 (output/"FLASH-INFO.txt").write_text(
  "ROZA — Cardputer ADV ESP32-S3FN8 / 8 MiB / no PSRAM\n"
  "Full image: roza-cardputer-adv-8mb.bin @ 0x0000\n"
  "Included: bootloader 0x0000, partition table 0x8000, OTA_0 app 0x20000\n"
  "Full image overwrites the NVS region and resets ROZA settings.\n"
  "Use the migration kit to preserve NVS on existing ROZA factory installations.\n"
  "The microSD is not included or modified.\n"
  f"Firmware: {len(app)} bytes / OTA slot capacity: {capacity} bytes\n"
  f"Full image: {len(image)} bytes\nCI validation is not a physical hardware test.\n",encoding="utf-8")
 print(f"Full image: {image_path} ({len(image)} bytes)")
 print(f"OTA slot: {capacity} bytes; firmware: {len(app)} bytes")

if __name__=="__main__":main()
