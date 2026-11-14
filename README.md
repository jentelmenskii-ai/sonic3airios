<div align="center">

<img src="Oxygen/sonic3air/build/_xcode/sonic3air_iosApp/Assets.xcassets/AppIcon.appiconset/180.png" width="112" alt="Sonic 3 A.I.R. icon" />

# Sonic 3 A.I.R. for iOS

**Angel Island, on your iPhone and iPad.**

An experimental native port of Sonic 3 A.I.R., powered by the Oxygen Engine, SDL2 and UIKit.

![Platform](https://img.shields.io/badge/platform-iOS_13%2B-147EFB?style=flat-square)
![Architecture](https://img.shields.io/badge/architecture-arm64-555555?style=flat-square)
![Status](https://img.shields.io/badge/status-experimental-F5A623?style=flat-square)
[![License](https://img.shields.io/badge/license-GPL--3.0-2EA44F?style=flat-square)](LICENSE)

**[Download v0.2.4](https://github.com/jentisalive/sonic3airios/releases/tag/ios-v0.2.4)** · **[All releases](https://github.com/jentisalive/sonic3airios/releases)** · **[Build guide](Oxygen/sonic3air/build/_ios/README.md)** · **[Original project](https://sonic3air.org/)**

</div>

---

## A native adventure

This port runs the game's C++ engine and LemonScript interpreter directly on iOS, with OpenGL ES presentation. It does not use a web version, WebView or a local web server.

- **Made for touch** — the engine's mobile controls work on iPhone and iPad.
- **Retina output** — window rendering and final scaling use the display's backing pixels.
- **Your files, accessible** — saves, settings and mods live in the app's Documents folder.
- **Mods within reach** — English installation guidance and an **Open mods folder** button.
- **Build from Windows** — GitHub Actions compiles and packages the iOS app on a macOS runner.

> **Bring your own game.** A compatible, unmodified Sonic 3 & Knuckles ROM is required. The ROM is not included or downloaded by the app.

## Get started

| Requirement | Details |
| :--- | :--- |
| Device | An arm64 iPhone or iPad running iOS 13 or later |
| App | `Sonic3AIR-iOS-unsigned.ipa` from a release's **Assets** section |
| Installation | Your compatible signing and sideloading method |
| Game data | Your own compatible Sonic 3 & Knuckles ROM |

1. Download the **[v0.2.4 IPA](https://github.com/jentisalive/sonic3airios/releases/download/ios-v0.2.4/Sonic3AIR-iOS-unsigned.ipa)**.
2. Sign and install the app using your usual method.
3. Launch **Sonic 3 AIR**, tap **Choose ROM…**, and select your ROM in Files.
4. Let the game prepare its scripts on the first launch, then play.

The IPA is unsigned. This is an experimental sideloaded build, rather than an App Store or TestFlight release. When updating, keep the app's Documents folder to preserve your saves and mods.

## Classic Mode & Anniversary Mode

Select **Classic Mode** below **Mods** in the regular app's main menu. The game
returns to the intro/title screen with 4:3 gameplay (320 × 224 with Genesis-style pixel-aspect correction),
30 FPS presentation and the fixed visual preset. Game logic remains at 60 Hz.
Select **Anniversary Mode** in the same place to restore widescreen gameplay and
your original visual preferences. Classic menus share the 320 × 224 field, with their main entries shifted left and original row spacing restored.

By default, Classic progress, time-attack records and save states use **Documents/classic/**.
In **Options → Gameplay → Play Modes → Save Profile**, choose **Shared (Anniversary)**
to use Anniversary's existing progress in both editions. Returning to **Separate**
restores the independent Classic profile. Switching profiles does not merge or overwrite the other profile.
Anniversary keeps the existing save locations. The selected mode is remembered;
ROM and mods remain shared. See the [release notes](Oxygen/sonic3air/build/_ios/RELEASE_NOTES.md)
for the exact locked settings and testing status.

## Speedrun Mode and Replays

Under **Options → Gameplay → Play Modes**, enable **Speedrun Mode** to run marked
skippable cutscenes automatically at 2× speed, returning to 1× when they end.
Enable **Replay Mode** before starting Normal Game or Act Select. After each completed
act, choose **Save Replay** or **Do Not Save** before continuing.

Open **Extras → Replays**, choose a zone, then an act recording. Entries show the act's
completion time, edition and local date/time. Playback restores the recorded game state,
random generator, settings and all four players' inputs; it runs the actual game simulation.
Press Start to pause, then **Exit Game** to return to the main menu. Playback also returns
to the menu when the recording ends. Replay playback never writes progress or achievements.
Press **Y** on a recording and confirm **Delete replay** to delete it; Cancel is selected by default.

Replays live in `Documents/replays/` and are shared between editions. Use the same game
build and enabled mods/settings as the recording; changed script code is rejected.
Recordings are capped at three hours per act. The iOS build still needs device validation;
see the [test checklist](docs/REPLAY_TESTING.md).

## Install mods

Open the app's folder in **Files**:

**On My iPhone / On My iPad → Sonic 3 AIR → mods**

1. Extract the mod archive.
2. Put each mod's folder inside `mods`, keeping its original contents and folder structure.
3. Restart Sonic 3 A.I.R. so it scans for new mods.
4. Open **Mods** in the game and enable the mods you want to use.

When no mods are installed, **Open mods folder** opens the native Files picker with `Documents/mods` as its starting directory. iOS may restore another location; use the path above if needed.

## What's new

| Version | Highlights | Download |
| :--- | :--- | :--- |
| **0.2.4** | 53 offline achievements adapted from the RetroAchievements Sonic 3 & Knuckles set | [Release](https://github.com/jentisalive/sonic3airios/releases/tag/ios-v0.2.4) |
| **0.2.3** | Compact Classic menu layout with original row spacing; scanline safety fix retained | [Release](https://github.com/jentisalive/sonic3airios/releases/tag/ios-v0.2.3) |
| **0.2.0** | Integrated Classic/Anniversary switch, independent saves, 4:3 and 30 FPS | [Release](https://github.com/jentisalive/sonic3airios/releases/tag/ios-v0.2.0) |
| **0.1.3** | English mod instructions and native mods-folder access | [Release](https://github.com/jentisalive/sonic3airios/releases/tag/ios-v0.1.3) |
| **0.1.2** | Retina window rendering, clipping and final scaling | [Release](https://github.com/jentisalive/sonic3airios/releases/tag/ios-v0.1.2) |
| **0.1.1** | Script startup fix and iPhone/iPad app icons | [Release](https://github.com/jentisalive/sonic3airios/releases/tag/ios-v0.1.1) |

## Build it yourself

**On Windows:** open **[Actions → Native iOS IPA](https://github.com/jentisalive/sonic3airios/actions/workflows/ios-native.yml)**, choose **Run workflow**, select `ios-native`, and download the IPA artifact after the run completes. A local Mac is not required for this route.

**On macOS:** use the CMake commands in the **[native iOS build guide](Oxygen/sonic3air/build/_ios/README.md)**. The target builds the vendored dependencies for the iOS SDK and packages source assets without a ROM.

<details>
<summary><strong>Repository map</strong></summary>

| Path | Contents |
| :--- | :--- |
| `Oxygen/sonic3air/build/_ios` | Native entry point, app metadata, CMake target and checks |
| `Oxygen/sonic3air/source` | Game-specific C++ code |
| `Oxygen/sonic3air/scripts` | LemonScript game scripts |
| `Oxygen/oxygenengine` | Oxygen Engine |
| `Oxygen/lemonscript` | LemonScript compiler and interpreter |
| `librmx` | Shared utility and media libraries |
| `framework/external` | Vendored dependencies, including SDL2 |
| `docs/releases` | English release descriptions |

Other platform targets and the original contributor list are documented in the [upstream README](docs/UPSTREAM.md).

</details>

## Testing and troubleshooting

Version 0.1.1 has been confirmed to launch and play on an iPhone. Version 0.1.3 passed native arm64 iOS compilation, bundle/resource/icon checks, the script-profile regression test and Retina rectangle mapping tests.

Retina presentation and the folder browser still need device verification. Not every feature or mod has been tested. Simulator builds and App Store distribution are outside the current workflow.

If something goes wrong, keep `logfile.txt` from the app's Documents folder. Include the app version, device, iOS version, steps to reproduce, and that log when reporting a problem. First-launch script compilation may take longer than in an official packaged release.

## Credits and license

**Sonic 3 A.I.R. and the Oxygen Engine were created by Eukaryot.** This repository is an unofficial iOS port of [Eukaryot/sonic3air](https://github.com/Eukaryot/sonic3air). The remastered soundtrack is by **SpinnRG**. Thanks to the original developers and every contributor; the complete upstream acknowledgments and dependency list are preserved in [docs/UPSTREAM.md](docs/UPSTREAM.md).

The project is distributed under **[GNU GPL v3](LICENSE)**. See the original license files for third-party components.

Sonic 3 A.I.R. is a non-profit fan game and is not affiliated with SEGA or Sonic Team. Sonic the Hedgehog and related characters, names, art and music belong to SEGA and their respective rights holders. No financial gain is made from this project. Any commercial use without SEGA's explicit consent is strictly prohibited. The [original project disclaimer](docs/UPSTREAM.md#disclaimer) is preserved with the upstream documentation.

### More achievements

**53 additional local challenges** from the [RetroAchievements Sonic 3 & Knuckles set](https://retroachievements.org/game/4874) join the original 18 AIR achievements under **Extras → Achievements**. Unlocks appear during play and persist in the current edition's save profile. English descriptions explain each challenge; point values are local reference values.

This is an offline adaptation to AIR's native game events, with no RetroAchievements login, online points, leaderboard submission or account synchronization. The original set was designed by **Totaya, blazethecat and RetroAchievements contributors**. The port uses AIR's existing artwork and rewritten descriptions.

Imported challenges are earned in **Normal Game**, with race challenges in **Competition Mode**. AIR Act Select and AIR Time Attack do not award them. Level Select / Debug unlocks *Pulling a Fast One?* and blocks other imported achievements until you return to the main menu and begin another session. Loading a save state cancels ongoing restricted challenges. These local rules do not run the website's emulator trigger definitions.

See the [implementation and device checklist](docs/ACHIEVEMENTS.md) for the complete native event mapping.
