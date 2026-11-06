#include <Arduino.h>
#include <Wire.h>
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
#include "display_settings.h"

namespace {
Display horizonDisplay(hardware::horizonCs), compassDisplay(hardware::compassCs);
AttitudeGauge horizon(horizonDisplay.canvas());
CompassGauge compass(compassDisplay.canvas());
IMU imu;
Magnetometer magnetometer;
DebouncedButton attitudeButton, compassButton;
DisplaySettings settings;
bool levelConfirmation=false;
uint32_t levelSetAt=0, levelSets=0, hShort=0, cShort=0, hLong=0, cLong=0;
bool watchdogReady=false, firstHorizon=true, firstCompass=true, preferHorizon=true;
uint32_t lastHorizon=0, lastCompass=0, dropped=0, loopMaxUs=0;
uint32_t horizonDrawMaxUs=0, compassDrawMaxUs=0;
uint8_t scanAddress=1;
enum Stage : uint32_t { Idle, ImuRead, MagRead, HorizonDraw, CompassDraw,
    HorizonTransfer, CompassTransfer, Logging, Storage, Scan, DisplayInit };
RTC_NOINIT_ATTR uint32_t retainedMagic;
RTC_NOINIT_ATTR volatile uint32_t retainedStage;
constexpr uint32_t magic=0x44554131;
uint32_t previousStage=UINT32_MAX;
void stage(Stage value) { retainedStage=value; }
void maximum(uint32_t &a,uint32_t b) { if(b>a) a=b; }

void handleButtons(DebouncedButton::Event hEvent, DebouncedButton::Event cEvent) {
    using Event=DebouncedButton::Event;
    if(hEvent==Event::ShortPress) {
        ++hShort; logLine("BUTTON gpio=13 action=short count=%lu\n",(unsigned long)hShort);
        settings.toggleAttitude(); horizonDisplay.cancel(); firstHorizon=true;
    } else if(hEvent==Event::LongPress) {
        ++hLong; logLine("BUTTON gpio=13 action=long count=%lu\n",(unsigned long)hLong);
        bool ok=imu.zero();
        if(ok) {
            ++levelSets; levelConfirmation=true; levelSetAt=millis();
            horizonDisplay.cancel(); firstHorizon=true; preferHorizon=true;
        }
        logLine("LEVEL ok=%d pitch=%.2f roll=%.2f sets=%lu imu_retry=%lu\n",ok,
                imu.relativePitch(),imu.relativeRoll(),(unsigned long)levelSets,
                (unsigned long)imu.stats.recoveries);
    }
    if(cEvent==Event::ShortPress) {
        ++cShort; logLine("BUTTON gpio=4 action=short count=%lu\n",(unsigned long)cShort);
        settings.toggleCompass(); compassDisplay.cancel(); firstCompass=true;
    } else if(cEvent==Event::LongPress) {
        ++cLong; logLine("BUTTON gpio=4 action=long count=%lu\n",(unsigned long)cLong);
        stage(Storage); magnetometer.startCalibration(true);
        compassDisplay.cancel(); firstCompass=true;
    }
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
        case 'c': case 'f': case 'x': case 's': case 'l': case 'e':
            stage(Storage); magnetometer.command(value); break;
        case 'i': scanAddress=1; break;
        case 'r': ESP.restart(); break;
        case '?': logLine("COMMANDS c=calibrate60s f=finish x=cancel s=saveCal l=loadCal e=eraseCal i=scan r=reboot; GPIO13 attitude, GPIO4 compass\n"); break;
    }
}

