from pathlib import Path
from zipfile import ZipFile, ZIP_DEFLATED

root = Path(__file__).resolve().parents[1]
folder = root / "dist/Air2Win"
archive = root / "dist/Air2Win-1.0.0-windows-x64.zip"
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
