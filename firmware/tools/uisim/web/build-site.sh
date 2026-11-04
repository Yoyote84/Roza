#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
SIM="$(cd "$HERE/.." && pwd)"
SITE="${1:-$SIM/site}"
mkdir -p "$SITE/screens"
cp "$HERE/index.html" "$HERE/manifest.webmanifest" "$HERE/sw.js" "$HERE/icon.svg" "$SITE/"
find "$SIM/out" -maxdepth 1 -type f -name '*.png' -print0 | sort -z | while IFS= read -r -d '' file; do
  cp "$file" "$SITE/screens/"
done
python3 - "$SITE/screens" "$SITE/screens.json" <<'PY'
import json, pathlib, sys
folder, target = map(pathlib.Path, sys.argv[1:])
screens = sorted(p.name for p in folder.glob("*.png"))
target.write_text(json.dumps({"screens": screens}, ensure_ascii=False, indent=2) + "\\n", encoding="utf-8")
if not screens:
    raise SystemExit("Aucune capture PNG générée par uisim.")
print(f"Site prêt : {len(screens)} captures, {target}")
PY