#include "display_settings.h"
#include "diagnostics.h"

void DisplaySettings::begin() {
    ready=preferences.begin("dual-buttons",false);
    light=savedLight=ready && preferences.getUChar("compass",0)==1;
    numerical=savedNumerical=ready && preferences.getUChar("attitude",0)==1;
    logLine("SETTINGS loaded nvs=%d compass=%s attitude=%s\n",ready,
            light ? "light" : "dark", numerical ? "numerical" : "standard");
}
void DisplaySettings::toggleCompass() {
    light=!light; compassDirty=light!=savedLight; compassChanged=millis(); compassDelay=300;
    logLine("THEME compass=%s\n",light ? "light" : "dark");
}
void DisplaySettings::toggleAttitude() {
    numerical=!numerical; attitudeDirty=numerical!=savedNumerical;
    attitudeChanged=millis(); attitudeDelay=300;
    logLine("MODE attitude=%s\n",numerical ? "numerical" : "standard");
}
void DisplaySettings::service() {
    if(!ready) return;
    uint32_t now=millis();
    // At most one changed key per loop; coalesce fast toggles and bound retries.
    if(compassDirty && now-compassChanged>=compassDelay) {
        bool ok=preferences.putUChar("compass",light)==1 &&
                preferences.getUChar("compass",255)==static_cast<uint8_t>(light);
        if(ok) { savedLight=light; compassDirty=false; }
        else { compassChanged=now; compassDelay=5000; }
        logLine("SETTINGS saved key=compass ok=%d value=%d\n",ok,light);
    } else if(attitudeDirty && now-attitudeChanged>=attitudeDelay) {
        bool ok=preferences.putUChar("attitude",numerical)==1 &&
                preferences.getUChar("attitude",255)==static_cast<uint8_t>(numerical);
        if(ok) { savedNumerical=numerical; attitudeDirty=false; }
        else { attitudeChanged=now; attitudeDelay=5000; }
        logLine("SETTINGS saved key=attitude ok=%d value=%d\n",ok,numerical);
    }
}
