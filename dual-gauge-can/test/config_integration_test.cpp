#include "engine_reading.h"
#include "ecu_decoder.h"
#include "display_settings.h"
#include <cassert>
#include <cmath>
#include <cstring>
int main() {
    using S=engine::Signal;
    assert(instrumentConfig::canId==0x18ff1234 && instrumentConfig::canExtended);
    assert(instrumentConfig::canBitrate==250000 && !instrumentConfig::canAck);
    assert(!gaugeConfig::temperatureF && !gaugeConfig::pressurePsi && gaugeConfig::compassLight);
    assert(!gaugeConfig::horizonNumerical && gaugeConfig::showAfr);
    // Every optional source is enabled only by inclusion in one of the manifests.
    assert(instrumentConfig::enableOil && instrumentConfig::enableFuel && instrumentConfig::enableLambda);
    CanFrame f;f.id=instrumentConfig::canId;f.extended=true;f.dlc=8;
    LinkGenericDashDecoder decoder;engine::Data d;
    f.data[0]=8;f.data[4]=0x2c;f.data[5]=1;decoder.onFrame(f,100,d);
    assert(d.get(S::OilKpa).value==300);
    assert(!strcmp(engineReading(Gauge::Oil,d,100).units,"kPa"));
    f.data[0]=7;f.data[6]=0x90;f.data[7]=1;decoder.onFrame(f,100,d);assert(d.get(S::FuelKpa).value==400);
    f.data[0]=6;f.data[4]=0xdc;f.data[5]=5;decoder.onFrame(f,100,d);assert(std::fabs(d.get(S::Lambda).value-1.5f)<0.0001);
    assert(!strcmp(engineReading(Gauge::Lambda,d,100).text,"14.70"));
    f.data[0]=2;f.data[6]=40;f.data[7]=0;decoder.onFrame(f,100,d);assert(!strcmp(engineReading(Gauge::Coolant,d,100).text,"-10.0"));
    auto matched=decoder.matched;f.extended=false;decoder.onFrame(f,200,d);assert(decoder.matched==matched);
    // Removed saved IDs fall back to the configured first face, not hard-coded local gauges.
    fake::nvs["dual-can/display1"]={uint8_t(Gauge::Rpm)};
    fake::nvs["dual-can/display2"]={uint8_t(Gauge::Battery)};
    DisplaySettings settings;settings.begin();
    assert(settings.first==Gauge::Coolant && settings.second==Gauge::Iat);
    settings.cycleFirst();fake::timeUs=400000;settings.service();
    DisplaySettings reboot;reboot.begin();assert(reboot.first==Gauge::Oil);
    settings.cycleSecond();assert(settings.second==Gauge::Horizon);
    // IDs retained across list reordering: persisted Oil is still Oil.
    assert(uint8_t(Gauge::Oil)==3 && uint8_t(Gauge::Iat)==10);
}
