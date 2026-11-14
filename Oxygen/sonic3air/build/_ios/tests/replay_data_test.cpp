#include "sonic3air/data/ReplayData.h"
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
    auto corrupt = bytes; corrupt[4] = 2; resign(corrupt);
    assert(!s3air::ReplayData::decode(corrupt, copy));
    corrupt = bytes;
    // Compatibility string length follows magic/version/act/time/date/flags/cutscene/frequency.
    for (int i=0; i<4; ++i) corrupt[33+i] = 0xff;
    resign(corrupt);
    assert(!s3air::ReplayData::decode(corrupt, copy));
    std::cout << "Replay codec: four-player input, state/settings/storage roundtrip, events, truncation and corruption checks passed\n";
}
