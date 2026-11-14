#include "sonic3air/data/ReplayData.h"
#include "sonic3air/data/ReplayRules.h"
#include <cassert>
#include <iostream>

int main()
{
    s3air::ReplayData original;
    original.zoneAct = 0x0501;
    original.actTime = 7321;
    original.timestamp = 1794650000;
    original.classic = true;
    original.speedrun = true;
    original.cutsceneFrames = 123;
    original.frequency = 60;
    original.compatibility = "game:mod1";
    original.state = {0, 1, 128, 255};
    original.initialInput = {8, 4, 1, 2};
    original.frames = {{8, 4, 1, 2}, {0x40, 0x20, 0x10, 0x80}, {0, 0, 0, 0}};
    original.previousInputs = {{0,0,0,0}, {8,4,1,2}, {0x40,0x20,0x10,0x80}};
    original.controls = {{4000,true}, {2000,false}, {1000,false}};
    original.actions = {{1, 77, true, {9,8,7}}};
    original.scope = s3air::ReplayScope::FULL_ZONE;
    original.actTimes = {3600, 3721};
    original.settings = {{0x80000301, 0}, {0x00000102, 2}};
    original.events = {{1, 0x00000102, 1}, {2, 0xffffffff, 0}};
    original.persistent = {{"s3air_savedata", "slot1", {5, 4, 3}}};
    const auto bytes = original.encode();
    s3air::ReplayData copy;
    assert(s3air::ReplayData::decode(bytes, copy));
    assert(copy.encode() == bytes);
    assert(copy.frames == original.frames && copy.initialInput == original.initialInput);
    assert(copy.classic && copy.speedrun && copy.zoneAct == 0x0501);
    assert(copy.persistent[0].bytes == original.persistent[0].bytes);
    assert(copy.events[1].frame == 2 && copy.events[1].key == 0xffffffff);
    // Every truncation, and every single-byte corruption, must be rejected without
    // mutating the caller's already-loaded recording.
    for (size_t i = 0; i < bytes.size(); ++i) {
        auto truncated = bytes; truncated.resize(i);
        assert(!s3air::ReplayData::decode(truncated, copy));
        assert(copy.encode() == bytes);
        auto corrupted = bytes; corrupted[i] ^= 0x80;
        assert(!s3air::ReplayData::decode(corrupted, copy));
    }
    assert(copy.scope == s3air::ReplayScope::FULL_ZONE && copy.actTimes[1] == 3721);
    assert(copy.actions[0].frame == 1 && copy.actions[0].state == std::vector<uint8_t>({9,8,7}));
    assert(copy.controls[0].manualSkip && copy.controls[0].speedMilli == 4000);
    auto invalid = original;
    invalid.events = {{3, 123, 1}};
    assert(!s3air::ReplayData::decode(invalid.encode(), copy));
    invalid.events = {{2, 123, 1}, {1, 123, 2}};
    assert(!s3air::ReplayData::decode(invalid.encode(), copy));
    invalid = original; invalid.frequency = 0;
    assert(!s3air::ReplayData::decode(invalid.encode(), copy));
    invalid = original; invalid.frames.clear();
    assert(invalid.encode().empty());
    invalid = original; invalid.state.clear();
    assert(invalid.encode().empty());
    // Correct checksum must not allow unknown format versions or oversized strings.
    auto resign = [](std::vector<uint8_t>& data) {
        const auto hash = s3air::ReplayData::checksum(data, data.size() - 8);
        for (size_t i = 0; i < 8; ++i) data[data.size()-8+i] = uint8_t(hash >> (i*8));
    };
    auto corrupt = bytes; corrupt[4] = 3; resign(corrupt);
    assert(!s3air::ReplayData::decode(corrupt, copy));
    corrupt = bytes;
    // Compatibility string length follows magic/version/act/time/date/flags/cutscene/frequency.
    for (int i=0; i<4; ++i) corrupt[33+i] = 0xff;
    resign(corrupt);
    assert(!s3air::ReplayData::decode(corrupt, copy));
    invalid = original; invalid.actions[0].frame = 3;
    assert(!s3air::ReplayData::decode(invalid.encode(), copy));
    invalid = original; invalid.actions = {{2,0,false,{1}}, {1,0,false,{2}}};
    assert(!s3air::ReplayData::decode(invalid.encode(), copy));
    invalid = original; invalid.actions[0].state.clear();
    assert(!s3air::ReplayData::decode(invalid.encode(), copy));
    invalid = original; invalid.controls[0].speedMilli = 0;
    assert(!s3air::ReplayData::decode(invalid.encode(), copy));
    invalid = original; invalid.scope = s3air::ReplayScope(9);
    assert(!s3air::ReplayData::decode(invalid.encode(), copy));
    assert(s3air::replaySpeedMilli(4.0f) == 4000);
    assert(s3air::replaySpeedMilli(2.0f) == 2000);
    assert(s3air::replaySpeedMilli(1.0f) == 1000);
    assert(s3air::replaySpeedMilli(0.0f, 4000) == 4000);
    assert(s3air::replaySpeedMilli(-1.0f, 2000) == 2000);
    assert(s3air::replaySpeedMilli(100.0f) == 32000);
    using S = s3air::ReplayScope;
    for (unsigned zone : {0u, 5u, 11u}) {
        const uint16_t a = uint16_t(zone << 8), b = a | 1;
        assert(s3air::replayShouldStart(S::EACH_ACT,a) && s3air::replayShouldStart(S::EACH_ACT,b));
        assert(s3air::replayShouldStart(S::FULL_ZONE,a) && !s3air::replayShouldStart(S::FULL_ZONE,b));
        assert(s3air::replayShouldStart(S::ACT_1,a) && !s3air::replayShouldStart(S::ACT_1,b));
        assert(!s3air::replayShouldStart(S::ACT_2,a) && s3air::replayShouldStart(S::ACT_2,b));
        assert(!s3air::replayShouldFinish(S::FULL_ZONE,a,a,2,false));
        assert(s3air::replayShouldFinish(S::FULL_ZONE,a,b,2,false));
        assert(s3air::replayShouldFinish(S::FULL_ZONE,a,a,1,false));
        assert(s3air::replayShouldFinish(S::FULL_ZONE,a,a,2,true));
        assert(!s3air::replayShouldFinish(S::FULL_ZONE,a,uint16_t(a+0x100),2,true));
        assert(s3air::replayShouldFinish(S::EACH_ACT,a,a,2,false));
        assert(!s3air::replayShouldFinish(S::ACT_1,a,b,2,false));
    }
    // Golden v1 file produced by the previous released codec, retained for migration.
    const std::vector<uint8_t> legacy = {83,51,82,80,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,60,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,1,2,3,1,0,0,0,8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,74,234,220,106,29,228,236};
    assert(s3air::ReplayData::decode(legacy, copy));
    assert(copy.formatVersion == 1 && copy.frames.size() == 1 && copy.zoneAct == 0x101);
    assert(copy.scope == S::EACH_ACT && copy.actions.empty());
    std::cout << "Replay v2: four-player input, state actions, cutscene speeds, scope boundaries, v1 migration and corruption checks passed\n";
}
