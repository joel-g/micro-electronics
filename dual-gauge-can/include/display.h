#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include "hardware.h"

#include "gauge_palette.h"

// Each display owns its canvas and shadow so an in-flight frame stays immutable.
// No full RGB565 framebuffer, PSRAM, DMA, or runtime framebuffer allocation.
class Display {
public:
    explicit Display(int cs);
    static void beginBus();
    void begin();
    GFXcanvas8 &canvas() { return buffer; }
    bool busy() const { return sending; }
    void cancel() { sending=false; }
    void present();
    void flushBatch(unsigned budget=4);
    uint32_t frames=0, completed=0, tiles=0, maxTransferUs=0, maxFrameUs=0;
private:
    class Canvas : public GFXcanvas8 {
    public:
        Canvas() : GFXcanvas8(240,240,false) { buffer=pixels; }
    private:
        alignas(4) uint8_t pixels[240*240] = {};
    } buffer;
    uint8_t previous[240*240] = {};
    Adafruit_GC9A01A tft;
    uint16_t tilePixels[16*8] = {};
    uint16_t nextTile=0;
    uint32_t frameTiles=0, started=0;
    bool sending=false, force=true;
};
