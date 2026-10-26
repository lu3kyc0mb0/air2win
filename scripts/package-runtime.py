import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("--objdump", required=True)
parser.add_argument("--destination", default=str(root / "dist/Air2Win/runtime"))
args = parser.parse_args()
deps = root / "work/media-deps/ucrt64"
destination = Path(args.destination)
destination.mkdir(parents=True, exist_ok=True)
(destination / "plugins").mkdir(exist_ok=True)
plugins = ["coreelements", "app", "playback", "typefindfunctions", "audioconvert",
           "audioresample", "volume", "autodetect", "audioparsers", "videoparsersbad",
           "videoconvertscale", "d3d11", "wasapi", "libav", "level"]
seeds = [(root / "work/build-uxplay/uxplay.exe", destination / "uxplay.exe")]
for executable in ["gst-inspect-1.0.exe", "gst-launch-1.0.exe"]:
    seeds.append((deps / "bin" / executable, destination / executable))
for plugin in plugins:
    filename = "libgst" + plugin + ".dll"
    seeds.append((deps / "lib/gstreamer-1.0" / filename, destination / "plugins" / filename))
known = {p.name.lower(): p for p in (deps / "bin").glob("*.dll")}
system = Path(os.environ["SystemRoot"]) / "System32"
visited = set()
missing = set()

def copy_closure(source, target):
    if source.name.lower() in visited:
        return
    if not source.is_file():
        raise RuntimeError("Missing runtime file: " + str(source))
    visited.add(source.name.lower())
    shutil.copy2(source, target)
    output = subprocess.check_output([args.objdump, "-p", str(source)], text=True, errors="replace")
    for name in re.findall(r"DLL Name:\s*(\S+)", output):
        key = name.lower()
        if key in known:
            copy_closure(known[key], destination / known[key].name)
        elif key.startswith(("api-ms-", "ext-ms-")) or (system / name).is_file():
            continue
        else:
            missing.add(name)

for source, target in seeds:
    copy_closure(source, target)
if missing:
    raise SystemExit("Missing dependency DLLs: " + ", ".join(sorted(missing)))
shutil.copy2(root / "work/media-deps.lock.json", destination / "packages.lock.json")
if (deps / "share/licenses").exists():
    shutil.copytree(deps / "share/licenses", destination / "licenses", dirs_exist_ok=True)
shutil.copy2(root / "work/uxplay/LICENSE", destination / "UXPLAY-LICENSE.txt")
shutil.copy2(root / "work/uxplay/README.md", destination / "UXPLAY-README.md")
print(f"Packaged {len(visited)} runtime binaries; no unresolved DLL imports.")