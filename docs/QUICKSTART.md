# Air2Win Quick start

1. Extract the entire `Air2Win` folder to a writable location on Windows 10 (1809+) or Windows 11, x64.
2. Run `Air2Win.exe` and click **Start AirPlay services**.
3. Connect your iPhone or iPad and PC to the same local network.
4. Open Control Center → **Screen Mirroring** and select the receiver name shown in Air2Win.
5. If PIN protection is enabled, enter the code displayed in Air2Win.

The status changes to **Ready to connect** once the receiver starts. Mirroring opens in a separate window. For audio output, select the receiver in the iPhone or iPad audio output menu.

For music sent through AirPlay audio, **Now Playing** window opens when track metadata arrives. It shows the supplied cover, title, artist, album, and progress. Reopen it from the main page if closed. Playback buttons appear when the sender advertises DACP remote control. The progress slider can seek when supported; rejected seeking becomes read-only. Otherwise control playback on your phone. Metadata and progress may be unavailable with some apps or during screen mirroring.

Now Playing stays open when the receiver window is minimized or closed. Its pin button keeps it above other apps; **Air2Win** in the footer reopens the receiver window. The background follows the album colors. Close both windows to exit and stop reception.

## If the receiver is not visible

- Check that both devices are on the same network and are not isolated by a guest network or VPN.
- Allow `runtime\uxplay.exe` through Windows Firewall on a private network if Windows prompts you.
- Open the **Log** page in Air2Win for the actual startup or connection error.

Air2Win does not change firewall settings automatically. Settings, the receiver identity key, and cache stay in the `data` folder next to the EXE. Closing the application stops the receiver. To remove Air2Win, delete the folder and any firewall rule you created manually.

Screen mirroring and audio are supported. Direct app video casting and DRM-protected video are not. The provided build is for Windows 10/11 x64; Windows x86 and Windows 7/8 are not supported by this package.