void render() {
    uint32_t now=millis();
    bool hDue=!horizonDisplay.busy() && (firstHorizon || now-lastHorizon>=67);
    bool cDue=!compassDisplay.busy() && (firstCompass || now-lastCompass>=50);
    // At most one CPU-heavy draw per loop; alternate priority when both are due.
    // Sampling and both transfer queues run regardless of which face is drawn.
    uint32_t started=micros();
    if(hDue && (!cDue || preferHorizon)) {
        stage(HorizonDraw); lastHorizon=now; firstHorizon=false;
        horizon.draw(imu.relativePitch(),imu.relativeRoll(),imu.fresh(),horizonDisplay.frames+1,levelConfirmation,settings.numerical);
        horizonDisplay.present(); preferHorizon=false;
        maximum(horizonDrawMaxUs,micros()-started);
    } else if(cDue) {
        stage(CompassDraw); lastCompass=now;
        auto state=magnetometer.displayState(); state.light=settings.light;
        if(compass.draw(state,firstCompass)) compassDisplay.present();
        firstCompass=false; preferHorizon=true;
        maximum(compassDrawMaxUs,micros()-started);
    }
}

void report(uint32_t now) {
    static uint32_t previousReport=0, previousHFrames=0, previousCFrames=0;
    float seconds=(now-previousReport)*0.001f;
    float hFps=(horizonDisplay.frames-previousHFrames)/seconds;
    float cFps=(compassDisplay.frames-previousCFrames)/seconds;
    previousReport=now; previousHFrames=horizonDisplay.frames; previousCFrames=compassDisplay.frames;
    auto offset=magnetometer.offsets();
    stage(Logging);
    if(!logLine("DUAL up=%lu theme=%s attitude=%s h_short=%lu c_short=%lu h_long=%lu c_long=%lu levels=%lu collecting=%d settings_pending=%d h_frame=%lu c_frame=%lu h_fps=%.1f c_fps=%.1f h_done=%lu c_done=%lu h_tiles=%lu c_tiles=%lu "
            "imu_fresh=%d imu_sample=%lu imu_err=%lu imu_retry=%lu pitch=%.2f roll=%.2f "
            "mag_fresh=%d mag_valid=%d mag_sample=%lu mag_err=%lu mag_retry=%lu overflow=%lu heading=%.2f "
            "xyz=%d,%d,%d cal=%d saved=%d offset=%.1f,%.1f,%.1f "
            "heap=%u min=%u maxalloc=%u max_us=%lu,%lu,%lu,%lu,%lu,%lu,%lu gap_us=%lu "
            "drop=%lu reset=%d prev=%lu wdt=%d\n",
            (unsigned long)now,settings.light ? "light" : "dark",settings.numerical ? "numerical" : "standard",
            (unsigned long)hShort,(unsigned long)cShort,(unsigned long)hLong,(unsigned long)cLong,
            (unsigned long)levelSets,magnetometer.isCollecting(),settings.pending(),
            (unsigned long)horizonDisplay.frames,(unsigned long)compassDisplay.frames,hFps,cFps,
            (unsigned long)horizonDisplay.completed,(unsigned long)compassDisplay.completed,
            (unsigned long)horizonDisplay.tiles,(unsigned long)compassDisplay.tiles,
            imu.fresh(),(unsigned long)imu.stats.samples,(unsigned long)imu.stats.errors,
            (unsigned long)imu.stats.recoveries,imu.relativePitch(),imu.relativeRoll(),
            magnetometer.fresh(),magnetometer.valid(),(unsigned long)magnetometer.stats.samples,
            (unsigned long)magnetometer.stats.errors,(unsigned long)magnetometer.stats.retries,
            (unsigned long)magnetometer.stats.overflows,magnetometer.headingValue(),
            magnetometer.raw.x,magnetometer.raw.y,magnetometer.raw.z,
            magnetometer.isCalibrated(),magnetometer.isSaved(),offset.x,offset.y,offset.z,
            (unsigned)ESP.getFreeHeap(),(unsigned)ESP.getMinFreeHeap(),(unsigned)ESP.getMaxAllocHeap(),
            (unsigned long)imu.stats.sensorMaxUs,(unsigned long)magnetometer.stats.i2cMaxUs,
            (unsigned long)horizonDrawMaxUs,(unsigned long)compassDrawMaxUs,
            (unsigned long)horizonDisplay.maxTransferUs,(unsigned long)compassDisplay.maxTransferUs,
            (unsigned long)loopMaxUs,(unsigned long)imu.stats.sampleGapMaxUs,(unsigned long)dropped,
            (int)esp_reset_reason(),(unsigned long)previousStage,watchdogReady)) ++dropped;
}
}

