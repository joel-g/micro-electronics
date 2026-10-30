#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include "hardware.h"

enum class CompassStatus { Waiting, Live, Uncalibrated, Calibrating, Fault, WeakField, CalibrationFailed };
struct DisplayState {
    float heading = 0;
    CompassStatus status = CompassStatus::Waiting;
    int seconds = 0;
    bool saved = false;
};

class CompassDisplay {
public:
    CompassDisplay();
    void begin();
    bool busy() const { return sending; }
    void prepare(DisplayState state);
    void flushBatch();
    uint32_t frames = 0, tiles = 0, maxDrawUs = 0, maxTransferUs = 0;
private:
    // Two indexed 8-bit buffers cost the same RAM as the horizon's one RGB565
    // buffer. Exact byte comparison means no hash collisions or stale pixels.
    class Canvas : public GFXcanvas8 {
    public:
        Canvas() : GFXcanvas8(240, 240, false) { buffer = pixels; }
    private:
        uint8_t pixels[240 * 240] = {};
    } canvas;
    uint8_t previous[240 * 240] = {};
    Adafruit_GC9A01A tft;
    uint16_t tilePixels[16 * 8] = {};
    uint16_t nextTile = 0;
    uint32_t frameTiles = 0;
    bool sending = false, force = true, hasState = false;
    DisplayState lastState{};
    void centered(const char *text, int y, uint8_t size, uint8_t color);
};
