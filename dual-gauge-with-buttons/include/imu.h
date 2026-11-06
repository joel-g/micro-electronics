#pragma once
#include <Arduino.h>

class IMU {
public:
    void begin();
    void service();
    bool fresh() const;
    bool zero();
    float relativePitch() const { return pitch-pitchZero; }
    float relativeRoll() const;
    struct Diagnostics {
        uint32_t samples=0, errors=0, recoveries=0, filterResyncs=0;
        uint32_t sensorMaxUs=0, sampleGapMaxUs=0;
        uint8_t i2cError=0;
    } stats;
private:
    enum class State { Offline, Waking, Ready } state=State::Offline;
    float pitch=0, roll=0, pitchZero=0, rollZero=0;
    uint32_t lastUpdate=0, lastSampleMs=0, lastAttemptUs=0, lastRecoveryMs=0, wakeAt=0;
    uint32_t consecutiveErrors=0;
    bool initialized=false;
    bool writeMPU(uint8_t reg, uint8_t value);
    bool wakeMPU();
    bool readMPU(float &ax,float &ay,float &az,float &gx,float &gy,float &gz);
    void updateOrientation();
};
