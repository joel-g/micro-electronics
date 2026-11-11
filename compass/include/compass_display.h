#pragma once
#include <IndexedFramebuffer.h>
#include <Adafruit_GC9A01A.h>
#include "hardware.h"

enum class CompassStatus { Waiting, Live, Uncalibrated, Calibrating, Fault, WeakField, CalibrationFailed };
struct DisplayState {
    float heading = 0;
    CompassStatus status = CompassStatus::Waiting;
    int seconds = 0;
    bool saved = false;
};

class CompassDisplay : private IndexedFramebuffer {
public:
    CompassDisplay();
    void begin();
    using IndexedFramebuffer::busy;
    void prepare(DisplayState state);
    void flushBatch();

    using IndexedFramebuffer::frames;
    using IndexedFramebuffer::tiles;
    using IndexedFramebuffer::maxTransferUs;
    uint32_t maxDrawUs = 0;

private:
    Adafruit_GC9A01A tft;
    bool hasState = false;
    DisplayState lastState{};
    void centered(const char *text, int y, uint8_t size, uint8_t color);
};
