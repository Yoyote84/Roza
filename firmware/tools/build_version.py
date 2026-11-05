Import("env")
import subprocess
from pathlib import Path

project_dir = Path(env["PROJECT_DIR"])
repo_dir = project_dir.parent
try:
    version = subprocess.check_output(
        ["git", "rev-parse", "--short=12", "HEAD"],
        cwd=str(repo_dir),
        text=True,
        stderr=subprocess.DEVNULL,
    ).strip()
except Exception:
    version = "dev"

env.Append(CPPDEFINES=[("ROZA_BUILD_VERSION", r'\"' + version + r'\"')])
print("[ROZA] Build version: " + version)
