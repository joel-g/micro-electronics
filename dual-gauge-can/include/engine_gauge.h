#pragma once
#include <Adafruit_GFX.h>
#include "gauge_style.h"
class EngineGauge {
public:
    explicit EngineGauge(GFXcanvas8 &c):canvas(c) {}
    bool draw(Gauge,const engine::Data &,uint32_t now,bool force=false);
private:
    GFXcanvas8 &canvas;
    bool initialized=false;
    Gauge previous=Gauge::Horizon;
    EngineReading last{};
    uint8_t lastAccent=colors::Muted;
    int lastSegment=-1;
    void scale(Gauge,const gaugeStyle::Style &,float position,bool valid);
    void icon(Gauge,uint8_t color);
    void centered(const char *,int y,uint8_t size,uint8_t color);
};
