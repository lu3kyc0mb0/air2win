# Contributing to Air2Win

Use issues for reproducible bugs and feature discussions. Report vulnerabilities privately using [SECURITY.md](SECURITY.md).

## Development setup

Build on Windows x64 with PowerShell, Git, Python 3.13, CMake 3.24 or newer (below 4), Ninja, Qt 6.8.3 with its MinGW 13.1 compiler, and a separate MinGW UCRT compiler for UxPlay.

The complete tool installation sequence is maintained in [.github/workflows/windows.yml](.github/workflows/windows.yml). Match its Qt version, compiler versions, and pinned UxPlay commit. From the repository root:

```powershell
python -m pip install 'aqtinstall==3.3.*' 'cmake>=3.24,<4' ninja backports.zstd
python -m aqt install-qt windows desktop 6.8.3 win64_mingw -O work/Qt
python -m aqt install-tool windows desktop tools_mingw1310 qt.tools.win64_mingw1310 -O work/Qt
git clone https://github.com/FDH2/UxPlay.git work/uxplay
git -C work/uxplay checkout 82ad05ce34ec862851fa6ab1240f50f58fb7fa5a
python scripts/fetch-media-deps.py
```

Install the UCRT64 GCC toolchain separately through MSYS2 (package `mingw-w64-ucrt-x86_64-gcc`), then pass its bin directory explicitly:

```powershell
.\scripts\build.ps1 -UcrtCompilerBin 'C:/msys64/ucrt64/bin' -Package
python scripts/archive.py
```

Adjust the compiler path for your installation. Keep Qt's compiler separate from the receiver's UCRT compiler. Close Air2Win before packaging. Output is under `dist/`; run `dist/Air2Win/Air2Win.exe` with its complete folder.

For interface-only iteration after the initial full build:

```powershell
.\scripts\build.ps1 -SkipReceiver -Package
```

This reuses the previously packaged receiver; use a full build for receiver or packaging changes. Do not commit `work/`, build directories, `dist/`, or local `data/` contents.

## Checks before a pull request

- Run the relevant build command above. The script builds the UI and runs CTest before building and packaging the receiver.
- Check the CMake output: this checkout currently has no test sources, so CTest can complete without running tests. If test sources are added, verify that the expected tests actually run.
- For app or receiver changes, start the packaged app, start/stop reception, connect an iPhone/iPad, and check the affected mirroring or audio behavior and the Log page.
- For playback or window changes, check Now Playing, available playback controls, and closing both windows to exit.
- For packaging changes, extract the complete ZIP into a fresh writable folder and verify launch and reception, ideally on a Windows machine without development tools.
- Run `git diff --check`; review the diff for unrelated changes, personal data, and generated files.

State which checks passed and which you could not run. GitHub Actions checks Windows builds; CodeQL provides additional static analysis of the repository's C/C++ sources without building. It does not analyze the separately fetched UxPlay source, and does not replace runtime testing.

## Pull requests

Use a focused branch, such as `feature/short-description`, and keep changes scoped to one problem. Explain the resulting behavior and include screenshots for visible UI changes. Follow the existing C++/QML style. Keep version changes and release tags out of unrelated fixes. Update documentation and license notices when behavior or bundled dependencies change.
