# Local achievement adaptation

The iOS port adds the 53 challenges in [RetroAchievements game 4874](https://retroachievements.org/game/4874) to the existing AIR achievement menu. Challenge design is credited to **Totaya, blazethecat and RetroAchievements contributors**. This implementation contains rewritten descriptions and AIR's existing achievement images.

These are local unlocks: no login, account sync, official points, hardcore verification or leaderboard submission. Point labels are reference values only. The implementation observes native game events instead of running the original emulator memory expressions.

## Progress and eligibility

- The original 18 achievement IDs and unlock states remain intact. Imported IDs occupy `0x10000..0x10034`.
- Both sets use `PlayerProgress` and the existing persistent achievement map. Classic uses `Documents/classic/`; Anniversary uses the existing Documents profile.
- Imported adventure challenges require Normal Game. Competition challenges require Competition Mode. AIR Act Select and AIR Time Attack are excluded.
- Original AIR secret thresholds count only original AIR achievements.
- Debug / original Level Select awards the cheat achievement and blocks other imported unlocks for that session. Begin a new session from the main menu to clear the block.
- Loading a save state invalidates restricted attempts. Starting from a checkpoint cannot create a fresh no-damage / no-Left attempt.
- Challenge tracking runs at simulation frequency, including Classic's 60 Hz simulation / 30 FPS presentation. No legacy game RAM is written by the tracker.

## Native event mapping

| Challenge | Trigger family | Local objective | Reference points |
|---|---|---|---:|
| Trouble in Paradise | Zone clear | Clear both acts of Angel Island. | 5 |
| Speed Wash | Zone clear | Clear both acts of Hydrocity. | 5 |
| Garden Variety | Zone clear | Clear both acts of Marble Garden. | 5 |
| All Fun and Games | Zone clear | Clear both acts of Carnival Night. | 5 |
| Snowy Surfing | Zone clear | Clear both acts of Ice Cap. | 5 |
| Mobius, We Have a Problem! | Zone clear | Clear both acts of Launch Base. | 5 |
| Fungi Fields | Zone clear | Clear both acts of Mushroom Hill. | 5 |
| Electric Dreadnought | Zone clear | Clear both acts of Flying Battery. | 10 |
| Pyramid Problems | Zone clear | Clear both acts of Sandopolis. | 10 |
| Magma Massacre | Zone clear | Clear Lava Reef, including its boss. | 10 |
| Angel Island Prophecy | Zone clear | Win the Hidden Palace fight against Knuckles as Sonic or Tails. | 5 |
| Mechanized Memories | Zone clear | Clear Sky Sanctuary as Sonic or Tails. | 10 |
| Another Scrambled Egg | Zone clear | Clear Death Egg. | 10 |
| The Blue Blur | Ending | Finish the adventure with Sonic. | 10 |
| Sonic's Flying Colors | Ending | Finish with Sonic and all Chaos and Super Emeralds. | 25 |
| The Red Rumbler | Ending | Finish the adventure with Knuckles. | 10 |
| Knuckles' Flying Colors | Ending | Finish with Knuckles and all Chaos and Super Emeralds. | 25 |
| The Twin-Tailed Tyke | Ending | Finish the adventure with Tails. | 10 |
| Tails' Flying Colors | Ending | Finish with Tails and all Chaos and Super Emeralds. | 25 |
| Not-So-Slow Burn | Act result | Clear Angel Island Act 1 before 2:00. | 10 |
| Aeronautics Funding | Act result | Finish Launch Base Act 2 holding 100 or more rings. | 10 |
| Soap Shoes | Act result | Finish Mushroom Hill Act 1 before 3:00 while wearing a Bubble Shield. | 25 |
| Hot-Tempered Temple | Act result | Finish Sandopolis Act 2 while wearing a Flame Shield. | 10 |
| Tectonic Thunder | Act result | Finish Lava Reef Act 2 as Sonic or Tails while wearing an Electric Shield. | 10 |
| Golden Flight | Gameplay | Hold 100 rings during The Doomsday. | 10 |
| Emerald Cost | Special stage | Earn Perfect and clear a Blue Spheres special stage. | 5 |
| Ooh, Shiny! | Special stage | Obtain one Chaos Emerald. | 5 |
| Bring Forth Chaos | Special stage | Obtain the seventh Chaos Emerald. | 10 |
| Hyper-Fixation | Special stage | Obtain the seventh Super Emerald. | 25 |
| Rings-a-Plenty | Gameplay | Hold at least 100 rings. | 5 |
| Ring Ruler | Gameplay | Hold at least 200 rings. | 10 |
| Dashing Decathlon | Gameplay | Increase your life count to exactly 10. | 5 |
| Live and Learn | Gameplay | Increase your life count to exactly 20. | 5 |
| Azure Lake Time Trial | Competition | Finish five laps of Azure Lake before 0:30 in Competition. | 5 |
| Balloon Park Time Trial | Competition | Finish five laps of Balloon Park before 0:50 in Competition. | 5 |
| Chrome Gadget Time Trial | Competition | Finish five laps of Chrome Gadget before 0:45 in Competition. | 5 |
| Desert Palace Time Trial | Competition | Finish five laps of Desert Palace before 0:40 in Competition. | 5 |
| Endless Mine Time Trial | Competition | Finish five laps of Endless Mine before 1:00 in Competition. | 5 |
| Speedy Spelling Bee | Competition | Finish all five Grand Prix tracks with a combined time below 4:00. | 10 |
| Get a Life | Monitor | Open the concealed 1-Up monitor high above Sonic's route in Angel Island Act 2. | 5 |
| Gambling Is a Skill | Bonus stage | Win the Slot Machine jackpot. | 10 |
| I'm Outta Here! | Bonus stage | Leave the Glowing Spheres bonus stage through its top. | 5 |
| High on 'Shrooms | Gameplay | Climb or fly above the top of Mushroom Hill. | 10 |
| Michael Jackson Style | Special stage | Clear the first Sonic 3 special stage with Perfect, travelling backwards without pausing. | 25 |
| The Element of Choice | Bonus stage | Switch to a different elemental shield five times in one Gumball Machine visit. | 5 |
| Deathless Egg | Boss encounter | Start Death Egg Act 1 and reach Giant Eggman Robo without death, bonus stages or Super/Hyper forms. | 25 |
| Frozen Assets | Gameplay | Discover the Ice Cap Act 2 room with 48 rings and a Giant Ring. | 5 |
| In the So-nick of Time | Act result | Finish an act while the timer reads 9:59. | 10 |
| Left to Launch | Act result | Finish either Launch Base act without ever pressing Left during that act. | 25 |
| Put Through the Ringer | Act result | Finish an act holding 400 or more rings. | 10 |
| Loan Shark | Gameplay | Visit the hidden 90-ring trapezoid in Hydrocity Act 1. | 10 |
| Keeping Your Marbles | Act result | Clear Marble Garden Act 1 without damage, bonus stages or Super/Hyper forms. | 25 |
| Pulling a Fast One? | Cheat | Use Level Select or Debug Mode. Other imported achievements stop for this session. | 1 |

## Adaptation details

Act results are sampled before the result tally changes the player's rings or score. Sonic/Tails Lava Reef completion is checked after its separate boss segment; Knuckles uses his own Act 2 result. Hidden Palace uses the Knuckles boss defeat event. Sky Sanctuary and Death Egg also use their outgoing zone transitions, so they do not depend on showing a result screen or surviving Doomsday. Character endings use the existing AIR ending event. Special-stage Perfect is checked when the emerald is actually collected.

The backwards challenge allows the starting approach before the first collection, then requires backwards movement; moving forward after collection / reversing or opening pause cancels it. It is limited to the first Sonic 3 special stage. Generated Blue Sphere stages and the standalone Blue Sphere extra are excluded by eligibility.

Life milestones require an observed increase to 10 or 20, rather than loading a file that already has that count. Competition timers are decoded from packed minute/second/centisecond fields. Grand Prix requires all five distinct tracks in the same series, without counting duplicate finish events.

Spatial checks use the original S3&K ring and object placements, verified against [sonicretro/skdisasm](https://github.com/sonicretro/skdisasm/tree/73de97c6b55055bcb8089c1d55d006b02136f163/Levels). The Hydrocity trapezoid has 90 ring entries between x=0x2570..0x2738, y=0x320..0x380. Ice Cap's room uses the matching 48-ring grids at x=0x3fac..0x4054 and its Giant Ring. Custom level-layout mods can move these locations.

## Validation

`retro_achievements_test.cpp` executes the production tracker: every one of the 53 award paths, exact timer boundaries, shield-only damage, deaths, bonus returns, transformations, repeated shield types, checkpoint starts, pause, save-state invalidation, cheat blocking, and excluded game modes. It verifies that the catalog totals 546 reference points.

Existing profile tests exercise real PersistentData files across both editions, including separate achievement slots. C++ syntax checks cover the modified native game, registry and menu files. CI builds the arm64 iOS app and checks its resources.

On-device verification remains necessary: open Extras → Achievements in both editions, earn ring and zone achievements, quit/relaunch to check persistence, and test the location, special-stage and Competition challenges during real play. Test conditions describe the local adaptation; passing unit tests does not establish parity with every original RA memory trigger.
