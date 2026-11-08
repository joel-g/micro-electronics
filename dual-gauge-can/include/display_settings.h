#pragma once
#include <Preferences.h>
#include <Arduino.h>
#include "gauge_config.h"
class DisplaySettings {
public:
    void begin();
    void cycleFirst();
    void cycleSecond();
    void service();
    Gauge first=gaugeConfig::first[0], second=gaugeConfig::second[0];
    bool light=false,numerical=false;
    bool pending() const {return firstDirty || secondDirty;}
private:
    Preferences preferences;
    bool ready=false,firstDirty=false,secondDirty=false;
    Gauge savedFirst=gaugeConfig::first[0],savedSecond=gaugeConfig::second[0];
    uint32_t firstChanged=0,secondChanged=0,firstDelay=300,secondDelay=300;
};
