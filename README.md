# <img width="25" height="25" alt="7629289" src="https://github.com/user-attachments/assets/09dac1c3-85e9-4354-9359-16026f926fd9" /> Air2Win

Portable AirPlay screen mirroring and audio receiver for Windows 10 (1809+) / Windows 11, x64.

## Run

1. Extract the whole `Air2Win-1.0.0-windows-x64.zip` archive.
2. Open `Air2Win.exe` and click **Start AirPlay services**.
3. Connect your iPhone/iPad and PC to the same network. Choose Air2Win in **Screen Mirroring** or the audio output menu.

Now Playing shows track information when provided by the sender. Playback controls and seeking depend on sender support. The pin keeps the player on top; **Air2Win** in its footer reopens the main window. Close both windows to exit.

Settings and receiver identity stay in `data/` beside the EXE. No installation or background service is required. See [Quick start](docs/QUICKSTART.md) for troubleshooting.

## Screenshot

<img width="432" height="307" alt="preview" src="https://github.com/user-attachments/assets/5a6d2acf-cb64-4ca1-aa90-d1dada6053ff" />

## Build

GitHub Actions builds the complete portable ZIP on every push to `main`, on pull requests,
and through **Actions → Windows build → Run workflow**. Download it from the run's
**Artifacts** section (GitHub sign-in required). Development builds include the commit ID
in their filename and are retained for 14 days.

To publish a release, update the version in `CMakeLists.txt`, commit and push it, then push
a matching tag, for example:

```powershell
git tag v1.0.1
git push origin v1.0.1
```

The tag must match the project version. A successful tag build automatically publishes
the release and attaches the ZIP. No additional repository secrets are required.

On this computer, open PowerShell in the project folder:

```powershell
.\scripts\build.ps1 -Package
python scripts\archive.py
```

Close Air2Win before packaging. For interface-only changes, use `-SkipReceiver -Package`. Output: `dist/Air2Win/` and `dist/Air2Win-1.0.0-windows-x64.zip`; keep the complete application folder together.

Build dependencies: Qt 6.8.3 with MinGW 13.1, a MinGW UCRT compiler for UxPlay, Python, and the media libraries. Local tools are in `work/`.

## Limits and licenses

Direct app video casting and DRM-protected video are unsupported. This build does not support Windows x86 or Windows 7/8.

The app shell is [MIT-licensed](LICENSE). UxPlay and bundled libraries have separate licenses: [Third-party notices](THIRD_PARTY_NOTICES.md). Air2Win is an independent project, not certified by Apple.
