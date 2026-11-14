# Sonic 3 A.I.R. for iOS · v0.2.5

- **Shared saves:** Options → Gameplay → Play Modes → Save Profile → Shared (Anniversary).
  Both editions use the Anniversary profile, including unlocks, time-attack records and
  save states. Separate Classic data is preserved and restored by choosing Separate.
- **Speedrun Mode:** automatically fast-forwards marked skippable cutscenes at 2×,
  then returns to 1×. Pause and save prompts remain paused.
- **Replay Mode:** asks whether to save after each completed Normal Game/Act Select act.
  Recordings restore the full initial simulation state, RNG, settings, storage snapshot
  and the exact current/previous input for all four players on every frame.
- **Extras → Replays:** zone browser, act time, edition and recording date; A to play,
  Y to delete with confirmation. Playback pauses with Start; Exit Game returns to the
  main menu. Playback ends at the final recorded frame and cannot award achievements
  or write save data. Both editions share the replay browser.
- Recordings validate a versioned checksum and compiled script/mod compatibility.
  Corrupt or incompatible recordings cannot start. Maximum length is three hours per act.

Validation: host C++ syntax checks and profile/storage/replay format tests. Native iOS
build is checked by GitHub Actions. Real-device playthrough checks remain necessary;
see `docs/REPLAY_TESTING.md`. The unsigned IPA requires sideloading. Keep Documents
when upgrading to preserve existing saves and mods.
