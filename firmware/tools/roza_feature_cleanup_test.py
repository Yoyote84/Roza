#!/usr/bin/env python3
"""Vérifie que les anciens fichiers et symboles du module de jeu restent absents."""
from pathlib import Path
import sys

REPO_ROOT = Path(__file__).resolve().parents[2]
IGNORED_DIRS = {".git", ".pio", "__pycache__", "node_modules", "build", "dist"}
# Les fragments sont séparés afin que le contrôle ne crée pas lui-même la chaîne interdite.
FORBIDDEN = ("Hawk" + "ins",)

violations: list[str] = []
for path in REPO_ROOT.rglob("*"):
    if not path.is_file():
        continue

    relative = path.relative_to(REPO_ROOT)
    if any(part in IGNORED_DIRS for part in relative.parts):
        continue

    if any(term.casefold() in relative.as_posix().casefold() for term in FORBIDDEN):
        violations.append(f"{relative}: nom de fichier interdit")
        continue

    try:
        content = path.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError):
        # Les ressources binaires ne peuvent pas contenir de code/source textuel
        # interprétable ; les noms de chemins restent contrôlés ci-dessus.
        continue

    folded = content.casefold()
    for term in FORBIDDEN:
        needle = term.casefold()
        start = 0
        while True:
            index = folded.find(needle, start)
            if index < 0:
                break
            line = content.count("\n", 0, index) + 1
            violations.append(f"{relative}:{line}: référence résiduelle interdite")
            start = index + len(needle)

if violations:
    for item in violations:
        print(f"ECHEC: {item}", file=sys.stderr)
    raise SystemExit(1)

print("OK: aucun ancien fichier, symbole ou texte du module de jeu dans le dépôt suivi.")
