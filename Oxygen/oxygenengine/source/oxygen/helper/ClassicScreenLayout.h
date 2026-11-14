#pragma once
#include <rmxbase.h>

namespace ClassicScreenLayout
{
    constexpr int GAME_WIDTH = 320;
    constexpr int GAME_HEIGHT = 224; // The following emulated RAM belongs to dynamic objects.
    inline Vec2i constrain(bool classic, Vec2i requested)
    {
        if (!classic) return requested;
        return Vec2i(GAME_WIDTH, GAME_HEIGHT);
    }
    inline int menuLeftShift(bool classic) { return classic ? 80 : 0; }
    inline bool isMenu(bool classic, Vec2i source, bool menuActive = false)
    {
        return classic && (menuActive || source == Vec2i(400, GAME_HEIGHT));
    }
    inline Vec2i compositionSize(bool classic, Vec2i source, bool menuActive = false)
    {
        // The original 224-line layout is shared by gameplay and native Classic menus.
        return constrain(classic, source);
    }
}
