#include "display_settings.h"
#include "diagnostics.h"
void DisplaySettings::begin() {
    light=gaugeConfig::compassLight;
    numerical=gaugeConfig::horizonNumerical;
    ready=preferences.begin("dual-can",false);
    if(ready) {
        first=static_cast<Gauge>(preferences.getUChar("display1",uint8_t(gaugeConfig::first[0])));
        second=static_cast<Gauge>(preferences.getUChar("display2",uint8_t(gaugeConfig::second[0])));
        if(!gaugeConfig::contains(gaugeConfig::first,first)) first=gaugeConfig::first[0];
        if(!gaugeConfig::contains(gaugeConfig::second,second)) second=gaugeConfig::second[0];
    }
    savedFirst=first;savedSecond=second;
    logLine("SETTINGS loaded nvs=%d display1=%s display2=%s compass=%s attitude=%s\n",ready,
        gaugeConfig::name(first),gaugeConfig::name(second),light?"light":"dark",numerical?"numerical":"standard");
}
void DisplaySettings::cycleFirst() {
    first=gaugeConfig::next(gaugeConfig::first,first);firstDirty=first!=savedFirst;firstChanged=millis();firstDelay=300;
    logLine("MODE display=1 gauge=%s\n",gaugeConfig::name(first));
}
void DisplaySettings::cycleSecond() {
    second=gaugeConfig::next(gaugeConfig::second,second);secondDirty=second!=savedSecond;secondChanged=millis();secondDelay=300;
    logLine("MODE display=2 gauge=%s\n",gaugeConfig::name(second));
}
void DisplaySettings::service() {
    if(!ready) return;
    uint32_t now=millis();
    if(firstDirty && now-firstChanged>=firstDelay) {
        bool ok=preferences.putUChar("display1",uint8_t(first))==1 && preferences.getUChar("display1",255)==uint8_t(first);
        if(ok) {savedFirst=first;firstDirty=false;} else {firstChanged=now;firstDelay=5000;}
        logLine("SETTINGS saved display=1 ok=%d gauge=%s\n",ok,gaugeConfig::name(first));
    } else if(secondDirty && now-secondChanged>=secondDelay) {
        bool ok=preferences.putUChar("display2",uint8_t(second))==1 && preferences.getUChar("display2",255)==uint8_t(second);
        if(ok) {savedSecond=second;secondDirty=false;} else {secondChanged=now;secondDelay=5000;}
        logLine("SETTINGS saved display=2 ok=%d gauge=%s\n",ok,gaugeConfig::name(second));
    }
}
