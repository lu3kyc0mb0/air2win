# Air2Win

Air2Win is a portable AirPlay receiver for Windows 10 (version 1809 or later) and Windows 11, x64. It receives iPhone and iPad screen mirroring and audio on the same local network. The interface uses Qt Quick; the controller is written in C++. A separate UxPlay process handles the AirPlay connection and opens video in its own window.

## Run

Download `dist/Air2Win-1.0.0-windows-x64.zip`, extract the entire `Air2Win` folder to a writable location, and open `Air2Win.exe`. Click **Start AirPlay services**, then choose the receiver from **Screen Mirroring** on an iPhone or iPad connected to the same network. If PIN protection is enabled, enter the code shown in the app.

The app keeps settings and the receiver identity in `data/` beside the EXE. It does not install a service, add startup entries, or create firewall rules. See [Quick start](docs/QUICKSTART.md) for troubleshooting and limitations.

## Build on this computer

Open PowerShell in `F:\project\air2win`:

```powershell
.\scripts\build.ps1 -Package
python scripts\archive.py
```

The build script compiles the Qt app and UxPlay helper, runs the receiver tests, and assembles `dist/Air2Win/`. Keep that whole folder together. If only the C++/QML interface changed and the helper is already packaged, use `-SkipReceiver -Package` for a faster build. Close any running Air2Win instance before packaging.

For QML changes, check syntax with `work\Qt\6.8.3\mingw_64\bin\qmllint.exe qml\Main.qml`. The script runs CTest automatically. `dist\Air2Win\Air2Win.exe --smoke-test` opens the UI and writes screenshots to `dist\Air2Win\data\`.

This checkout has its build tools in `work/`. Another development machine needs Qt 6.8.3, compatible MinGW 13.1, a MinGW UCRT compiler for UxPlay, Python, and the media dependencies. The `work/` directory is not part of the portable release.

## Scope and compatibility

Version 1.0.0 supports screen mirroring and audio. Direct app video casting and DRM-protected video are outside its current scope. The shipped build targets Windows 10/11 x64. Windows x86 and Windows 7/8 require separate toolchains, dependency builds, and testing; this ZIP is not compatible with them.

## Licenses

The original shell code is MIT-licensed. The receiver helper and bundled libraries have separate licenses; see [Third-party notices](THIRD_PARTY_NOTICES.md). Air2Win is an independent project and is not certified by Apple.
