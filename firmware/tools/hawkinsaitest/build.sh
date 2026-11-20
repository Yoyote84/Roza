#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
CXX_BIN="\${CXX:-g++}"
JSON_INC="../../.pio/libdeps/cardputer-adv/ArduinoJson/src"
if [ ! -f "$JSON_INC/ArduinoJson.h" ]; then
  echo "ArduinoJson absent : executer d'abord platformio run -e cardputer-adv."
  exit 1
fi
"$CXX_BIN" -std=c++17 -Wall -Wextra -Werror \
  -I ../../src -I "$JSON_INC" \
  main.cpp ../../src/hawkins_ai_protocol.cpp \
  -o /tmp/roza-hawkins-ai-test
/tmp/roza-hawkins-ai-test
