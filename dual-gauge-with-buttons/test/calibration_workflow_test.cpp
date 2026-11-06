#include "magnetometer.h"
#include <Preferences.h>
#include <cassert>
#include <cstdio>

namespace {
bool moving=true, injectBusError=false;
unsigned samples=0, begins=0;
void advance(Magnetometer &mag,unsigned ms=20) { fake::timeUs+=ms*1000; mag.service(); }
void complete(Magnetometer &mag) {
    for(unsigned i=0;mag.isCollecting();++i) { assert(i<3100); advance(mag); }
}
bool same(compass::Vector a,compass::Vector b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
}
bool Qmc5883p::begin() { ++begins; return true; }
Qmc5883p::Result Qmc5883p::read(MagneticSample &sample) {
    if(injectBusError) return Result::BusError;
    ++samples;
    sample={int16_t(100+(moving && (samples&1) ? 800 : -800)),
            int16_t(-50+(moving && (samples&2) ? 800 : -800)),
            int16_t(20+(moving && (samples&4) ? 800 : -800))};
    return Result::Sample;
}
int main() {
    CalibrationStore store; assert(store.begin());
    const compass::Vector old{11,22,33}; assert(store.save(old));
    Magnetometer mag; mag.begin(); advance(mag); advance(mag);
    assert(mag.fresh() && mag.isSaved() && same(mag.offsets(),old));
    assert(mag.startCalibration(true));
    auto started=millis();
    for(unsigned i=0;i<1500;++i) advance(mag);
    assert(mag.isCollecting() && !mag.startCalibration(true));
    mag.command('f'); assert(mag.isCollecting());
    complete(mag);
    assert(millis()-started>=60000 && millis()-started<60100);
    const compass::Vector expected{100,-50,20};
    assert(mag.isSaved() && same(mag.offsets(),expected));
    assert(mag.displayState().status==CompassStatus::CalibrationSaved);
    assert(begins==1); // No sensor restart for calibration.
    Magnetometer reboot; reboot.begin(); assert(reboot.isSaved() && same(reboot.offsets(),expected));
    advance(mag,3001); assert(mag.displayState().status==CompassStatus::Live);

    auto savedRecord=fake::nvs.at("qmc5883p/hardiron");
    auto writes=fake::writeAttempts;
    moving=false; assert(mag.startCalibration(true)); complete(mag);
    assert(mag.displayState().status==CompassStatus::CalibrationFailed);
    assert(same(mag.offsets(),expected) && mag.isSaved());
    assert(fake::writeAttempts==writes && fake::nvs.at("qmc5883p/hardiron")==savedRecord);

    moving=true; assert(mag.startCalibration(true));
    injectBusError=true; advance(mag); injectBusError=false; complete(mag);
    assert(mag.displayState().status==CompassStatus::CalibrationFailed);
    assert(fake::writeAttempts==writes && same(mag.offsets(),expected));

    assert(mag.startCalibration(true)); fake::failWrites=true; complete(mag);
    assert(mag.displayState().status==CompassStatus::SaveFailed);
    assert(same(mag.offsets(),expected) && fake::nvs.at("qmc5883p/hardiron")==savedRecord);
    fake::failWrites=false;

    // Existing serial calibration remains explicitly RAM-first, with optional s.
    mag.command('c'); for(unsigned i=0;i<510;++i) advance(mag);
    writes=fake::writeAttempts; mag.command('f');
    assert(!mag.isSaved() && !mag.isCollecting() && fake::writeAttempts==writes);
    mag.command('s'); assert(mag.isSaved());
    puts("PASS: full 60s auto-save/reload, progress/result expiry, duplicate hold/early-finish guard, failed/invalid calibration preservation, save failure, serial RAM workflow");
}
