#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "sonic3air/data/ReplayRules.h"

namespace s3air
{
using std::size_t;
// Explicit little-endian format: independent of host ABI, with bounded parsing.
struct ReplayData
{
    struct StateEvent {
        uint32_t frame = 0, cutsceneFrames = 0;
        bool manualSkip = false;
        std::vector<uint8_t> state;
        uint16_t speedMilli = 1000;
    };
    // One entry per simulated frame: speed after the frame and manual Y before filtering.
    struct FrameControl { uint16_t speedMilli = 1000; bool manualSkip = false; };
    uint32_t formatVersion = 2; // Runtime-only; v1 has no speed/action timeline.
    ReplayScope scope = ReplayScope::EACH_ACT;
    std::array<uint32_t, 2> actTimes{};
    uint16_t initialSpeedMilli = 1000;
    std::vector<StateEvent> actions;
    std::vector<FrameControl> controls;
    struct SettingEvent { uint32_t frame, key, value; };
    std::vector<SettingEvent> events;
    using Input = std::array<uint16_t, 4>;
    struct StoredValue { std::string file, key; std::vector<uint8_t> bytes; };
    static constexpr uint32_t MAX_FRAMES = 60 * 60 * 60 * 3;
    static constexpr size_t MAX_BYTES = 64 * 1024 * 1024;
    uint16_t zoneAct = 0;
    uint32_t actTime = 0, cutsceneFrames = 0, frequency = 60;
    uint64_t timestamp = 0;
    bool classic = false, speedrun = false, normalGame = true;
    std::string compatibility;
    Input initialInput{};
    std::vector<uint8_t> state;
    std::vector<Input> frames;
    std::vector<Input> previousInputs;
    std::vector<std::pair<uint32_t, uint32_t>> settings;
    std::vector<StoredValue> persistent;

