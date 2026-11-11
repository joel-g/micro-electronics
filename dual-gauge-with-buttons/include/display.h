#pragma once
#include <IndexedFramebuffer.h>
#include <Adafruit_GC9A01A.h>
#include "hardware.h"

namespace colors {
enum : uint8_t { Background, White, Muted, Gold, North, Cyan, Red, Green,
                 Sky, Ground, Black, Yellow, HorizonCyan, DarkGold };
}

// Each display owns its canvas and shadow so an in-flight frame stays immutable.
// No full RGB565 framebuffer, PSRAM, DMA, or runtime framebuffer allocation.
class Display : private IndexedFramebuffer {
public:
    explicit Display(int cs);
    static void beginBus();
    void begin();
    using IndexedFramebuffer::canvas;
    using IndexedFramebuffer::busy;
    using IndexedFramebuffer::cancel;
    using IndexedFramebuffer::present;
    void flushBatch(unsigned budget = 4);

    using IndexedFramebuffer::frames;
    using IndexedFramebuffer::completed;
    using IndexedFramebuffer::tiles;
    using IndexedFramebuffer::maxTransferUs;
    using IndexedFramebuffer::maxFrameUs;

private:
    Adafruit_GC9A01A tft;
};
