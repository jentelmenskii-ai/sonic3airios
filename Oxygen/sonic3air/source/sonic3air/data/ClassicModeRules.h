#pragma once
#include "sonic3air/data/SharedDatabase.h"
#include "oxygen/helper/ClassicScreenLayout.h"

// Effective settings only: the user's Anniversary preferences remain untouched.
namespace ClassicModeRules
{
    inline bool forcedSetting(uint32 key, uint32& value)
    {
        using S = SharedDatabase::Setting;
        switch (key)
        {
            case S::SETTING_SMOOTH_ROTATION:
            case S::SETTING_EXTENDED_HUD:
            case S::SETTING_SPEEDUP_AFTERIMGS:
            case S::SETTING_SUPERFAST_RUNANIM:
            case S::SETTING_GFX_ANTIFLICKER: value = 0; return true;
            case S::SETTING_LIVES_DISPLAY: value = 2; return true;
            default: return false;
        }
    }

    inline std::wstring saveRoot(const std::wstring& documents, bool classic, bool shared = false)
    {
        return documents + ((classic && !shared) ? L"classic/" : L"");
    }
    inline Vec2i gameSize(bool classic) { return classic ? Vec2i(ClassicScreenLayout::GAME_WIDTH, ClassicScreenLayout::GAME_HEIGHT) : Vec2i(400, 224); }
}
