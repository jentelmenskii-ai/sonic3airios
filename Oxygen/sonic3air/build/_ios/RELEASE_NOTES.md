# Sonic 3 A.I.R. for iOS · v0.2.6

- **Replay Recording scope:** Gameplay → Play Modes now offers Each Act, Full Zone,
  Act 1 Only and Act 2 Only. Full Zone starts in Act 1, records both acts and their
  transition without an intermediate prompt, then asks to save at the zone end.
  Single-act zones finish at their exit. The browser labels full-zone recordings.
- **Complete gameplay timeline:** replays retain restarts and checkpoint restarts,
  script pause-menu inputs, deaths, bonus stages, manual Y fast-forward and automatic
  2× cutscenes. Exact running-speed changes are recorded independently from filtered
  gamepad input. Restarting no longer clears an in-progress recording.
- **State events:** native restarts, script level starts inside the same recording
  and external save-state loads are captured at simulation-frame boundaries, with
  RNG/runtime state and cutscene speed. Recorded Options actions reapply recorded
  settings without opening interactive Options during playback.
- **Replay format v2:** bounded state events and per-frame speed/skip metadata.
  Version 1 files remain readable subject to the original compiled-script/mod checks.
  Playback remains isolated from disk progress; pause → Exit returns to the menu.

Tests cover scope selection, two-act and single-act boundaries, speed transitions,
state-event serialization, malformed/truncated files and v1 decoding. iOS compilation
is checked in GitHub Actions. Physical-device gameplay validation is still required;
see docs/REPLAY_TESTING.md. App version 0.2.6, build 12; unsigned sideloaded IPA.
