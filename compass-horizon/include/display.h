#pragma once
#include <IndexedFramebuffer.h>
#include <Adafruit_GC9A01A.h>
#include "hardware.h"

namespace colors {
enum : uint8_t { Background, White, Muted, Gold, North, Cyan, Red, Green,
                 Sky, Ground, Black, Yellow, HorizonCyan, DarkGold };
}

// One fixed palette for BOTH faces: shadow-buffer bytes always describe the
// physical display, including a partly transmitted frame interrupted by a switch.
class Display : private IndexedFramebuffer {
public:
    Display();
    void begin();
    using IndexedFramebuffer::canvas;
    using IndexedFramebuffer::busy;
    using IndexedFramebuffer::cancel;
    using IndexedFramebuffer::present;
    void flushBatch();

    using IndexedFramebuffer::frames;
    using IndexedFramebuffer::tiles;
    using IndexedFramebuffer::maxTransferUs;
    using IndexedFramebuffer::maxFrameUs;

private:
    Adafruit_GC9A01A tft;
};
