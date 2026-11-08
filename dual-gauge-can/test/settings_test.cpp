#include "display_settings.h"
#include <cassert>
#include <cstdio>
int main() {
    fake::nvs["qmc5883p/hardiron"]={1,2,3,4};
    fake::nvs["dual-buttons/compass"]={1};fake::nvs["dual-buttons/attitude"]={1};
    DisplaySettings s;s.begin();
    assert(s.first==Gauge::Horizon && s.second==Gauge::Compass && s.light==gaugeConfig::compassLight && s.numerical==gaugeConfig::horizonNumerical);
    s.cycleFirst();assert(s.first==Gauge::Coolant && s.second==Gauge::Compass);
    fake::timeUs=300000;s.service();s.cycleSecond();fake::timeUs=600000;s.service();
    DisplaySettings reboot;reboot.begin();assert(reboot.first==Gauge::Coolant && reboot.second==Gauge::Battery);
    auto writes=fake::writeAttempts;
    for(int i=0;i<int(sizeof(gaugeConfig::first)/sizeof(Gauge));++i) reboot.cycleFirst();
    fake::timeUs+=500000;reboot.service();assert(fake::writeAttempts==writes);
    reboot.cycleFirst();fake::failWrites=true;fake::timeUs+=300000;reboot.service();
    assert(reboot.pending());writes=fake::writeAttempts;
    fake::timeUs+=4999000;reboot.service();assert(fake::writeAttempts==writes);
    fake::failWrites=false;fake::timeUs+=1000;reboot.service();assert(!reboot.pending());
    // New stable ID appends after Battery; old saved IDs retain their meanings.
    fake::nvs["dual-can/display2"]={uint8_t(Gauge::Battery)};
    DisplaySettings withIat;withIat.begin();assert(withIat.second==Gauge::Battery);
    withIat.cycleSecond();assert(withIat.second==Gauge::Iat);
    fake::timeUs+=300000;withIat.service();
    DisplaySettings savedIat;savedIat.begin();assert(savedIat.second==Gauge::Iat);
    savedIat.cycleSecond();assert(savedIat.second==Gauge::Compass);
    // Previously selectable, now removed faces fall back to this manifest's first entries.
    fake::nvs["dual-can/display1"]={uint8_t(Gauge::Oil)};
    fake::nvs["dual-can/display2"]={uint8_t(Gauge::Fuel)};
    DisplaySettings removed;removed.begin();
    assert(removed.first==gaugeConfig::first[0] && removed.second==gaugeConfig::second[0]);
    fake::nvs["dual-can/display1"]={255};fake::nvs["dual-can/display2"]={0};
    DisplaySettings corrupt;corrupt.begin();assert(corrupt.first==Gauge::Horizon && corrupt.second==Gauge::Compass);
    assert(fake::nvs["dual-buttons/compass"]==std::vector<uint8_t>({1}));
    assert(fake::nvs["qmc5883p/hardiron"]==std::vector<uint8_t>({1,2,3,4}));
    puts("PASS: independent persisted stable gauge IDs, defaults, configured presentation, invalid IDs, wraparound, bounded write retries and original namespaces preserved");
}