    static uint64_t checksum(const std::vector<uint8_t>& bytes, size_t length)
    {
        uint64_t hash = 14695981039346656037ull;
        for (size_t i = 0; i < length; ++i) { hash ^= bytes[i]; hash *= 1099511628211ull; }
        return hash;
    }
    std::vector<uint8_t> encode() const
    {
        if (controls.size() != frames.size() || actions.size() > 256 || previousInputs.size() != frames.size() || state.empty() || frames.empty() || frames.size() > MAX_FRAMES || settings.size() > 4096 || persistent.size() > 16384 || events.size() > MAX_FRAMES) return {};
        std::vector<uint8_t> out;
        auto number = [&](uint64_t n, size_t size) { for (size_t i = 0; i < size; ++i) out.push_back(uint8_t(n >> (8*i))); };
        auto blob = [&](const auto& data) { number(data.size(), 4); out.insert(out.end(), data.begin(), data.end()); };
        number(0x50523353, 4); number(2, 4); // S3RP, version 2
        number(zoneAct, 2); number(actTime, 4); number(timestamp, 8);
        number(classic, 1); number(speedrun, 1); number(normalGame, 1);
        number(cutsceneFrames, 4); number(frequency, 4); blob(compatibility);
        for (auto n : initialInput) number(n, 2);
        number(settings.size(), 4);
        for (auto p : settings) { number(p.first, 4); number(p.second, 4); }
        number(persistent.size(), 4);
        for (const auto& entry : persistent) { blob(entry.file); blob(entry.key); blob(entry.bytes); }
        number(events.size(), 4);
        for (const auto& event : events) { number(event.frame, 4); number(event.key, 4); number(event.value, 4); }
        number(uint8_t(scope), 1); number(initialSpeedMilli, 2);
        for (auto time : actTimes) number(time, 4);
        number(actions.size(), 4);
        for (const auto& action : actions) {
            number(action.frame, 4); number(action.cutsceneFrames, 4); number(action.manualSkip, 1); number(action.speedMilli, 2); blob(action.state);
        }
        blob(state); number(frames.size(), 4);
        for (size_t f = 0; f < frames.size(); ++f) {
            for (auto n : frames[f]) number(n, 2);
            for (auto n : previousInputs[f]) number(n, 2);
            number(controls[f].speedMilli, 2); number(controls[f].manualSkip, 1);
            if (out.size() > MAX_BYTES - 8) return {};
        }
        if (out.size() + 8 > MAX_BYTES) return {};
        number(checksum(out, out.size()), 8);
        return out;
    }
    static bool decode(const std::vector<uint8_t>& bytes, ReplayData& result)
    {
        if (bytes.size() < 64 || bytes.size() > MAX_BYTES) return false;
        size_t pos = 0;
        bool valid = true;
        auto number = [&](size_t size) -> uint64_t {
            if (pos > bytes.size() || size > bytes.size() - pos) { valid = false; return 0; }
            uint64_t n = 0; for (size_t i = 0; i < size; ++i) n |= uint64_t(bytes[pos++]) << (8*i);
            return n;
        };
        const size_t end = bytes.size() - 8;
        pos = end;
        const uint64_t expected = number(8);
        if (checksum(bytes, end) != expected) return false;
        pos = 0;
        auto blob = [&](auto& data, size_t limit) {
            const uint64_t count = number(4);
            if (!valid || count > limit || pos > end || count > end - pos) { valid = false; return; }
            data.assign(bytes.begin() + pos, bytes.begin() + pos + size_t(count)); pos += size_t(count);
        };
        if (number(4) != 0x50523353) return false;
        const auto version = number(4);
        if (version != 1 && version != 2) return false;
        ReplayData data;
        data.formatVersion = uint32_t(version);
        data.zoneAct = uint16_t(number(2)); data.actTime = uint32_t(number(4)); data.timestamp = number(8);
        const auto classic = number(1), speedrun = number(1), normal = number(1);
        if (classic > 1 || speedrun > 1 || normal > 1) return false;
        data.classic = classic; data.speedrun = speedrun; data.normalGame = normal;
        data.cutsceneFrames = uint32_t(number(4)); data.frequency = uint32_t(number(4));
        if (data.frequency < 30 || data.frequency > 240 || data.cutsceneFrames > 5*60*60) return false;
        blob(data.compatibility, 65536);
        for (auto& n : data.initialInput) n = uint16_t(number(2));
        const auto settingsCount = number(4);
        if (!valid || settingsCount > 4096) return false;
        for (size_t i = 0; i < settingsCount; ++i) {
            const uint32_t key = uint32_t(number(4)); const uint32_t value = uint32_t(number(4));
            data.settings.emplace_back(key, value);
        }
        const auto persistentCount = number(4);
        if (!valid || persistentCount > 16384) return false;
        for (size_t i = 0; i < persistentCount && valid; ++i) {
            StoredValue value;
            blob(value.file, 4096); blob(value.key, 4096); blob(value.bytes, 0x100000);
            data.persistent.push_back(std::move(value));
        }
        const auto eventCount = number(4);
        if (!valid || eventCount > MAX_FRAMES || pos > end || eventCount * 12 > end - pos) return false;
        for (size_t i = 0; i < eventCount; ++i) {
            SettingEvent event;
            event.frame = uint32_t(number(4)); event.key = uint32_t(number(4)); event.value = uint32_t(number(4));
            if (i && event.frame < data.events.back().frame) return false;
            data.events.push_back(event);
        }
        if (version >= 2) {
            const auto scope = number(1);
            if (scope > 3) return false;
            data.scope = ReplayScope(scope);
            data.initialSpeedMilli = uint16_t(number(2));
            if (data.initialSpeedMilli == 0 || data.initialSpeedMilli > 32000) return false;
            for (auto& time : data.actTimes) time = uint32_t(number(4));
            const auto actionCount = number(4);
            if (!valid || actionCount > 256) return false;
            for (size_t i = 0; i < actionCount && valid; ++i) {
                StateEvent action;
                action.frame = uint32_t(number(4)); action.cutsceneFrames = uint32_t(number(4));
                const auto manualSkip = number(1);
                if (manualSkip > 1 || action.cutsceneFrames > 5*60*60 ||
                    (i && action.frame < data.actions.back().frame)) return false;
                action.manualSkip = manualSkip != 0;
                action.speedMilli = uint16_t(number(2));
                if (action.speedMilli == 0 || action.speedMilli > 32000) return false;
                blob(action.state, 32 * 1024 * 1024);
                if (!valid || action.state.empty()) return false;
                data.actions.push_back(std::move(action));
            }
        } else data.actTimes[data.zoneAct & 1] = data.actTime;
        blob(data.state, 32 * 1024 * 1024);
        const auto frameCount = number(4);
        if (!valid || data.state.empty() || frameCount == 0 || frameCount > MAX_FRAMES || pos > end || frameCount * (version == 1 ? 16 : 19) != end - pos) return false;
        if (!data.events.empty() && data.events.back().frame >= frameCount) return false;
        if (!data.actions.empty() && data.actions.back().frame >= frameCount) return false;
        data.frames.resize(size_t(frameCount));
        data.controls.resize(size_t(frameCount));
        data.previousInputs.resize(size_t(frameCount));
        for (size_t f = 0; f < frameCount; ++f) {
            for (auto& n : data.frames[f]) n = uint16_t(number(2));
            for (auto& n : data.previousInputs[f]) n = uint16_t(number(2));
            if (version >= 2) {
                data.controls[f].speedMilli = uint16_t(number(2));
                const auto manualSkip = number(1);
                if (data.controls[f].speedMilli == 0 || data.controls[f].speedMilli > 32000 || manualSkip > 1) return false;
                data.controls[f].manualSkip = manualSkip != 0;
            }
        }
        if (!valid || pos != end) return false;
        result = std::move(data);
        return true;
    }
};
}
