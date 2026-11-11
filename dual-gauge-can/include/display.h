#pragma once
#include <IndexedFramebuffer.h>
#include <Adafruit_GC9A01A.h>
#include "hardware.h"

#include "gauge_palette.h"

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
