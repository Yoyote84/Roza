# ROZA update 105 — controlled rebuild from known-good 104

## Purpose

This is a controlled update/rebuild test based on the exact known-good firmware source commit:

`dbc9e79c22bfa6757cd26243d1a7f1dae3dc29c5` (build #104).

No application source code, startup sequence, display initialization, memory allocation, partition table, or dependency pin is changed in this test.

## Test procedure

1. Build the Cardputer ADV firmware using the existing pinned PlatformIO environment.
2. Run the repository's existing software test suites.
3. Produce the normal full-flash package and the application-only `ROZA_UPDATE.bin`.
4. Compare the resulting binary and package hashes with the archived #104 artifacts where available.
5. Only after build/test success, install the application-only image on the physical Cardputer ADV at the documented application offset, without erasing NVS or rewriting bootloader/partitions.

## Safety

- This is not a feature release.
- Do not flash a full image for this controlled test.
- Do not erase flash or NVS.
- Do not merge this PR automatically.
- If the device does not boot normally, restore the known-good #104 recovery package.
