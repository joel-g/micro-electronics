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
#include "engine_gauge.h"
#include "can_receiver.h"

namespace {
Display horizonDisplay(hardware::horizonCs), compassDisplay(hardware::compassCs);
AttitudeGauge horizon(horizonDisplay.canvas());
CompassGauge compass(compassDisplay.canvas());
CompassGauge compassOnFirst(horizonDisplay.canvas());
AttitudeGauge horizonOnSecond(compassDisplay.canvas());
IMU imu;
Magnetometer magnetometer;
DebouncedButton attitudeButton, compassButton;
DisplaySettings settings;
EngineGauge firstEngine(horizonDisplay.canvas()), secondEngine(compassDisplay.canvas());
engine::Data engineData;
LinkGenericDashDecoder decoder;
CanReceiver can;
bool levelConfirmation=false;
uint32_t levelSetAt=0, levelSets=0, hShort=0, cShort=0, hLong=0, cLong=0;
bool watchdogReady=false, firstHorizon=true, firstCompass=true, preferHorizon=true;
uint32_t lastHorizon=0, lastCompass=0, dropped=0, loopMaxUs=0;
uint32_t horizonDrawMaxUs=0, compassDrawMaxUs=0;
uint8_t scanAddress=1;
enum Stage : uint32_t { Idle, ImuRead, MagRead, HorizonDraw, CompassDraw,
    HorizonTransfer, CompassTransfer, Logging, Storage, Scan, DisplayInit, CanService };
RTC_NOINIT_ATTR uint32_t retainedMagic;
RTC_NOINIT_ATTR volatile uint32_t retainedStage;
constexpr uint32_t magic=0x44474331;
uint32_t previousStage=UINT32_MAX;
void stage(Stage value) { retainedStage=value; }
void maximum(uint32_t &a,uint32_t b) { if(b>a) a=b; }

void handleButtons(DebouncedButton::Event hEvent, DebouncedButton::Event cEvent) {
    using Event=DebouncedButton::Event;
    if(hEvent==Event::ShortPress) {
        ++hShort; logLine("BUTTON gpio=13 action=short count=%lu\n",(unsigned long)hShort);
        settings.cycleFirst(); levelConfirmation=false; horizonDisplay.cancel(); firstHorizon=true;
    } else if(hEvent==Event::LongPress) {
        ++hLong; logLine("BUTTON gpio=13 action=long count=%lu\n",(unsigned long)hLong);
        bool ok=settings.first==Gauge::Horizon && imu.zero();
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
        settings.cycleSecond(); compassDisplay.cancel(); firstCompass=true;
    } else if(cEvent==Event::LongPress) {
        ++cLong; logLine("BUTTON gpio=4 action=long count=%lu\n",(unsigned long)cLong);
        logLine("BUTTON gpio=4 long=unassigned gauge=%s\n",gaugeConfig::name(settings.second));
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
        case 'j': can.listIds(); break;
        case 'v':
            logLine("CONFIG hash=%s profile=%s ecu=%s firmware=%s can_bus=%u channel=%u id=%lu format=%s declared_rate_hz=%u faces=%u,%u\n",
                instrumentConfig::hash,instrumentConfig::profile,instrumentConfig::ecuModel,instrumentConfig::ecuFirmware,
                instrumentConfig::ecuBus,instrumentConfig::ecuChannel,(unsigned long)instrumentConfig::canId,
                instrumentConfig::canExtended?"extended":"standard",instrumentConfig::ecuRateHz,
                unsigned(sizeof(gaugeConfig::first)/sizeof(Gauge)),unsigned(sizeof(gaugeConfig::second)/sizeof(Gauge)));
            break;
        case '?': logLine("COMMANDS c=calibrate60s f=finish x=cancel s=saveCal l=loadCal e=eraseCal i=scan r=reboot j=listCANidsAndMuxSnapshots v=config; GPIO13 display1, GPIO4 display2\n"); break;
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
        stage(HorizonDraw); lastHorizon=now; bool firstHorizonDraw=firstHorizon; firstHorizon=false;
        bool painted=true;
        if(settings.first==Gauge::Horizon)
            horizon.draw(imu.relativePitch(),imu.relativeRoll(),imu.fresh(),horizonDisplay.frames+1,levelConfirmation,settings.numerical);
        else if(settings.first==Gauge::Compass) {
            auto state=magnetometer.displayState();state.light=gaugeConfig::compassLight;
            painted=compassOnFirst.draw(state,firstHorizonDraw);
        } else painted=firstEngine.draw(settings.first,engineData,now,firstHorizonDraw);
        if(painted) horizonDisplay.present();
        preferHorizon=false;
        maximum(horizonDrawMaxUs,micros()-started);
    } else if(cDue) {
        stage(CompassDraw); lastCompass=now;
        bool painted;
        if(settings.second==Gauge::Compass) {
            auto state=magnetometer.displayState(); state.light=gaugeConfig::compassLight;
            painted=compass.draw(state,firstCompass);
        } else if(settings.second==Gauge::Horizon) {
            horizonOnSecond.draw(imu.relativePitch(),imu.relativeRoll(),imu.fresh(),compassDisplay.frames+1,levelConfirmation,settings.numerical);
            painted=true;
        } else painted=secondEngine.draw(settings.second,engineData,now,firstCompass);
        if(painted) compassDisplay.present();
        firstCompass=false; preferHorizon=true;
        maximum(compassDrawMaxUs,micros()-started);
    }
}

void engineReport(uint32_t now) {
    using S=engine::Signal;
    const S signals[]={S::Rpm,S::CoolantC,S::MapKpa,S::BaroKpa,S::BatteryV,S::IatC};
    char values[6][20]{};
    for(unsigned i=0;i<6;++i) {
        const auto &channel=engineData.get(signals[i]);
        switch(channel.state(now,gaugeConfig::staleMs)) {
            case engine::State::Never:snprintf(values[i],sizeof(values[i]),"UNAVAILABLE");break;
            case engine::State::Invalid:snprintf(values[i],sizeof(values[i]),"INVALID");break;
            case engine::State::Stale:snprintf(values[i],sizeof(values[i]),"STALE");break;
            case engine::State::Valid:snprintf(values[i],sizeof(values[i]),"%.2f",channel.value);break;
        }
    }
    logLine("ENGINE up=%lu rpm=%s coolant_c=%s map_kpa=%s baro_kpa=%s battery_v=%s iat_c=%s matched=%lu malformed=%lu unknown_mux=%lu invalid_values=%lu optional_sources=%d,%d,%d\n",
        (unsigned long)now,values[0],values[1],values[2],values[3],values[4],values[5],
        (unsigned long)decoder.matched,(unsigned long)decoder.malformed,
        (unsigned long)decoder.unknownMux,(unsigned long)decoder.invalidValues,
        instrumentConfig::enableOil,instrumentConfig::enableFuel,instrumentConfig::enableLambda);
}

void report(uint32_t now) {
    static uint32_t previousReport=0, previousHFrames=0, previousCFrames=0;
    float seconds=(now-previousReport)*0.001f;
    float hFps=(horizonDisplay.frames-previousHFrames)/seconds;
    float cFps=(compassDisplay.frames-previousCFrames)/seconds;
    previousReport=now; previousHFrames=horizonDisplay.frames; previousCFrames=compassDisplay.frames;
    auto offset=magnetometer.offsets();
    stage(Logging);
    if(!logLine("DUAL up=%lu display1=%s display2=%s theme=%s attitude=%s h_short=%lu c_short=%lu h_long=%lu c_long=%lu levels=%lu collecting=%d settings_pending=%d h_frame=%lu c_frame=%lu h_fps=%.1f c_fps=%.1f h_done=%lu c_done=%lu h_tiles=%lu c_tiles=%lu "
            "imu_fresh=%d imu_sample=%lu imu_err=%lu imu_retry=%lu pitch=%.2f roll=%.2f "
            "mag_fresh=%d mag_valid=%d mag_sample=%lu mag_err=%lu mag_retry=%lu overflow=%lu heading=%.2f "
            "xyz=%d,%d,%d cal=%d saved=%d offset=%.1f,%.1f,%.1f "
            "heap=%u min=%u maxalloc=%u max_us=%lu,%lu,%lu,%lu,%lu,%lu,%lu gap_us=%lu "
            "drop=%lu reset=%d prev=%lu wdt=%d\n",
            (unsigned long)now,gaugeConfig::name(settings.first),gaugeConfig::name(settings.second),settings.light?"light":"dark",settings.numerical ? "numerical" : "standard",
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
    // CAN acknowledgment is selected explicitly by the build environment.
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
    logLine("DISPLAY index=1 face=%s cs=10 initialized=1 readback=unavailable\n",gaugeConfig::name(settings.first));
    if(watchdogReady) esp_task_wdt_reset();
    compassDisplay.begin();
    logLine("DISPLAY index=2 face=%s cs=7 initialized=1 readback=unavailable\n",gaugeConfig::name(settings.second));
    // Begin sampling only after the startup-only display initialization delays.
    imu.begin(); stage(CanService); can.begin(); stage(Idle);
    logLine("Dual Gauge CAN v1.0.0 bus=%d cs=10,7 spi_hz=%u indexed_buffers=230400 tile_buffers=512 heap=%u reset=%d prev=%lu wdt=%d; keep MPU still at startup\n",
            busReady,(unsigned)hardware::spiHz,(unsigned)ESP.getFreeHeap(),
            (int)esp_reset_reason(),(unsigned long)previousStage,watchdogReady);
    command('v');
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
    stage(CanService); can.service(decoder,engineData);
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
    if(now-lastReport>=1000) { lastReport=now; report(now); can.report(); engineReport(now); }
    stage(Idle); maximum(loopMaxUs,micros()-started);
    if(watchdogReady) esp_task_wdt_reset();
    delay(1);
}
