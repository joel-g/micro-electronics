#pragma once
#include "engine_data.h"
#include "gauge_config.h"
#include <cstdio>
struct EngineReading {
    const char *label="UNKNOWN", *units="";
    engine::State state=engine::State::Never;
    char text[24]{};
    float value=0; // Converted display units, only meaningful when state is Valid.
};
inline EngineReading engineReading(Gauge gauge,const engine::Data &data,uint32_t now) {
    using S=engine::Signal; using State=engine::State;
    EngineReading out;
    S signal=S::Rpm;
    switch(gauge) {
        case Gauge::Coolant:out.label="COOLANT TEMP";out.units=gaugeConfig::temperatureF?"F":"C";signal=S::CoolantC;break;
        case Gauge::Iat:out.label="INTAKE AIR TEMP";out.units=gaugeConfig::temperatureF?"F":"C";signal=S::IatC;break;
        case Gauge::Oil:out.label="OIL PRESSURE";out.units=gaugeConfig::pressurePsi?"PSI":"kPa";signal=S::OilKpa;break;
        case Gauge::Rpm:out.label="ENGINE SPEED";out.units="RPM";break;
        case Gauge::Boost:out.label="BOOST";out.units=gaugeConfig::pressurePsi?"PSI":"kPa";signal=S::MapKpa;break;
        case Gauge::Fuel:out.label="FUEL PRESSURE";out.units=gaugeConfig::pressurePsi?"PSI":"kPa";signal=S::FuelKpa;break;
        case Gauge::Fans:out.label="FAN STATUS";signal=S::Fan1;break;
        case Gauge::Lambda:out.label=gaugeConfig::showAfr?"AIR FUEL RATIO":"LAMBDA";out.units=gaugeConfig::showAfr?"AFR":"LAMBDA";signal=S::Lambda;break;
        case Gauge::Battery:out.label="BATTERY";out.units="V";signal=S::BatteryV;break;
        default:return out;
    }
    const auto &channel=data.get(signal);
    out.state=channel.state(now,gaugeConfig::staleMs);
    if(out.state!=State::Valid) return out;
    float value=channel.value;
    if(gauge==Gauge::Boost || gauge==Gauge::Fans) {
        const auto &other=data.get(gauge==Gauge::Boost?S::BaroKpa:S::Fan2);
        out.state=other.state(now,gaugeConfig::staleMs);
        if(out.state!=State::Valid) return out;
        if(gauge==Gauge::Fans) {
            if((value!=0 && value!=1) || (other.value!=0 && other.value!=1)) {out.state=State::Invalid;return out;}
            snprintf(out.text,sizeof(out.text),"1 %s  2 %s",value==1?"ON":"OFF",other.value==1?"ON":"OFF");
            return out;
        }
        if(value<0 || other.value<=0) {out.state=State::Invalid;return out;}
        value-=other.value; // Never substitute a fixed atmospheric pressure.
    }
    if(gauge==Gauge::Coolant || gauge==Gauge::Iat) value=gaugeConfig::temperature(value);
    if(gauge==Gauge::Oil || gauge==Gauge::Fuel || gauge==Gauge::Boost) value=gaugeConfig::pressure(value);
    if(gauge==Gauge::Lambda && gaugeConfig::showAfr) value*=gaugeConfig::stoichiometricAfr;
    out.value=value;
    snprintf(out.text,sizeof(out.text),gauge==Gauge::Rpm?"%.0f":gauge==Gauge::Lambda?"%.2f":"%.1f",value);
    return out;
}
