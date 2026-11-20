#!/usr/bin/env bash
# Tests natifs du moteur Hawkins, sans matériel ESP32.
set -euo pipefail
cd "$(dirname "$0")"
if [ -z "${CXX:-}" ]; then CXX=g++; fi
SANFLAGS=""
if [ "${SAN:-0}" = "1" ]; then
  SANFLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer"
fi
"$CXX" -std=c++17 -Wall -Wextra -Werror $SANFLAGS \
  -I ../../src \
  main.cpp ../../src/hawkins_world.cpp ../../src/hawkins_save_codec.cpp \
  ../../src/hawkins_exploration.cpp ../../src/hawkins_character.cpp \
  -o /tmp/roza-hawkinstest
/tmp/roza-hawkinstest
"$CXX" -std=c++17 -Wall -Wextra -Werror $SANFLAGS \
  -I ../../src missions_test.cpp ../../src/hawkins_missions.cpp \
  -o /tmp/roza-hawkins-missions-test

"$CXX" -std=c++17 -Wall -Wextra -Werror $SANFLAGS \
  -I ../../src adventure_test.cpp ../../src/hawkins_adventure.cpp \
  ../../src/hawkins_character.cpp ../../src/hawkins_missions.cpp \
  -o /tmp/roza-hawkins-adventure-test
/tmp/roza-hawkins-adventure-test
/tmp/roza-hawkins-missions-test
