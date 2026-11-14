#pragma once

#include <rmxbase.h>

namespace rmx
{
    // Map window coordinates to backing pixels without changing logical UI/input units.
    inline Recti mapWindowRectToPixels(const Recti& rect, Vec2i windowSize, Vec2i drawableSize)
    {
        if (windowSize.x <= 0 || windowSize.y <= 0 || drawableSize.x <= 0 || drawableSize.y <= 0)
            return rect;
        const double scaleX = (double)drawableSize.x / windowSize.x;
        const double scaleY = (double)drawableSize.y / windowSize.y;
        // Round shared edges, rather than widths, to keep adjacent rectangles aligned.
        const int left = roundToInt(rect.x * scaleX);
        const int bottom = roundToInt(rect.y * scaleY);
        const int right = roundToInt((rect.x + rect.width) * scaleX);
        const int top = roundToInt((rect.y + rect.height) * scaleY);
        return Recti(left, bottom, right - left, top - bottom);
    }
}
