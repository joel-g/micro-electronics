#include "engine_reading.h"
#include "ecu_decoder.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main() {
    using S=engine::Signal;using State=engine::State;
    engine::Data data;
    for(auto g:gaugeConfig::first) if(g!=Gauge::Horizon) assert(engineReading(g,data,0).state==State::Never);
    for(auto g:gaugeConfig::second) if(g!=Gauge::Compass) assert(engineReading(g,data,0).state==State::Never);
    LinkGenericDashDecoder decoder;CanFrame frame;frame.id=0x3E8;frame.dlc=8;memset(frame.data,0xff,8);
    decoder.onFrame(frame,100,data);for(auto &c:data.channels) assert(!c.received);
    data.get(S::Rpm).set(0,100);auto r=engineReading(Gauge::Rpm,data,100);
    assert(r.state==State::Valid && !strcmp(r.text,"0"));
    assert(engineReading(Gauge::Rpm,data,1600).state==State::Valid);
    assert(engineReading(Gauge::Rpm,data,1601).state==State::Stale);
    data.get(S::CoolantC).set(100,200);assert(!strcmp(engineReading(Gauge::Coolant,data,200).text,"212.0"));
    data.get(S::OilKpa).set(689.4757f,200);assert(!strcmp(engineReading(Gauge::Oil,data,200).text,"100.0"));
    data.get(S::MapKpa).set(200,200);assert(engineReading(Gauge::Boost,data,200).state==State::Never);
    data.get(S::BaroKpa).set(100,200);assert(!strcmp(engineReading(Gauge::Boost,data,200).text,"14.5"));
    data.get(S::MapKpa).set(50,200);assert(!strcmp(engineReading(Gauge::Boost,data,200).text,"-7.3"));
    data.get(S::BaroKpa).set(100,0);data.get(S::MapKpa).set(200,1600);
    assert(engineReading(Gauge::Boost,data,1600).state==State::Stale);
    data.get(S::Lambda).set(NAN,200);assert(engineReading(Gauge::Lambda,data,200).state==State::Invalid);
    data.get(S::Fan1).set(0,200);assert(engineReading(Gauge::Fans,data,200).state==State::Never);
    data.get(S::Fan2).set(1,200);assert(!strcmp(engineReading(Gauge::Fans,data,200).text,"1 OFF  2 ON"));
    data.get(S::Fan2).set(2,200);assert(engineReading(Gauge::Fans,data,200).state==State::Invalid);
    data.get(S::BatteryV).set(12,0xfffffff0u);assert(engineReading(Gauge::Battery,data,20).state==State::Valid);
    assert(engineReading(Gauge::Battery,data,1600).state==State::Stale);
    puts("PASS: unknown multiplexor remains unavailable; real zero, stale/invalid/rollover, unit conversions, boost requires fresh barometer, fan validity");
}
