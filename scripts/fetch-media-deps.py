import concurrent.futures
import hashlib
import io
import json
from pathlib import Path
import re
import tarfile
import urllib.request

try:
    from compression import zstd
except ImportError:
    from backports import zstd

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / "work"
BASE = "https://repo.msys2.org/mingw/ucrt64/"
PREFIX = "mingw-w64-ucrt-x86_64-"

def download(url, destination):
    with urllib.request.urlopen(url, timeout=120) as response:
        destination.write_bytes(response.read())

def main():
    WORK.mkdir(exist_ok=True)
    db = WORK / "ucrt64.db"
    if not db.exists():
        download(BASE + "ucrt64.db", db)
    packages = {}
    raw = db.read_bytes()
    if raw.startswith(b"\x28\xb5\x2f\xfd"):
        raw = zstd.decompress(raw)
    with tarfile.open(fileobj=io.BytesIO(raw)) as archive:
        for member in archive:
            if member.name.endswith("/desc"):
                content = archive.extractfile(member).read().decode()
                fields = {}
                for block in content.strip().split("\n\n"):
                    lines = block.splitlines()
                    fields[lines[0].strip("%")] = lines[1:]
                packages[fields["NAME"][0]] = fields
    selected = {}

    def add(name):
        name = re.split(r"[<>=]", name)[0]
        if name in selected:
            return
        if name not in packages:
            candidates = [p for p, f in packages.items()
                          if any(re.split(r"[<>=]", v)[0] == name for v in f.get("PROVIDES", []))]
            if not candidates:
                raise RuntimeError("Unresolved package: " + name)
            name = candidates[0]
        fields = packages[name]
        selected[name] = fields
        for dependency in fields.get("DEPENDS", []):
            add(dependency)

    for name in ["gstreamer", "gst-plugins-base", "libplist", "openssl", "pkgconf", "ffmpeg"]:
        add(PREFIX + name)
    for name in ["gst-plugins-good", "gst-plugins-bad", "gst-plugins-bad-libs", "gst-libav"]:
        selected[PREFIX + name] = packages[PREFIX + name]
    cache = WORK / "packages"
    cache.mkdir(exist_ok=True)
    size = sum(int(f["CSIZE"][0]) for f in selected.values())
    print(f"Downloading {len(selected)} packages ({size / 1024**2:.1f} MiB)", flush=True)

    def fetch(item):
        name, fields = item
        filename = fields["FILENAME"][0]
        path = cache / filename
        expected = fields["SHA256SUM"][0]
        if not path.exists() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            download(BASE + filename, path)
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise RuntimeError("Checksum mismatch: " + filename)
        print("Verified " + name, flush=True)
        return path

    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
        paths = list(executor.map(fetch, selected.items()))
    destination = WORK / "media-deps"
    destination.mkdir(exist_ok=True)
    for path in paths:
        marker = destination / (path.name + ".extracted")
        if marker.exists():
            continue
        with zstd.open(path, "rb") as source:
            with tarfile.open(fileobj=io.BytesIO(source.read())) as archive:
                members = [m for m in archive if m.name.startswith("ucrt64/") and not m.issym() and not m.islnk()]
                archive.extractall(destination, members=members, filter="data")
        marker.touch()
    (WORK / "media-deps.lock.json").write_text(json.dumps([
        {"name": n, "version": f["VERSION"][0], "file": f["FILENAME"][0],
         "sha256": f["SHA256SUM"][0]} for n, f in selected.items()
    ], indent=2) + "\n")
    print("Media dependencies ready: " + str(destination / "ucrt64"), flush=True)

if __name__ == "__main__":
    main()