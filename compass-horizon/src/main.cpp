#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include "hardware.h"
#include "diagnostics.h"
#include "display.h"
#include "compass_gauge.h"
#include "attitude_gauge.h"
#include "imu.h"
#include "magnetometer.h"
#include "button.h"

namespace {
constexpr int buttonPin=13;
enum class Mode : uint8_t { Compass=0, Horizon=1 };
Mode mode=Mode::Compass, storedMode=Mode::Compass;
Display display;
CompassGauge compassGauge(display.canvas());
AttitudeGauge attitudeGauge(display.canvas());
IMU imu;
Magnetometer magnetometer;
DebouncedButton button;
Preferences preferences;
bool modeStoreReady=false, modeDirty=false, faceDirty=true, watchdogReady=false;
uint32_t modeChangedAt=0, presses=0, switches=0, dropped=0, loopMaxUs=0, drawMaxUs=0;
uint32_t modeSaveDelay=500;
uint32_t lastFrame=0;
uint8_t scanAddress=1;
enum Stage : uint32_t { Idle, Input, ImuRead, MagRead, Draw, Transfer, Logging, Storage, Scan };
RTC_NOINIT_ATTR uint32_t retainedMagic;
RTC_NOINIT_ATTR volatile uint32_t retainedStage;
constexpr uint32_t magic=0x43485231;
uint32_t previousStage=UINT32_MAX;
void stage(Stage value) { retainedStage=value; }
void maximum(uint32_t &a,uint32_t b) { if(b>a) a=b; }
const char *modeName() { return mode==Mode::Compass ? "compass" : "horizon"; }

void toggleMode(const char *source) {
    mode=mode==Mode::Compass ? Mode::Horizon : Mode::Compass;
    ++switches; modeChangedAt=millis(); modeSaveDelay=500; modeDirty=mode!=storedMode;
    // All transfers are synchronous. Stop any remaining old-face tiles; shadow
    // bytes already reflect what was actually sent, so the new face replaces it.
    display.cancel(); faceDirty=true;
    logLine("MODE source=%s mode=%s switches=%lu presses=%lu\n",source,modeName(),
            (unsigned long)switches,(unsigned long)presses);
}

void saveMode() {
    if(!modeDirty || !modeStoreReady || millis()-modeChangedAt<modeSaveDelay) return;
    stage(Storage);
    bool ok=preferences.putUChar("mode",static_cast<uint8_t>(mode))==1 &&
            preferences.getUChar("mode",255)==static_cast<uint8_t>(mode);
    if(ok) { storedMode=mode; modeDirty=false; }
    else { modeChangedAt=millis(); modeSaveDelay=5000; }
    logLine("MODE save=%d mode=%s\n",ok,modeName());
}

void scan() {
    if(scanAddress>=127) return;
    stage(Scan);
    Wire.beginTransmission(scanAddress);
    uint8_t error=Wire.endTransmission();
    if(!error) logLine("SCAN ACK=0x%02X\n",scanAddress);
    else if(error!=2) logLine("SCAN address=0x%02X error=%u\n",scanAddress,error);
    if(++scanAddress==127) logLine("SCAN DONE\n");
}

void command(char value) {
    switch(value) {
        case 'm': toggleMode("serial"); break;
        case 'z': logLine("MPU zero=%d\n",imu.zero()); break;
        case 'c': case 'f': case 'x': case 's': case 'l': case 'e':
            stage(Storage); magnetometer.command(value); break;
        case 'i': scanAddress=1; break;
        case 'r': ESP.restart(); break;
        case '?': logLine("COMMANDS m=toggle z=zeroMPU c=calibrate60s f=finish x=cancel s=saveCal l=loadCal e=eraseCal i=scan r=reboot\n"); break;
    }
}

void report() {
    auto offset=magnetometer.offsets();
    stage(Logging);
    if(!logLine("INSTRUMENT up=%lu mode=%s btn=%d presses=%lu switches=%lu mode_saved=%d "
            "frame=%lu tiles=%lu imu_fresh=%d imu_sample=%lu imu_err=%lu imu_retry=%lu "
            "pitch=%.2f roll=%.2f mag_fresh=%d mag_valid=%d mag_sample=%lu mag_err=%lu "
            "mag_retry=%lu overflow=%lu heading=%.2f xyz=%d,%d,%d cal=%d saved=%d "
            "offset=%.1f,%.1f,%.1f heap=%u min=%u max_us=%lu,%lu,%lu,%lu,%lu gap_us=%lu "
            "drop=%lu reset=%d prev=%lu wdt=%d\n",
            (unsigned long)millis(),modeName(),digitalRead(buttonPin),(unsigned long)presses,
            (unsigned long)switches,modeStoreReady && mode==storedMode,
            (unsigned long)display.frames,(unsigned long)display.tiles,imu.fresh(),
            (unsigned long)imu.stats.samples,(unsigned long)imu.stats.errors,
            (unsigned long)imu.stats.recoveries,imu.relativePitch(),imu.relativeRoll(),
            magnetometer.fresh(),magnetometer.valid(),(unsigned long)magnetometer.stats.samples,
            (unsigned long)magnetometer.stats.errors,(unsigned long)magnetometer.stats.retries,
            (unsigned long)magnetometer.stats.overflows,magnetometer.headingValue(),
            magnetometer.raw.x,magnetometer.raw.y,magnetometer.raw.z,
            magnetometer.isCalibrated(),magnetometer.isSaved(),offset.x,offset.y,offset.z,
            (unsigned)ESP.getFreeHeap(),(unsigned)ESP.getMinFreeHeap(),
            (unsigned long)imu.stats.sensorMaxUs,(unsigned long)magnetometer.stats.i2cMaxUs,
            (unsigned long)drawMaxUs,(unsigned long)display.maxTransferUs,(unsigned long)loopMaxUs,
            (unsigned long)imu.stats.sampleGapMaxUs,(unsigned long)dropped,
            (int)esp_reset_reason(),(unsigned long)previousStage,watchdogReady)) ++dropped;
}
}

