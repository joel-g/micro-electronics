#pragma once
#include "engine_reading.h"
#include "gauge_palette.h"
namespace gaugeStyle {
struct Style {
    float minimum=0, maximum=100;
    uint8_t accent=colors::Cyan;
    const char *status="LIVE";
};
constexpr float temperature(float c) {return gaugeConfig::temperature(c);}
inline uint8_t coolantColor(float f) {
    return f<temperature(gaugeConfig::coolantWarmC)?colors::EngineBlue:
           f<temperature(gaugeConfig::coolantHighC)?colors::EngineGreen:
           f<temperature(gaugeConfig::coolantOverheatC)?colors::EngineOrange:colors::EngineRed;
}
inline Style forReading(Gauge gauge,const EngineReading &reading) {
    Style s;
    auto range=gaugeConfig::scale(gauge);s.minimum=range.minimum;s.maximum=range.maximum;
    switch(gauge) {
        case Gauge::Coolant:
            s.accent=coolantColor(reading.value);
            s.status=s.accent==colors::EngineBlue?"COLD":s.accent==colors::EngineGreen?"NORMAL":
                s.accent==colors::EngineOrange?"HIGH":"OVERHEAT";break;
        case Gauge::Iat:s.accent=colors::Cyan;s.status="INTAKE";break;
        case Gauge::Rpm:s.accent=colors::Gold;s.status=reading.value==0?"ENGINE OFF":"ENGINE SPEED";break;
        case Gauge::Boost:
            s.accent=reading.value<0?colors::EngineBlue:colors::Cyan;
            s.status=reading.value<0?"VACUUM":"BOOST";break;
        case Gauge::Battery:s.accent=colors::Cyan;s.status="ECU SUPPLY";break;
        case Gauge::Oil:s.accent=colors::Gold;s.status="OIL SYSTEM";break;
        case Gauge::Fuel:s.accent=colors::Gold;s.status="FUEL SYSTEM";break;
        case Gauge::Lambda:
            s.accent=colors::Cyan;s.status="MIXTURE";break;
        case Gauge::Fans:s.accent=colors::Cyan;s.status="FAN CONTROL";break;
        default:break;
    }
    return s;
}
inline float position(const Style &style,float value) {
    float p=(value-style.minimum)/(style.maximum-style.minimum);
    return p<0?0:p>1?1:p;
}
}
