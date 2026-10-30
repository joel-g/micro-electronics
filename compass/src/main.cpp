#include <Arduino.h>
#include <Wire.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include "hardware.h"
#include "diagnostics.h"
#include "qmc5883p.h"
#include "compass_math.h"
#include "compass_display.h"
#include "calibration_store.h"

#ifndef COMPASS_WATCHDOG_TEST
#define COMPASS_WATCHDOG_TEST 0
#endif

namespace {
Qmc5883p magnetometer;
CompassDisplay display;
CalibrationStore storage;
compass::HardIronCalibration calibration;
compass::Vector offset{};
MagneticSample raw;
bool found[128] = {};
uint8_t scanAddress = 1;
bool busReady = false, sensorReady = false, calibrated = false, saved = false;
bool receivedSinceBegin = false;
bool collecting = false, rejected = false, headingReady = false, fieldValid = false;
bool watchdogReady = false, storageReady = false;
uint32_t lastRead = 0, lastSample = 0, lastBegin = 0, sensorStarted = 0;
uint32_t calibrationStarted = 0, rejectedAt = 0;
float heading = 0, targetHeading = 0;
constexpr uint32_t calibrationDuration = 60000;
struct Counters {
    uint32_t samples = 0, errors = 0, overflows = 0, retries = 0, drops = 0;
    uint32_t streak = 0, i2cMaxUs = 0, loopMaxUs = 0, loops = 0;
} stats;
enum Stage : uint32_t { Idle, Sensor, Draw, Transfer, Logging, Recovery, Storage, Scan, WatchdogTest };
RTC_NOINIT_ATTR uint32_t retainedMagic;
RTC_NOINIT_ATTR volatile uint32_t retainedStage;
uint32_t previousStage = UINT32_MAX;
constexpr uint32_t magic = 0x434F4D50;
void stage(Stage value) { retainedStage = value; }
void maxTime(uint32_t &a, uint32_t b) { if (b > a) a = b; }
bool fresh() { return sensorReady && receivedSinceBegin && millis() - lastSample < 250; }

void serviceScan() {
    if (scanAddress >= 127) return;
    stage(Scan);
    Wire.beginTransmission(scanAddress);
    uint8_t error = Wire.endTransmission();
    if (!error) {
        found[scanAddress] = true;
        logLine("SCAN ACK=0x%02X%s\n", scanAddress, scanAddress == 0x68 ? " MPU present; ignored" : "");
    } else if (error != 2) {
        logLine("SCAN address=0x%02X error=%u\n", scanAddress, error);
    }
    if (++scanAddress == 127) {
        logLine("SCAN DONE qmc2c=%d mpu68=%d qmcL0d=%d hmc1e=%d\n",
                found[0x2c], found[0x68], found[0x0d], found[0x1e]);
        lastBegin = millis() - 1000;
    }
    stage(Idle);
}

void serviceSensor() {
    if (scanAddress < 127 && !sensorReady) return;
    uint32_t now = millis();
    if (!sensorReady) {
        if (now - lastBegin < 1000) return;
        lastBegin = now;
        stage(Recovery);
        if (stats.retries || !busReady) {
            Wire.end();
            busReady = Wire.begin(hardware::sda, hardware::scl, hardware::i2cHz);
            Wire.setTimeOut(hardware::i2cTimeoutMs);
        }
        ++stats.retries;
        sensorReady = busReady && magnetometer.begin();
        receivedSinceBegin = false;
        sensorStarted = now;
        stats.streak = 0;
        headingReady = false;
        logLine("MAG detected=%d address=0x2C id=0x%02X ctrl1=0x%02X ctrl2=0x%02X error=%u\n",
                sensorReady, magnetometer.chipId, magnetometer.control1,
                magnetometer.control2, magnetometer.busError);
        stage(Idle);
        return;
    }
    if (now - lastRead < 10) return;
    lastRead = now;
    stage(Sensor);
    uint32_t started = micros();
    Qmc5883p::Result result = magnetometer.read(raw);
    maxTime(stats.i2cMaxUs, micros() - started);
    if (result == Qmc5883p::Result::Sample) {
        lastSample = now;
        receivedSinceBegin = true;
        ++stats.samples;
        stats.streak = 0;
        compass::Vector value{float(raw.x), float(raw.y), float(raw.z)};
        if (collecting) calibration.add(value);
        fieldValid = compass::heading(value, offset, targetHeading);
        if (fieldValid && !headingReady) { heading = targetHeading; headingReady = true; }
    } else if (result == Qmc5883p::Result::BusError) {
        ++stats.errors;
        if (++stats.streak >= 5) sensorReady = false;
    } else if (result == Qmc5883p::Result::Overflow) {
        ++stats.overflows;
        fieldValid = false;
    }
    // An ACK without new DRDY samples is not a healthy sensor.
    uint32_t newest = receivedSinceBegin ? lastSample : sensorStarted;
    if (now - newest > 1000) sensorReady = false;
    stage(Idle);
}

void finishCalibration() {
    if (!collecting) return;
    compass::Vector candidate;
    bool valid = fresh() && calibration.finish(millis() - calibrationStarted, candidate);
    collecting = false;
    if (valid) {
        offset = candidate;
        calibrated = true; saved = false; rejected = false; headingReady = false;
        logLine("CAL applied offset=%.1f,%.1f,%.1f samples=%lu; RAM only, send s to save\n",
                offset.x, offset.y, offset.z, (unsigned long)calibration.count);
    } else {
        rejected = true; rejectedAt = millis();
        auto extent = calibration.span();
        logLine("CAL REJECTED samples=%lu span=%.0f,%.0f,%.0f; rotate all axes for at least 10s; prior offsets kept\n",
                (unsigned long)calibration.count, extent.x, extent.y, extent.z);
    }
}

void command(char value) {
    switch (value) {
#if COMPASS_WATCHDOG_TEST
        case 'w':
            if (!watchdogReady) { logLine("WATCHDOG TEST refused: watchdog unavailable\n"); break; }
            logLine("WATCHDOG TEST: stalling loop without feeding watchdog\n");
            stage(WatchdogTest);
            // Keep interrupts and other tasks running; only the watchdog can
            // recover this loop. Never compiled into normal firmware.
            for (;;) delay(10);
#endif
        case 'c':
            if (!fresh()) { logLine("CAL unavailable: magnetometer not fresh\n"); break; }
            calibration.start(); collecting = true; rejected = false;
            calibrationStarted = millis();
            logLine("CAL START 60 seconds: slowly rotate/tumble through all axes away from magnetic objects\n");
            break;
        case 'f': finishCalibration(); break;
        case 'x': collecting = false; logLine("CAL cancelled; prior offsets kept\n"); break;
        case 's':
            if (!calibrated || collecting) { logLine("SAVE refused: finish a valid calibration first\n"); break; }
            stage(Storage); saved = storage.save(offset); stage(Idle);
            logLine("SAVE ok=%d offset=%.1f,%.1f,%.1f\n", saved, offset.x, offset.y, offset.z);
            break;
        case 'l': {
            if (collecting) break;
            compass::Vector loaded;
            stage(Storage); bool ok = storage.load(loaded); stage(Idle);
            if (ok) { offset = loaded; calibrated = saved = true; headingReady = false; }
            logLine("LOAD ok=%d offset=%.1f,%.1f,%.1f\n", ok, offset.x, offset.y, offset.z);
            break;
        }
        case 'e':
            stage(Storage);
            if (storage.clear()) {
                offset = {}; calibrated = saved = collecting = false; headingReady = false;
                logLine("CAL erased from RAM and NVS\n");
            } else logLine("CAL erase failed; existing calibration kept\n");
            stage(Idle);
            break;
        case 'i': memset(found, 0, sizeof(found)); scanAddress = 1; break;
        case 'r': ESP.restart(); break;
        case '?':
            logLine("COMPASS commands c=calibrate60s f=finish x=cancel s=save l=load e=erase i=scan r=reboot ?=help\n");
            break;
    }
}

DisplayState displayState() {
    DisplayState state;
    state.heading = heading; state.saved = saved;
    if (scanAddress < 127 && !sensorReady) state.status = CompassStatus::Waiting;
    else if (!fresh()) state.status = CompassStatus::Fault;
    else if (collecting) {
        state.status = CompassStatus::Calibrating;
        state.seconds = (calibrationDuration - (millis() - calibrationStarted) + 999) / 1000;
    } else if (rejected && millis() - rejectedAt < 5000) state.status = CompassStatus::CalibrationFailed;
    else if (!fieldValid) state.status = CompassStatus::WeakField;
    else state.status = calibrated ? CompassStatus::Live : CompassStatus::Uncalibrated;
    return state;
}

void report() {
    auto span = calibration.span();
    stage(Logging);
    bool sent = logLine("COMPASS up=%lu sample=%lu frame=%lu tiles=%lu fresh=%d valid=%d "
            "xyz=%d,%d,%d heading=%.2f target=%.2f cal=%d saved=%d collecting=%d "
            "span=%.0f,%.0f,%.0f offset=%.1f,%.1f,%.1f "
            "err=%lu overflow=%lu retry=%lu drop=%lu bus=%u id=0x%02X status=0x%02X "
            "scan=%d mpu=%d heap=%u min=%u max_us=%lu,%lu,%lu,%lu reset=%d prev=%lu wdt=%d nvs=%d test=%d\n",
            (unsigned long)millis(), (unsigned long)stats.samples, (unsigned long)display.frames,
            (unsigned long)display.tiles, fresh(), fieldValid,
            raw.x, raw.y, raw.z, heading, targetHeading, calibrated, saved, collecting,
            span.x, span.y, span.z, offset.x, offset.y, offset.z,
            (unsigned long)stats.errors, (unsigned long)stats.overflows, (unsigned long)stats.retries,
            (unsigned long)stats.drops, magnetometer.busError, magnetometer.chipId, magnetometer.status,
            scanAddress >= 127, found[0x68], (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
            (unsigned long)stats.i2cMaxUs, (unsigned long)display.maxDrawUs,
            (unsigned long)display.maxTransferUs, (unsigned long)stats.loopMaxUs,
            (int)esp_reset_reason(), (unsigned long)previousStage, watchdogReady, storageReady,
            COMPASS_WATCHDOG_TEST);
    if (!sent) ++stats.drops;
    stage(Idle);
}
}

void setup() {
    previousStage = retainedMagic == magic ? retainedStage : UINT32_MAX;
    retainedMagic = magic; stage(Idle);
    beginDiagnostics();
    esp_task_wdt_config_t config{};
    config.timeout_ms = 5000; config.trigger_panic = true;
    auto result = esp_task_wdt_init(&config);
    if (result == ESP_ERR_INVALID_STATE) result = esp_task_wdt_reconfigure(&config);
    if (result == ESP_OK) watchdogReady = esp_task_wdt_add(nullptr) == ESP_OK;
    stage(Storage);
    storageReady = storage.begin();
    saved = calibrated = storage.load(offset);
    stage(Recovery);
    busReady = Wire.begin(hardware::sda, hardware::scl, hardware::i2cHz);
    Wire.setTimeOut(hardware::i2cTimeoutMs);
    stage(Draw); display.begin(); stage(Idle);
    logLine("Standalone Compass v1.0.0 QMC5883P only; magnetic north, LEVEL ONLY; calibration_loaded=%d\n", calibrated);
    command('?');
    if (watchdogReady) esp_task_wdt_reset();
}

void loop() {
    uint32_t started = micros();
    serviceScan();
    serviceSensor();
    if (Serial.available()) command(Serial.read());
    if (collecting && millis() - calibrationStarted >= calibrationDuration) finishCalibration();
    static uint32_t lastAnimation = micros();
    uint32_t now = micros();
    float dt = (now - lastAnimation) * 0.000001f;
    lastAnimation = now;
    if (fresh() && fieldValid && !collecting) heading = compass::smooth(heading, targetHeading, dt);
    static uint32_t lastFrame = 0;
    if (!display.busy() && millis() - lastFrame >= 50) {
        lastFrame = millis();
        stage(Draw); display.prepare(displayState());
    }
    stage(Transfer); display.flushBatch(); stage(Idle);
    static uint32_t lastReport = 0;
    if (millis() - lastReport >= 1000) { lastReport = millis(); report(); }
    maxTime(stats.loopMaxUs, micros() - started);
    ++stats.loops;
    if (watchdogReady) esp_task_wdt_reset();
    delay(1);
}