void setup() {
    previousStage=retainedMagic==magic ? retainedStage : UINT32_MAX;
    retainedMagic=magic; stage(Idle);
    beginDiagnostics();
    pinMode(hardware::attitudeButton,INPUT_PULLUP);
    pinMode(hardware::compassButton,INPUT_PULLUP);
    attitudeButton.begin(digitalRead(hardware::attitudeButton)==HIGH,millis());
    compassButton.begin(digitalRead(hardware::compassButton)==HIGH,millis());
    // GPIO5/6 and the CAN peripheral remain untouched.
    esp_task_wdt_config_t config{};
    config.timeout_ms=5000; config.trigger_panic=true;
    auto result=esp_task_wdt_init(&config);
    if(result==ESP_ERR_INVALID_STATE) result=esp_task_wdt_reconfigure(&config);
    if(result==ESP_OK) watchdogReady=esp_task_wdt_add(nullptr)==ESP_OK;
    bool busReady=Wire.begin(hardware::sda,hardware::scl,hardware::i2cHz);
    Wire.setTimeOut(hardware::i2cTimeoutMs);
    stage(Storage); settings.begin(); magnetometer.begin();
    stage(DisplayInit); Display::beginBus();
    horizonDisplay.begin();
    logLine("DISPLAY index=1 face=horizon cs=10 initialized=1 readback=unavailable\n");
    if(watchdogReady) esp_task_wdt_reset();
    compassDisplay.begin();
    logLine("DISPLAY index=2 face=compass cs=7 initialized=1 readback=unavailable\n");
    // Begin sampling only after the startup-only display initialization delays.
    imu.begin(); stage(Idle);
    logLine("Dual Gauge with Buttons v1.0.0 bus=%d cs=10,7 spi_hz=%u indexed_buffers=230400 tile_buffers=512 heap=%u reset=%d prev=%lu wdt=%d; keep MPU still at startup\n",
            busReady,(unsigned)hardware::spiHz,(unsigned)ESP.getFreeHeap(),
            (int)esp_reset_reason(),(unsigned long)previousStage,watchdogReady);
    command('?');
    if(watchdogReady) esp_task_wdt_reset();
}

void loop() {
    uint32_t started=micros();
    uint32_t buttonNow=millis();
    auto hEvent=attitudeButton.update(digitalRead(hardware::attitudeButton)==HIGH,buttonNow);
    auto cEvent=compassButton.update(digitalRead(hardware::compassButton)==HIGH,buttonNow);
    stage(ImuRead); imu.service();
    stage(MagRead); magnetometer.service();
    // Snapshot the just-filtered attitude, without an intervening sensor update.
    handleButtons(hEvent,cEvent);
    stage(Storage); settings.service();
    if(levelConfirmation && millis()-levelSetAt>=1500) {
        levelConfirmation=false; horizonDisplay.cancel(); firstHorizon=true;
    }
    if(Serial.available()) command(Serial.read());
    scan();
    render();
    // Eight changed tiles total at most, matching the proven single-screen
    // transfer budget. Every write completes and releases CS before the next.
    stage(HorizonTransfer); horizonDisplay.flushBatch(4);
    stage(CompassTransfer); compassDisplay.flushBatch(4);
    static uint32_t lastReport=0;
    uint32_t now=millis();
    if(now-lastReport>=1000) { lastReport=now; report(now); }
    stage(Idle); maximum(loopMaxUs,micros()-started);
    if(watchdogReady) esp_task_wdt_reset();
    delay(1);
}
