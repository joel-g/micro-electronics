#pragma once
#include <Adafruit_GFX.h>
#ifndef GAUGE_DEBUG_OVERLAY
#define GAUGE_DEBUG_OVERLAY 0
#endif
class AttitudeGauge {
public:
    explicit AttitudeGauge(GFXcanvas8 &surface) : canvas(surface) {}
    void draw(float pitch, float roll, bool fresh, uint32_t frame);
private:
    GFXcanvas8 &canvas;
    void drawRotatedLine(float x, float y, float halfLength, float angle, uint16_t color);
    void drawHorizon(float pitch, float roll);
    void drawAircraftSymbol();
    void drawBankScale(float roll);
#if GAUGE_DEBUG_OVERLAY
    void drawReadouts(float pitch, float roll);
#endif
};
