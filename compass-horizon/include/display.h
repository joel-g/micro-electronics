#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include "hardware.h"

namespace colors {
enum : uint8_t { Background, White, Muted, Gold, North, Cyan, Red, Green,
                 Sky, Ground, Black, Yellow, HorizonCyan, DarkGold };
}

// One fixed palette for BOTH faces: shadow-buffer bytes always describe the
// physical display, including a partly transmitted frame interrupted by a switch.
class Display {
public:
    Display();
    void begin();
    GFXcanvas8 &canvas() { return buffer; }
    bool busy() const { return sending; }
    void cancel() { sending = false; }
    void present();
    void flushBatch();
    uint32_t frames=0, tiles=0, maxTransferUs=0, maxFrameUs=0;
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
