#include "display_settings.h"
#include <cassert>
#include <cstdio>
int main() {
    fake::nvs["qmc5883p/hardiron"]={1,2,3,4};
    DisplaySettings settings; settings.begin();
    assert(!settings.light && !settings.numerical && !settings.pending());
    settings.toggleCompass();
    assert(settings.light && !settings.numerical && settings.pending());
    fake::timeUs=299000; settings.service(); assert(fake::writeAttempts==0);
    fake::timeUs=300000; settings.service(); assert(!settings.pending());
    settings.toggleAttitude(); fake::timeUs=600000; settings.service();
    DisplaySettings reboot; reboot.begin();
    assert(reboot.light && reboot.numerical);
    assert(fake::nvs.at("qmc5883p/hardiron")==std::vector<uint8_t>({1,2,3,4}));
    auto writes=fake::writeAttempts;
    reboot.toggleCompass(); reboot.toggleCompass();
    fake::timeUs+=1000000; reboot.service(); assert(fake::writeAttempts==writes);
    reboot.toggleAttitude(); fake::failWrites=true;
    fake::timeUs+=300000; reboot.service(); assert(reboot.pending());
    writes=fake::writeAttempts;
    fake::timeUs+=4999000; reboot.service(); assert(fake::writeAttempts==writes);
    fake::failWrites=false; fake::timeUs+=1000; reboot.service(); assert(!reboot.pending());
    DisplaySettings again; again.begin(); assert(again.light && !again.numerical);
    puts("PASS: independent mode keys/defaults/reload, coalesced toggles, bounded save retries, calibration namespace unchanged");
}
