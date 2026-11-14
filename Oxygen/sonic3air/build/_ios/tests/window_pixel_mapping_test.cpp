#include "rmxmedia/framework/WindowPixelMapping.h"
#include <cstdlib>
#include <iostream>

static void expect(Recti actual, Recti expected)
{
    if (actual.x != expected.x || actual.y != expected.y ||
        actual.width != expected.width || actual.height != expected.height)
    {
        std::cerr << "Incorrect drawable rectangle\n";
        std::exit(1);
    }
}

int main()
{
    using rmx::mapWindowRectToPixels;
    // iPhone 11: window points must fill the 2x backing framebuffer.
    expect(mapWindowRectToPixels(Recti(0, 0, 896, 414), Vec2i(896, 414), Vec2i(1792, 828)), Recti(0, 0, 1792, 828));
    // A clipped UI region and the upscaler's letterboxed viewport both need scaling.
    expect(mapWindowRectToPixels(Recti(10, 20, 100, 50), Vec2i(896, 414), Vec2i(1792, 828)), Recti(20, 40, 200, 100));
    expect(mapWindowRectToPixels(Recti(48, 0, 800, 414), Vec2i(896, 414), Vec2i(1792, 828)), Recti(96, 0, 1600, 828));
    // 3x devices, portrait/landscape changes and external displays are not hardcoded.
    expect(mapWindowRectToPixels(Recti(0, 0, 844, 390), Vec2i(844, 390), Vec2i(2532, 1170)), Recti(0, 0, 2532, 1170));
    expect(mapWindowRectToPixels(Recti(0, 0, 390, 844), Vec2i(390, 844), Vec2i(1170, 2532)), Recti(0, 0, 1170, 2532));
    // Fractional scales round shared boundaries, so adjacent scissor areas don't gap.
    const Recti a = mapWindowRectToPixels(Recti(0, 0, 1, 7), Vec2i(3, 7), Vec2i(8, 17));
    const Recti b = mapWindowRectToPixels(Recti(1, 0, 2, 7), Vec2i(3, 7), Vec2i(8, 17));
    if (a.x + a.width != b.x || b.x + b.width != 8 || a.height != 17) return 1;
    // A minimized/uninitialized backing size must not divide by zero.
    expect(mapWindowRectToPixels(Recti(1, 2, 3, 4), Vec2i(0, 0), Vec2i(0, 0)), Recti(1, 2, 3, 4));
    expect(mapWindowRectToPixels(Recti(1, 2, 3, 4), Vec2i(10, 10), Vec2i(10, 10)), Recti(1, 2, 3, 4));
    std::cout << "Retina viewport/scissor mapping passed: 1x, 2x, 3x, rotation, fractional scale, zero-size fallback\n";
}
