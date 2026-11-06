#!/usr/bin/env sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
git submodule update --init --recursive
python3 scripts/apply-roza-branding.py
python3 -m platformio run -d launcher -e m5stack-cardputer
