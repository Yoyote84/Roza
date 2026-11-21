#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
CXX_BIN="${CXX:-g++}"
"$CXX_BIN" -std=c++17 -O2 -Wall -Wextra -pthread \
  -DLLM_FORCE_SCALAR \
  -I host -I ../../src/hawkins_llm \
  main.cpp ../../src/hawkins_llm/llm.cpp \
  -o /tmp/roza-hawkins-llm-test
/tmp/roza-hawkins-llm-test