void setup() {
    previousStage=retainedMagic==magic ? retainedStage : UINT32_MAX;
    retainedMagic=magic; stage(Idle);
    beginDiagnostics();
    pinMode(buttonPin,INPUT_PULLUP);
    button.begin(digitalRead(buttonPin)==HIGH,millis());
    esp_task_wdt_config_t config{};
    config.timeout_ms=5000; config.trigger_panic=true;
    auto result=esp_task_wdt_init(&config);
    if(result==ESP_ERR_INVALID_STATE) result=esp_task_wdt_reconfigure(&config);
    if(result==ESP_OK) watchdogReady=esp_task_wdt_add(nullptr)==ESP_OK;
    stage(Storage);
    modeStoreReady=preferences.begin("compass-horizon",false);
    uint8_t savedMode=modeStoreReady ? preferences.getUChar("mode",0) : 0;
    mode=storedMode=savedMode==1 ? Mode::Horizon : Mode::Compass;
    magnetometer.begin();
    bool busReady=Wire.begin(hardware::sda,hardware::scl,hardware::i2cHz);
    Wire.setTimeOut(hardware::i2cTimeoutMs);
    imu.begin();
    stage(Draw); display.begin(); stage(Idle);
    logLine("Compass & Horizon v1.0.0 mode=%s mode_nvs=%d bus=%d button=GPIO13 activeLOW debounce=40ms; keep MPU level at startup\n",
            modeName(),modeStoreReady,busReady);
    command('?');
    if(watchdogReady) esp_task_wdt_reset();
}

void loop() {
    uint32_t started=micros();
    stage(Input);
    if(button.update(digitalRead(buttonPin)==HIGH,millis())) { ++presses; toggleMode("button"); }
    scan();
    stage(ImuRead); imu.service();
    stage(MagRead); magnetometer.service();
    if(Serial.available()) command(Serial.read());
    saveMode();
    uint32_t frameMs=mode==Mode::Compass ? 50 : 67;
    if(!display.busy() && (faceDirty || millis()-lastFrame>=frameMs)) {
        lastFrame=millis(); stage(Draw);
        uint32_t drawStarted=micros();
        bool painted=true;
        if(mode==Mode::Compass) painted=compassGauge.draw(magnetometer.displayState(),faceDirty);
        else attitudeGauge.draw(imu.relativePitch(),imu.relativeRoll(),imu.fresh(),display.frames+1);
        faceDirty=false;
        maximum(drawMaxUs,micros()-drawStarted);
        if(painted) display.present();
    }
    stage(Transfer); display.flushBatch();
    static uint32_t lastReport=0;
    if(millis()-lastReport>=1000) { lastReport=millis(); report(); }
    stage(Idle); maximum(loopMaxUs,micros()-started);
    if(watchdogReady) esp_task_wdt_reset();
    delay(1);
}
