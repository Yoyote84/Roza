Import("env")
import os

raw = os.environ.get("GITHUB_RUN_NUMBER", os.environ.get("ROZA_BUILD_NUMBER", "0"))
try:
    number = max(0, int(raw))
except (TypeError, ValueError):
    number = 0

env.Append(CPPDEFINES=[("ROZA_BUILD_NUMBER", number)])
print("ROZA OTA build number: %d" % number)
