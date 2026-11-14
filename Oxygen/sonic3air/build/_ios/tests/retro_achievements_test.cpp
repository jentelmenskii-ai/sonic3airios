#include "sonic3air/data/RetroAchievements.h"
#include <cassert>
#include <set>
#include <iostream>
using namespace retroachievements;
struct Harness {
    Tracker tracker;
    Snapshot s;
    std::set<unsigned> earned;
    Harness() { s.normal = s.active = s.countersActive = true; s.character = 1; s.lives = 3; }
    void event(Event e, unsigned a=0, unsigned b=0) {
        tracker.process(e, s, a, b, [&](unsigned id) { earned.insert(id-ID_BASE); });
    }
    bool has(Index i) const { return earned.count(i); }
    void start(unsigned act) { s.zoneAct=act; s.time=0; s.checkpoint=0; event(LEVEL_START); }
};
int main() {
    std::set<unsigned> coverage;
    const auto record = [&](const Harness& h) { coverage.insert(h.earned.begin(), h.earned.end()); };
    // All progression / completion events, with actual S3&K internal zone IDs.
    for (unsigned act : {0x0001,0x0101,0x0201,0x0301,0x0501,0x0601,0x0701,0x0401,0x0801,0x1600,0x0a00}) {
        Harness h; h.s.shield=0x20; h.event(ACT_FINISH,act); record(h);
    }
    { Harness h; h.event(ACT_FINISH,0x0b01); assert(h.has(DEZ)); record(h); }
    for (unsigned c : {0,1,2,3,4}) {
        Harness h; h.s.character=c; h.s.superEmeralds=7; h.event(GAME_FINISH); record(h);
        if (c==2) assert(h.has(TAILS_END) && !h.has(SONIC_END));
        if (c==3 || c==4) assert(h.has(KNUCKLES_END) && !h.has(DEZ));
    }
    { Harness h; h.event(HPZ_WIN); record(h); }
    { Harness h; h.s.character=3; h.event(HPZ_WIN); assert(!h.has(HPZ)); }
    { Harness h; h.s.rings=400; h.s.time=599*60+59; h.event(ACT_FINISH,0x0601); assert(h.has(LAST_SECOND)); record(h); }
    { Harness h; h.s.time=120*60-1; h.event(ACT_FINISH,0); assert(h.has(AIZ_FAST)); record(h); }
    { Harness h; h.s.time=120*60; h.event(ACT_FINISH,0); assert(!h.has(AIZ_FAST)); }
    { Harness h; h.s.shield=0x40; h.s.time=180*60-1; h.event(ACT_FINISH,0x0700); assert(h.has(MHZ_BUBBLE)); record(h); }
    { Harness h; h.s.shield=0x10; h.event(ACT_FINISH,0x0801); record(h); }
    { Harness h; h.start(0x0200); h.event(ACT_FINISH,0x0200); assert(h.has(NO_DAMAGE)); record(h); }
    // Shield-only hits, deaths, transformations and bonus entries disqualify
    // the entire attempt, even after a checkpoint or bonus-stage return.
    for (Event fail : {DAMAGE,DEATH,BONUS_ENTER,INVALIDATE}) {
        Harness h; h.start(0x0200); h.event(fail); h.s.checkpoint=1;
        h.event(LEVEL_START); h.event(ACT_FINISH,0x0200); assert(!h.has(NO_DAMAGE));
    }
    { Harness h; h.start(0x0200); h.s.transformed=true; h.event(FRAME);
      h.s.transformed=false; h.event(ACT_FINISH,0x0200); assert(!h.has(NO_DAMAGE)); }
    { Harness h; h.start(0x0600); h.event(ACT_FINISH,0x0600); assert(h.has(NO_LEFT)); record(h); }
    { Harness h; h.start(0x0600); h.s.input=4; h.event(FRAME); h.s.input=0;
      h.event(ACT_FINISH,0x0600); assert(!h.has(NO_LEFT)); }
    { Harness h; h.s.checkpoint=3; h.s.zoneAct=0x0600; h.event(LEVEL_START);
      h.event(ACT_FINISH,0x0600); assert(!h.has(NO_LEFT)); }
    { Harness h; h.start(0x0b00); h.start(0x0b01); h.event(DEZ_ROBO); assert(h.has(DEATHLESS)); record(h); }
    for (Event fail : {DEATH,BONUS_ENTER,INVALIDATE}) {
        Harness h; h.start(0x0b00); h.event(fail); h.start(0x0b01);
        h.event(DEZ_ROBO); assert(!h.has(DEATHLESS));
    }
    { Harness h; h.start(0x0c00); h.s.rings=200; h.event(FRAME); record(h); }
    for (unsigned lives : {10,20}) {
        Harness h; h.start(0); h.s.lives=lives; h.event(FRAME); record(h);
    }
    { Harness h; h.s.lives=20; h.start(0); h.event(FRAME); assert(!h.has(LIVES20)); }
    { Harness h; h.start(0); h.event(BONUS_ENTER); h.s.active=false; h.s.rings=200; h.s.lives=10;
      h.event(FRAME); assert(h.has(RINGS200) && h.has(LIVES10)); }
    { Harness h; h.start(0); h.s.active=h.s.countersActive=false; h.s.rings=200; h.s.lives=10;
      h.event(FRAME); assert(!h.has(RINGS200) && !h.has(LIVES10)); }
    { Harness h; h.start(0x0001); h.event(LIFE_MONITOR,0x1ba8,0x203); record(h); }
    { Harness h; h.start(0x0001); h.event(LIFE_MONITOR,0x1ba8,0x5c0); assert(!h.has(HIDDEN_LIFE)); }
    { Harness h; h.start(0x0100); h.s.x=0x2600; h.s.y=0x340; h.event(FRAME); record(h); }
    { Harness h; h.start(0x0501); h.s.x=0x4000; h.s.y=0x3c0; h.event(FRAME); record(h); }
    { Harness h; h.start(0x0700); h.s.y=0xfffa; h.s.cameraY=0; h.event(FRAME); record(h); }
    { Harness h; h.start(0x0700); h.s.y=0x100; h.s.cameraY=0x200; h.event(FRAME); assert(!h.has(MHZ_TOP)); }
    { Harness h; h.event(JACKPOT_EVENT); h.event(GLOW_EXIT_EVENT); record(h); }
    { Harness h; h.event(BONUS_ENTER); for(unsigned v : {0x10,0x20,0x40,0x10,0x20}) h.event(SHIELD_CHANGE,v); assert(h.has(SHIELDS)); record(h); }
    { Harness h; h.s.shield=0x11; h.event(BONUS_ENTER); for(int i=0;i<5;++i) h.event(SHIELD_CHANGE,0x10); assert(!h.has(SHIELDS)); }
    { Harness h; h.event(BONUS_ENTER); for(int i=0;i<20;++i) h.event(SHIELD_CHANGE,0x10); assert(!h.has(SHIELDS)); }
    { Harness h; h.s.blueRemaining=102; h.event(SPECIAL_START,0,0); h.event(SPECIAL_FRAME,0xff00);
      h.s.blueRemaining=0; h.s.ringsRemaining=0; h.s.chaos=7; h.event(SPECIAL_WIN,0,0);
      h.s.superEmeralds=7; h.event(SPECIAL_WIN,0,1); record(h); }
    for(Event fail : {PAUSE,INVALIDATE}) {
        Harness h; h.s.blueRemaining=102; h.event(SPECIAL_START); h.event(SPECIAL_FRAME,0xff00);
        h.event(fail); h.s.ringsRemaining=0; h.event(SPECIAL_WIN); assert(!h.has(BACKWARDS));
    }
    { Harness h; h.event(SPECIAL_START); h.event(SPECIAL_FRAME,0xff00); h.event(SPECIAL_FRAME,0x100);
      h.s.ringsRemaining=0; h.event(SPECIAL_WIN); assert(!h.has(BACKWARDS)); }
    { Harness h; h.s.normal=false; h.s.competition=true; h.s.character=0;
      for(unsigned zone : {0x0e,0x0f,0x11,0x10,0x12}) h.event(COMPETITION_FINISH,zone,2999);
      assert(h.has(GRAND_PRIX)); record(h); }
    { Harness h; h.s.normal=false; h.s.competition=true;
      h.event(COMPETITION_FINISH,0x0e,3000); assert(!h.has(AZURE)); }
    { Harness h; h.event(CHEAT_EVENT); h.s.rings=400; h.event(FRAME); h.event(GAME_FINISH);
      assert(h.has(CHEAT) && h.earned.size()==1); record(h); }
    { Harness h; h.s.normal=false; h.s.rings=400; h.event(FRAME); h.event(GAME_FINISH); assert(h.earned.empty()); }
    // Every imported entry is exercised by at least one successful event path.
    assert(coverage.size()==COUNT);
    unsigned points=0; for(const auto& e : CATALOG) points+=e.points;
    assert(points==546);
    std::cout << "53 local achievements: all award paths, challenge invalidation and boundaries passed\n";
}
