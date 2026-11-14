# iOS replay and mode regression checklist

Automated host checks: `bash Oxygen/sonic3air/build/_ios/tests/run-profile-test.sh`.
The workflow builds an unsigned arm64 IPA for pull requests targeting `ios-native`.

## Device checks required before release

1. Create different Classic/Anniversary slots. Enable Shared (Anniversary) in the main
   menu's Gameplay options, switch editions, continue/save a slot in both, and restart
   the app. Verify common slots, unlocks and time-attack records. Disable Shared and
   verify the original Classic slots remain intact. Check save-state folder routing.
2. Enable Speedrun Mode and run Sonic/Knuckles introductions and between-zone
   cutscenes. Verify 2× only inside marked cutscenes and 1× immediately afterwards;
   pause, open options, disable Speedrun, return to gameplay, and change editions.
3. Enable Replay Mode before starting Normal Game and Act Select. Complete Act 1,
   save it, complete Act 2 and decline. Verify one recording, then save multiple runs
   of the same act in the same second. Restart the app and verify all saved entries.
4. Exercise seamless AIZ/ICZ act transitions, SOZ/DEZ transitions, Sky Sanctuary and
   Death Egg result-free exits, and final endings. Verify one prompt per completed
   act, no gameplay frames during the prompt, and recording of the next act.
5. Replay a run with two players, death/checkpoint, a bonus stage, manual cutscene
   fast-forward and an in-game options change. Compare timing, position and result
   to the original run. Reopen in the opposite edition and verify recorded layout.
6. Press Start during playback, Continue, then Start → Exit Game. Verify the main
   menu, original edition/settings, original saves and unchanged achievements.
   Let a replay finish naturally and repeat these checks. Force-close during playback
   and verify disk saves are unchanged after relaunch.
7. In Extras → Replays select a zone and act. Verify duration/date at both screen
   widths. Press Y: Cancel must preserve it; Delete must remove just that recording.
   Verify empty lists, Back navigation and scrolling with many entries.
8. Change a script mod or its settings: incompatible recordings must fail gracefully.
   Test a truncated recording and a full/read-only Documents directory. Failed save
   must leave the prompt open so the player can retry or decline.

The host environment cannot run Apple's SDK or the game/ROM on an iPhone. Automated
format/storage tests and a successful IPA compile do not replace these device checks.
