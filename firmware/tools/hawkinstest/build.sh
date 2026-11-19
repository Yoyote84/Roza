#!/usr/bin/env bash
# Tests natifs du moteur de monde persistant Hawkins, sans matériel ESP32.
set -euo pipefail
cd "$(dirname "$0")"
: "${CXX:=g++}"
SANFLAGS=""
if [[ "${SAN:-0}" == "1" ]]; then
  SANFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
fi
"$CXX" -std=c++17 -Wall -Wextra -Werror $SANFLAGS \
  -I ../../src \
  main.cpp ../../src/hawkins_world.cpp \
  -o /tmp/roza-hawkinstest
/tmp/roza-hawkinstest
