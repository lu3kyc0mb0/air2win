# <img width="25" height="25" alt="7629289" src="https://github.com/user-attachments/assets/09dac1c3-85e9-4354-9359-16026f926fd9" /> Air2Win

Portable AirPlay screen mirroring and audio receiver for Windows 10 (1809+) / Windows 11, x64.

## Run

1. Extract the whole `Air2Win-1.0.0-windows-x64.zip` archive.
2. Open `Air2Win.exe` and click **Start AirPlay services**.
3. Connect your iPhone/iPad and PC to the same network. Choose Air2Win in **Screen Mirroring** or the audio output menu.

Now Playing shows track information when provided by the sender. Playback controls and seeking depend on sender support. The pin keeps the player on top; **Air2Win** in its footer reopens the main window. Close both windows to exit.

Settings and receiver identity stay in `data/` beside the EXE. No installation or background service is required. See [Quick start](docs/QUICKSTART.md) for troubleshooting.

## Screenshots

<details>
  <summary>Click to view</summary>

  <p>
    <img width="540" height="384" alt="Air2Win" src="https://github.com/user-attachments/assets/5975f5aa-14c9-46aa-83bc-a81515c08ced" />
  </p>

  <p>
    <img width="160" height="272" alt="Now Playing" src="https://github.com/user-attachments/assets/cdd9be80-25d8-4fc9-8ad7-397954f380e7" />
    <img width="160" height="272" alt="Now Playing" src="https://github.com/user-attachments/assets/d220be9e-cfc1-407b-ba30-5d131d706eb5" />
    <img width="160" height="272" alt="Now Playing" src="https://github.com/user-attachments/assets/368da6b3-5ad7-4499-b521-f3d52a9633cf" />
    <img width="160" height="272" alt="Now Playing" src="https://github.com/user-attachments/assets/890ca86a-5084-405c-b91a-d402f92f5814" />
  </p>
</details>

## Build

GitHub Actions builds the complete portable ZIP on every push to `main`, on pull requests,
and through **Actions → Windows build → Run workflow**. Download it from the run's
**Artifacts** section (GitHub sign-in required). Development downloads contain the
application files directly and are retained for 14 days.

To build in your computer, open PowerShell in the project folder:

```powershell
.\scripts\build.ps1 -Package
python scripts\archive.py
```

Close Air2Win before packaging. For interface-only changes, use `-SkipReceiver -Package`. Output: `dist/Air2Win/` and `dist/Air2Win-1.0.0-windows-x64.zip`; keep the complete application folder together.

Build dependencies: Qt 6.8.3 with MinGW 13.1, a MinGW UCRT compiler for UxPlay, Python, and the media libraries. Local tools are in `work/`.

## Limits and licenses

Direct app video casting and DRM-protected video are unsupported. This build does not support Windows x86 or Windows 7/8.

The app shell is [MIT-licensed](LICENSE). UxPlay and bundled libraries have separate licenses: [Third-party notices](THIRD_PARTY_NOTICES.md). Air2Win is an independent project, not certified by Apple.
