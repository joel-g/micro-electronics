#include "ecu_decoder.h"
#include "engine_reading.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>
static CanFrame packet(uint8_t mux,uint16_t a,uint16_t b,uint16_t c) {
    CanFrame f;f.id=1000;f.dlc=8;f.data[0]=mux;f.data[1]=0;
    const uint16_t words[]={a,b,c};
    for(unsigned i=0;i<3;++i) {f.data[2+i*2]=words[i]&255;f.data[3+i*2]=words[i]>>8;}
    return f;
}
int main() {
    using S=engine::Signal;using State=engine::State;
    LinkGenericDashDecoder decoder;engine::Data d;
    // Independent documented examples: 0x1484 RPM=5252; map=200 kPa;
    // baro 1013=>101.3 kPa; ECT 40=>-10 C; volts 1385=>13.85 V.
    auto f=packet(0,5252,200,200);decoder.onFrame(f,100,d);
    assert(d.get(S::Rpm).value==5252 && d.get(S::MapKpa).value==200);
    assert(engineReading(Gauge::Boost,d,100).state==State::Never);
    decoder.onFrame(packet(1,1013,450,100),101,d);
    assert(std::fabs(d.get(S::BaroKpa).value-101.3f)<0.001f);
    assert(!strcmp(engineReading(Gauge::Boost,d,101).text,"14.3"));
    decoder.onFrame(packet(2,0,2000,40),102,d);
    assert(d.get(S::CoolantC).value==-10);
    decoder.onFrame(packet(3,65,1385,0),103,d);
    assert(std::fabs(d.get(S::BatteryV).value-13.85f)<0.001f);
    assert(d.get(S::IatC).value==15);
    assert(!strcmp(engineReading(Gauge::Iat,d,103).text,"59.0"));
    assert(!strcmp(engineReading(Gauge::Iat,d,103).label,"INTAKE AIR TEMP"));
    assert(!strcmp(engineReading(Gauge::Iat,d,103).units,"F"));
    assert(d.get(S::IatC).state(1604)==State::Stale);
    decoder.onFrame(packet(3,40,1385,0),104,d);
    assert(d.get(S::IatC).value==-10 && !strcmp(engineReading(Gauge::Iat,d,104).text,"14.0"));
    decoder.onFrame(packet(3,29,1385,0),105,d);
    assert(d.get(S::IatC).state(105)==State::Invalid && d.get(S::BatteryV).state(105)==State::Valid);
    decoder.onFrame(packet(3,65,1385,0),106,d);
    assert(d.get(S::IatC).state(106)==State::Valid);
    // Physical zero is valid RPM; each multiplexed channel expires independently.
    decoder.onFrame(packet(0,0,50,50),1602,d);
    assert(d.get(S::Rpm).value==0 && d.get(S::Rpm).state(1602)==State::Valid);
    assert(d.get(S::BaroKpa).state(1602)==State::Stale);
    assert(engineReading(Gauge::Boost,d,1602).state==State::Stale);
    decoder.onFrame(packet(1,1000,0,0),1602,d);
    assert(!strcmp(engineReading(Gauge::Boost,d,1602).text,"-7.3"));
    // Wrong frame types/IDs, truncated/oversized payloads, reserved byte, unknown mux.
    auto updated=d.get(S::Rpm).updated;
    f.id=0;decoder.onFrame(f,2000,d);f.id=1000;
    f.extended=true;decoder.onFrame(f,2000,d);f.extended=false;
    f.rtr=true;decoder.onFrame(f,2000,d);f.rtr=false;
    f.dlc=7;decoder.onFrame(f,2000,d);f.dlc=9;decoder.onFrame(f,2000,d);f.dlc=8;
    f.data[1]=1;decoder.onFrame(f,2000,d);f.data[1]=0;
    f.data[0]=255;decoder.onFrame(f,2000,d);
    assert(d.get(S::Rpm).updated==updated && decoder.malformed==3 && decoder.unknownMux==1);
    // Missing sources cannot become plausible zeros or values from placeholder bytes.
    for(unsigned mux=4;mux<14;++mux) decoder.onFrame(packet(mux,100,100,100),2100,d);
    for(auto s:{S::OilKpa,S::FuelKpa,S::Lambda,S::Fan1,S::Fan2}) assert(!d.get(s).received);
    decoder.onFrame(packet(0,65535,65535,0),2200,d);
    decoder.onFrame(packet(1,0,0,0),2200,d);
    decoder.onFrame(packet(2,0,0,256),2200,d);
    decoder.onFrame(packet(3,0,65535,0),2200,d);
    for(auto s:{S::Rpm,S::MapKpa,S::BaroKpa,S::CoolantC,S::BatteryV,S::IatC}) assert(d.get(s).state(2200)==State::Invalid);
    assert(decoder.invalidValues==7);
    decoder.onFrame(packet(0,0,100,100),0xfffffff0u,d);
    assert(d.get(S::Rpm).state(20)==State::Valid && d.get(S::Rpm).state(1600)==State::Stale);
    puts("PASS: Generic Dash layout/scales, little endian, negative coolant, real zero, independent expiry/rollover, invalid ranges, strict framing and absent sensors");
}
