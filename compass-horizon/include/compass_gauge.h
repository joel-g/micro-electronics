#pragma once
#include <Adafruit_GFX.h>
enum class CompassStatus { Waiting, Live, Uncalibrated, Calibrating, Fault, WeakField, CalibrationFailed };
struct CompassState {
    float heading=0;
    CompassStatus status=CompassStatus::Waiting;
    int seconds=0;
    bool saved=false;
    bool light=false;
};
class CompassGauge {
public:
    explicit CompassGauge(GFXcanvas8 &surface) : canvas(surface) {}
    bool draw(CompassState state, bool force=false);
private:
    GFXcanvas8 &canvas;
    bool hasState=false;
    CompassState lastState{};
    void centered(const char *text, int y, uint8_t size, uint8_t color);
};
