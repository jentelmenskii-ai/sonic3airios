<div align="center">

## Sonic 3 A.I.R. for iOS · v0.1.2

**Retina display support**

Native C++ engine · iPhone & iPad · Experimental

### [Download unsigned IPA](https://github.com/jentelmenskii-ai/sonic3airios/releases/download/ios-v0.1.2/Sonic3AIR-iOS-unsigned.ipa)

</div>

---

### What's changed

- **Enabled Retina output:** the app window uses the screen's physical backing pixels.
- **Corrected viewport and clipping conversion** from logical coordinates to display pixels.
- **Updated final scaling** to use the physical output resolution.
- **Preserved menu and touch coordinates** and offscreen texture sizes.
- **Added a `Retina output` log entry** showing window and drawable dimensions.

### Install & play

1. Download **Sonic3AIR-iOS-unsigned.ipa** from **Assets** below.
2. Sign and install it using your compatible sideloading method.
3. Launch **Sonic 3 AIR** and select your own compatible, unmodified **Sonic 3 & Knuckles ROM** in Files.

> The ROM is not included or downloaded. This IPA is unsigned and requires signing before installation. Keep the app's Documents folder when updating to preserve saves, settings and mods.

### Mods & saves

Find the app's writable files in **Files → On My iPhone / On My iPad → Sonic 3 AIR**. Extract mod folders into **mods**, then restart the game to scan for them.

### Validation

Native iOS compilation and bundle checks passed. Automated mapping tests cover 1×, 2× and 3× scaling, rotation, fractional edges and zero-size fallback. Visual sharpness and touch alignment still need verification on a device.

### Source & credits

[Source for this version](https://github.com/jentelmenskii-ai/sonic3airios/tree/ios-v0.1.2) · [iOS build guide](https://github.com/jentelmenskii-ai/sonic3airios/blob/ios-native/Oxygen/sonic3air/build/_ios/README.md) · [Original Sonic 3 A.I.R.](https://sonic3air.org/)

An unofficial, non-profit port of Eukaryot's Sonic 3 A.I.R. Project license: **GPL-3.0**. Sonic the Hedgehog and related properties belong to SEGA. Not affiliated with SEGA or Sonic Team.
