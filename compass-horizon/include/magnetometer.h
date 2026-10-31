#pragma once
#include "qmc5883p.h"
#include "compass_math.h"
#include "compass_gauge.h"
#include "calibration_store.h"

class Magnetometer {
public:
    void begin();
    void service();
    void command(char value);
    bool fresh() const { return sensorReady && receivedSinceBegin && millis()-lastSample<250; }
    bool valid() const { return fieldValid; }
    bool isCalibrated() const { return calibrated; }
    bool isSaved() const { return saved; }
    float headingValue() const { return heading; }
    compass::Vector offsets() const { return offset; }
    CompassState displayState() const;
    MagneticSample raw;
    struct Counters {
        uint32_t samples=0, errors=0, overflows=0, retries=0, streak=0, i2cMaxUs=0;
    } stats;
private:
    Qmc5883p magnetometer;
    CalibrationStore storage;
    compass::HardIronCalibration calibration;
    compass::Vector offset{};
    bool sensorReady=false, calibrated=false, saved=false, receivedSinceBegin=false;
    bool collecting=false, rejected=false, headingReady=false, fieldValid=false;
    uint32_t lastRead=0, lastSample=0, lastBegin=0, sensorStarted=0;
    uint32_t calibrationStarted=0, rejectedAt=0, lastAnimation=0;
    float heading=0, targetHeading=0;
    static constexpr uint32_t calibrationDuration=60000;
    void serviceSensor();
    void finishCalibration();
};
