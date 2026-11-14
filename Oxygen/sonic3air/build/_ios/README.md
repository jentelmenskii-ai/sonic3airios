# Experimental native iOS port

This target compiles the C++ Oxygen Engine, LemonScript interpreter and SDL2 for
arm64 iOS. It does not use WebKit, WebView, Emscripten or a web server.

Status: the user confirmed version 0.1.1 launches and plays on iPhone. Version
0.1.2 enables Retina output to address blurred low-resolution presentation.
The Retina change still needs device testing; not all features/mods are verified.

## Download

The regular app now includes **Classic Mode / Anniversary Mode** under **Mods**.
Download the unsigned IPA from [v0.2.3](https://github.com/jentisalive/sonic3airios/releases/tag/ios-v0.2.3).

Classic Mode uses 320 × 224 gameplay with Genesis-style pixel-aspect correction for 4:3 and a 30 FPS presentation
cap with 60 Hz logic. Classic menus use the original 224-line layout, shifted inward to fit the 320-pixel field. Its progress,
time-attack records and save states live in `Documents/classic/` by default.
Choose **Options → Gameplay → Play Modes → Save Profile → Shared (Anniversary)**
to use the Anniversary profile in both modes without deleting the separate Classic profile. Anniversary
uses the original save locations. Switching modes returns to the intro/title screen.
The selection is saved across restarts.

Classic locks the six visual settings shown in the release notes. Anniversary
preferences are retained separately from these effective values. ROM and mods
remain shared. This is an update to the regular app, not the standalone 4x3 build.

## Build from Windows using GitHub Actions

1. Open the repository's **Actions** tab. If GitHub has disabled workflows for the
   new fork, enable them with **I understand my workflows, go ahead and enable them**.
2. Select **Native iOS IPA**, then **Run workflow** on the `ios-native` branch
   (or `main` after merging the changes).
3. Download the `Sonic3AIR-iOS-unsigned` artifact from the completed run and unzip
   it to obtain `Sonic3AIR-iOS-unsigned.ipa`.
4. Sign/install the IPA with your compatible iOS installation method. The archive
   is unsigned; it is not an App Store or TestFlight release. This workflow does
   not require Apple signing certificates or a ROM in GitHub secrets.

On first launch, choose your compatible, unmodified Sonic 3 & Knuckles ROM from
Files. The app validates it with the same checksum and compatibility byte as the
engine and keeps a copy in Documents. No ROM is included or downloaded.

Files → On My iPhone → Sonic 3 AIR contains the `mods` folder, settings, saves and
`logfile.txt`. Unzip mods into `mods` using the usual Sonic 3 A.I.R. layout.
The empty Mods screen also offers **Open mods folder**. This presents the system
Files picker with `Documents/mods` as its initial directory, creating the folder
if needed. iOS may restore another location; the path above remains available
manually. Selecting a file in this browser does not import or replace it.
Restart the game after adding mods so it scans the new folders.
The engine's existing mobile touch controls are used.

## Local build on macOS

```sh
cmake -S Oxygen/sonic3air/build/_ios -B build-ios -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0 \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
cmake --build build-ios --config Release --parallel 3
mkdir -p ipa/Payload
cp -R build-ios/Release-iphoneos/Sonic3AIR.app ipa/Payload/
(cd ipa && zip -qry ../Sonic3AIR-iOS-unsigned.ipa Payload)
```

## Changes from the unfinished upstream iOS target

- An independent CMake target builds all vendored dependencies for the iOS SDK;
  it avoids the old Xcode project's hand-maintained source lists.
- SDL's UIKit application lifecycle runs the native entry point.
- Native ROM selection uses UIDocumentPicker; an invalid ROM can be replaced
  before the engine starts.
- Screen rendering binds SDL's UIKit framebuffer, rather than framebuffer zero.
  The handle is queried again for each use so it can change after rotation.
- The render-method limit is OpenGL ES presentation with the software game
  renderer, matching the existing GLES-compatible path.
- Read-only assets and script sources live in the app bundle. Settings, mods,
  save states and script-cache writes use Documents.
- Rendering pauses while iOS marks the app minimized; settings are saved on
  entering the background.
- The bundle declares the minimum iOS version and iPhone/iPad device families,
  including when built with Ninja instead of Xcode.
- CI packages a device build into the required `Payload/*.app` IPA structure.

## Current limits and testing

The editing environment has no physical iPhone. Compilation runs on a macOS
GitHub runner with the iOS SDK; portable changes also pass a Linux C++ syntax
check. Test the IPA on-device. In
particular, validate first-launch ROM selection, landscape rotation, multitouch,
audio interruption/background resume, saves and representative mods.

The initial bundle includes source assets and the remastered soundtrack. Script
compilation on first launch can take longer than an official packaged release.
Automatic downloading/updating is unavailable in this iOS target. Simulator
builds and App Store distribution are outside this initial workflow.

Original project credits and GPL-3.0 license remain in the repository root.
