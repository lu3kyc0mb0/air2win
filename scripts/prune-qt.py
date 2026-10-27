from pathlib import Path
import shutil

root = (Path(__file__).resolve().parents[1] / "dist/Air2Win").resolve()
if not (root / "Air2Win.exe").is_file():
    raise SystemExit("Air2Win.exe is missing; deploy Qt before pruning")

unused_styles = ("FluentWinUI3", "Fusion", "Imagine", "Material", "Universal", "Windows")
for style in unused_styles:
    path = (root / "qml/QtQuick/Controls" / style).resolve()
    if not path.is_relative_to(root):
        raise RuntimeError(f"Refusing to remove a path outside the release: {path}")
    if path.exists():
        shutil.rmtree(path)

for path in (root / "qmltooling",):
    path = path.resolve()
    if not path.is_relative_to(root):
        raise RuntimeError(f"Refusing to remove a path outside the release: {path}")
    if path.exists():
        shutil.rmtree(path)

for dll in root.glob("Qt6QuickControls2*.dll"):
    if any(style in dll.name for style in unused_styles):
        dll.unlink()

print("Removed unused Qt styles and QML debugging tools")
