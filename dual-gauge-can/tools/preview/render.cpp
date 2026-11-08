#include "engine_gauge.h"
#include "compass_gauge.h"
#include <cassert>
#include <fstream>
#include <string>
#include <vector>
void save(GFXcanvas8 &canvas,const std::string &path) {
    std::ofstream out(path,std::ios::binary);out<<"P6\n240 240\n255\n";
    for(int i=0;i<240*240;++i) {
        auto index=canvas.getBuffer()[i];assert(index<sizeof(colors::rgb565)/sizeof(colors::rgb565[0]));
        auto color=colors::rgb565[index];
        uint8_t rgb[]={uint8_t(((color>>11)&31)*255/31),uint8_t(((color>>5)&63)*255/63),uint8_t((color&31)*255/31)};
        out.write(reinterpret_cast<char*>(rgb),3);
    }
}
int main(int argc,char **argv) {
    assert(argc==2);std::string dir=argv[1];
    GFXcanvas8 canvas(240,240);EngineGauge gauge(canvas);engine::Data data;using S=engine::Signal;
    const float temperatures[]={14,90,105,115};const char *names[]={"cold","normal","high","overheat"};
    for(unsigned i=0;i<4;++i) {
        data.get(S::CoolantC).set(temperatures[i],100);
        assert(gauge.draw(Gauge::Coolant,data,100,true));save(canvas,dir+"/"+names[i]+".ppm");
        assert(!gauge.draw(Gauge::Coolant,data,100));
    }
    data.get(S::IatC).set(15,100);gauge.draw(Gauge::Iat,data,100,true);save(canvas,dir+"/iat.ppm");
    data.get(S::BatteryV).set(11.8f,100);gauge.draw(Gauge::Battery,data,100,true);save(canvas,dir+"/battery.ppm");
    data.get(S::MapKpa).set(92,100);data.get(S::BaroKpa).set(100.9f,100);
    gauge.draw(Gauge::Boost,data,100,true);save(canvas,dir+"/boost.ppm");
    gauge.draw(Gauge::Fuel,data,100,true);save(canvas,dir+"/unavailable.ppm");
    CompassGauge compass(canvas);CompassState state;state.status=CompassStatus::Live;state.heading=128;state.saved=true;state.light=true;
    compass.draw(state,true);assert(canvas.getBuffer()[0]==colors::Black);save(canvas,dir+"/compass.ppm");
    // Stale/invalid frames must suppress live colored scales, not retain a hot state.
    assert(gauge.draw(Gauge::Coolant,data,2000,true));
    for(int i=0;i<240*240;++i) assert(canvas.getBuffer()[i]!=colors::EngineRed);
    save(canvas,dir+"/stale.ppm");
    data.get(S::CoolantC).invalidate(2000);assert(gauge.draw(Gauge::Coolant,data,2000));
    save(canvas,dir+"/invalid.ppm");
}
