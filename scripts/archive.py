import argparse
from pathlib import Path
import re
from zipfile import ZipFile, ZIP_DEFLATED

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--suffix', default='', help='Optional build suffix, e.g. dev-abc1234')
args = parser.parse_args()
if args.suffix and not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9.-]*', args.suffix):
    parser.error('Suffix must contain only letters, numbers, dots, and hyphens')
version = re.search(r'project\(Air2Win VERSION (\d+\.\d+\.\d+)',
                    (root / 'CMakeLists.txt').read_text()).group(1)
if args.suffix:
    version += '-' + args.suffix
folder = root / "dist/Air2Win"
archive = root / f"dist/Air2Win-{version}-windows-x64.zip"
if not (folder / "Air2Win.exe").exists() or not (folder / "runtime/uxplay.exe").exists():
    raise SystemExit("Build the complete portable folder before creating an archive.")
with ZipFile(archive, "w", ZIP_DEFLATED, compresslevel=6) as output:
    for path in sorted(folder.rglob("*")):
        if path.is_file() and "data" not in path.relative_to(folder).parts:
            output.write(path, Path("Air2Win") / path.relative_to(folder))
with ZipFile(archive) as check:
    if check.testzip() is not None:
        raise RuntimeError("ZIP validation failed")
    assert not any("/data/" in name or name.endswith("identity.key") for name in check.namelist())
print(f"{archive} ({archive.stat().st_size / 1024**2:.1f} MiB)")
