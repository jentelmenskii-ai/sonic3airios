#pragma once
#include <cstdint>

namespace s3air
{
// A native UI pause has no simulation duration; retain the last running speed.
inline uint16_t replaySpeedMilli(float speed, uint16_t previous = 1000)
{
    if (!(speed > 0.0f)) return previous;
    if (speed >= 32.0f) return 32000;
    const auto value = static_cast<uint16_t>(speed * 1000.0f + 0.5f);
    return value ? value : 1;
}

// Persisted option values; keep stable across versions.
enum class ReplayScope : uint8_t { EACH_ACT = 0, FULL_ZONE = 1, ACT_1 = 2, ACT_2 = 3 };

inline bool replayShouldStart(ReplayScope scope, uint16_t zoneAct)
{
    const unsigned act = zoneAct & 0xff;
    if (act > 1) return false;
    switch (scope)
    {
        case ReplayScope::FULL_ZONE:
        case ReplayScope::ACT_1: return act == 0;
        case ReplayScope::ACT_2: return act == 1;
        case ReplayScope::EACH_ACT: return true;
    }
    return false;
}

inline bool replayShouldFinish(ReplayScope scope, uint16_t startAct, uint16_t completedAct, int zoneActs, bool leavingZone)
{
    if ((startAct >> 8) != (completedAct >> 8)) return false;
    if (scope == ReplayScope::FULL_ZONE) return leavingZone || zoneActs <= 1 || (completedAct & 0xff) == 1;
    return startAct == completedAct;
}
}
