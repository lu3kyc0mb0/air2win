# Third-party components

These notices apply to the Air2Win 1.0.0 portable Windows x64 build.

Air2Win's original C++/QML shell is covered by the repository's MIT license.
That license does not replace the licenses of any bundled components.

## Receiver helper

The separate `runtime/uxplay.exe` is built from
[UxPlay](https://github.com/FDH2/UxPlay), revision
`82ad05ce34ec862851fa6ab1240f50f58fb7fa5a`, with the changes in
`scripts/patch-uxplay.py`. UxPlay is GPL-3.0; its bundled components also carry
their own notices. Preserve its LICENSE and sources when distributing builds.
The bridge adds line-based status output, graceful stdin shutdown, and failure
handling for an unavailable control port. It does not claim Apple certification.

## UI and media libraries

- Qt 6.8.3: dynamically linked open-source Qt modules, with applicable LGPL/GPL
  and third-party notices. Source: https://download.qt.io/archive/qt/6.8/6.8.3/
- GStreamer, FFmpeg, OpenSSL, libplist and their dependencies: obtained from
  MSYS2 UCRT64. Package filenames, versions and SHA-256 hashes are saved in
  `runtime/packages.lock.json`. Package-provided licenses are copied into
  `runtime/licenses/`. Some FFmpeg components use GPL licensing.
- Qt MinGW and MSYS2 GCC runtimes retain their upstream licenses and runtime
  exceptions where applicable.

This build is a local development release. Before public binary distribution, assemble
the complete corresponding source and notices required by the actual package
set, including the modified helper, MSYS2 package build recipes and any LGPL
relinking requirements. Package metadata: https://packages.msys2.org/
Build recipes: https://github.com/msys2/MINGW-packages

AirPlay is a trademark of Apple Inc. Air2Win is an independent project.
