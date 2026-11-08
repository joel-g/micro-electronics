#include "gauge_style.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main() {
    using namespace colors;
    using S=engine::Signal;
    engine::Data data;
    const float temps[]={59,60,99,100,109,110};
    const uint8_t expected[]={EngineBlue,EngineGreen,EngineGreen,EngineOrange,EngineOrange,EngineRed};
    const char *labels[]={"COLD","NORMAL","NORMAL","HIGH","HIGH","OVERHEAT"};
    for(unsigned i=0;i<6;++i) {
        data.get(S::CoolantC).set(temps[i],100);
        auto reading=engineReading(Gauge::Coolant,data,100);
        auto style=gaugeStyle::forReading(Gauge::Coolant,reading);
        assert(style.accent==expected[i]);assert(!strcmp(style.status,labels[i]));
        assert(gaugeStyle::position(style,-999)==0 && gaugeStyle::position(style,999)==1);
    }
    auto stale=engineReading(Gauge::Coolant,data,1601);assert(stale.state==engine::State::Stale);
    data.get(S::CoolantC).invalidate(1602);assert(engineReading(Gauge::Coolant,data,1602).state==engine::State::Invalid);
    data.get(S::MapKpa).set(92,2000);data.get(S::BaroKpa).set(100.9f,2000);
    auto style=gaugeStyle::forReading(Gauge::Boost,engineReading(Gauge::Boost,data,2000));
    assert(style.accent==EngineBlue && !strcmp(style.status,"VACUUM"));
    data.get(S::MapKpa).set(200,2000);style=gaugeStyle::forReading(Gauge::Boost,engineReading(Gauge::Boost,data,2000));
    assert(style.accent==Cyan && !strcmp(style.status,"BOOST"));
    puts("PASS: coolant threshold boundaries/labels, unit conversion, bounded scale, stale/invalid and vacuum/boost styling");
}
