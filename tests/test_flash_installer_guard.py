#!/usr/bin/env python3
"""Exercise the ROZA installer safety gate without accessing serial hardware."""
from __future__ import annotations

import hashlib
import json
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
INSTALLER = ROOT / "tools" / "installer" / "flash.sh"
ERASE_REGIONS = ["0x11000-0x11FFF", "0x13000-0x18FFF", "0x19000-0x19FFF"]


def make_manifest(*, hardware_tested: bool, safe_to_flash: bool) -> dict:
    return {
        "hardware": {"chip": "ESP32-S3FN8", "flash_bytes": 8 * 1024 * 1024},
        "flash_image": {"write_offset": "0x0", "sha256": ""},
        "release": {
            "class": "complete-software-device-test-candidate",
            "hardware_tested": hardware_tested,
            "safe_to_flash": safe_to_flash,
            "installation": {
                "whole_flash_erase": False,
                "legacy_nvs_backup_required": True,
                "legacy_nvs_restored_exactly_after_merged_image_write": True,
                "legacy_nvs_restored_after_contiguous_merge_bin_write": True,
                "legacy_nvs_readback_compared_byte_for_byte": True,
                "new_nvs_initialized_empty": True,
                "otadata_reset": True,
                "erase_regions": ERASE_REGIONS,
                "micro_sd_contents_erased": False,
            },
        },
    }


def run_installer(tmp: Path, name: str, manifest: dict, fake_pythonpath: Path, log: Path):
    image = tmp / f"{name}.bin"
    image.write_bytes(b"ROZA installer guard test image\x00\x01")
    manifest["flash_image"]["sha256"] = hashlib.sha256(image.read_bytes()).hexdigest()
    manifest_path = tmp / f"{name}.json"
    manifest_path.write_text(json.dumps(manifest), encoding="utf-8")

    env = os.environ.copy()
    env.update({
        "MANIFEST": str(manifest_path),
        "PORT": "validation-only-not-a-device",
        "PYTHONPATH": str(fake_pythonpath) + os.pathsep + env.get("PYTHONPATH", ""),
        "ROZA_FAKE_ESPTOOL_LOG": str(log),
    })
    result = subprocess.run(
        ["bash", str(INSTALLER), str(image)],
        cwd=tmp,
        input="NON\n",
        text=True,
        capture_output=True,
        env=env,
        check=False,
    )
    return result, result.stdout + result.stderr


def assert_rejected(tmp: Path, name: str, manifest: dict, fake_pythonpath: Path, expected: str):
    log = tmp / f"{name}-esptool.log"
    result, output = run_installer(tmp, name, manifest, fake_pythonpath, log)
    assert result.returncode != 0, f"{name}: unsafe manifest unexpectedly passed"
    assert expected in output, f"{name}: expected {expected!r}, got:\n{output}"
    assert not log.exists(), f"{name}: esptool was invoked before manifest rejection"


def main() -> int:
    if not INSTALLER.is_file():
        raise SystemExit(f"Installateur introuvable : {INSTALLER}")

    with tempfile.TemporaryDirectory(prefix="roza-installer-guard-") as tmp_name:
        tmp = Path(tmp_name)
        fake_pythonpath = tmp / "fake-python"
        fake_module = fake_pythonpath / "esptool"
        fake_module.mkdir(parents=True)
        (fake_module / "__init__.py").write_text("", encoding="utf-8")
        (fake_module / "__main__.py").write_text(
            "import os, sys\n"
            "with open(os.environ['ROZA_FAKE_ESPTOOL_LOG'], 'a', encoding='utf-8') as f:\n"
            "    f.write(' '.join(sys.argv[1:]) + '\\n')\n"
            "if sys.argv[1:] == ['--help']:\n"
            "    raise SystemExit(0)\n"
            "raise SystemExit(97)\n",
            encoding="utf-8",
        )

        # Each approval flag is independently mandatory.
        assert_rejected(
            tmp, "untested",
            make_manifest(hardware_tested=False, safe_to_flash=True),
            fake_pythonpath, "le matériel n'a pas été validé",
        )
        assert_rejected(
            tmp, "not-approved",
            make_manifest(hardware_tested=True, safe_to_flash=False),
            fake_pythonpath, "n'autorise pas l'écriture de cette image",
        )

        # A coherent approved manifest must pass validation, but the test
        # deliberately answers NON at the explicit confirmation prompt. The fake
        # esptool module accepts only --help, so no serial/erase/write operation can run.
        approved_log = tmp / "approved-esptool.log"
        approved = make_manifest(hardware_tested=True, safe_to_flash=True)
        approved_result, approved_output = run_installer(
            tmp, "approved", approved, fake_pythonpath, approved_log
        )
        assert approved_result.returncode != 0, "test must stop at the confirmation prompt"
        assert "SHA-256 image vérifié" in approved_output, approved_output
        assert "Installation annulée." in approved_output, approved_output
        calls = approved_log.read_text(encoding="utf-8").splitlines()
        assert calls == ["--help"], f"unexpected esptool calls: {calls}"

        # A manifest that requests a full-chip erase remains rejected even when
        # the two approval flags are true.
        erase_manifest = make_manifest(hardware_tested=True, safe_to_flash=True)
        erase_manifest["release"]["installation"]["whole_flash_erase"] = True
        assert_rejected(
            tmp, "whole-erase", erase_manifest, fake_pythonpath,
            "n'interdit pas explicitement l'effacement global",
        )

        # The erase plan must exactly match the script's three bounded regions.
        range_manifest = make_manifest(hardware_tested=True, safe_to_flash=True)
        range_manifest["release"]["installation"]["erase_regions"] = ["0x0-0x7FFFFF"]
        assert_rejected(
            tmp, "wrong-ranges", range_manifest, fake_pythonpath,
            "Plages d'effacement du manifeste incompatibles",
        )

    print("ROZA installer guard: PASS")
    print(" - each release approval flag is independently required")
    print(" - approved manifest reaches explicit confirmation only")
    print(" - no serial read, erase or write command was executed")
    print(" - whole-flash erase manifest rejected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
