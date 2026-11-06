#pragma once
#include <Preferences.h>
#include <Arduino.h>

// Independent keys in a new namespace; never touches saved compass calibration.
class DisplaySettings {
public:
    void begin();
    void toggleCompass();
    void toggleAttitude();
    void service();
    bool light=false, numerical=false;
    bool pending() const { return compassDirty || attitudeDirty; }
private:
    Preferences preferences;
    bool ready=false, savedLight=false, savedNumerical=false;
    bool compassDirty=false, attitudeDirty=false;
    uint32_t compassChanged=0, attitudeChanged=0;
    uint32_t compassDelay=300, attitudeDelay=300;
};
