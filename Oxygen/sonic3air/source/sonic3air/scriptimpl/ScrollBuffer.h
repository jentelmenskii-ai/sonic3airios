#pragma once
#include "oxygen/helper/ClassicScreenLayout.h"

namespace s3air
{
    constexpr uint32 SCROLL_BUFFER_START = 0xffffe000;
    constexpr uint32 DYNAMIC_OBJECTS_START = 0xffffe380;
    static_assert(SCROLL_BUFFER_START + ClassicScreenLayout::GAME_HEIGHT * 4 == DYNAMIC_OBJECTS_START);
    template<typename Memory>
    void writeLegacyScrollOffsets(Memory& memory, uint32 value, int requestedLines)
    {
        const int lines = std::max(0, std::min(requestedLines, ClassicScreenLayout::GAME_HEIGHT));
        for (int line = 0; line < lines; ++line)
            memory.writeMemory32(SCROLL_BUFFER_START + line * 4, value);
    }
}
