#include "imu.h"
#include "diagnostics.h"
#include <Wire.h>
#include <cmath>
namespace {
constexpr uint8_t MPU_ADDR=0x68;
constexpr float DEG=180.0f/3.14159265358979323846f;
void recordMax(uint32_t &a,uint32_t b) { if(b>a) a=b; }
float angleDifference(float target,float current) { return remainderf(target-current,360.0f); }
}
void IMU::begin() { lastRecoveryMs=millis()-1000; }
bool IMU::fresh() const { return initialized && state==State::Ready && millis()-lastSampleMs<=250; }
float IMU::relativeRoll() const { return angleDifference(roll,rollZero); }
bool IMU::zero() {
    if(!fresh()) return false;
    pitchZero=pitch; rollZero=roll; return true;
}
bool IMU::writeMPU(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    stats.i2cError = Wire.endTransmission();
    return stats.i2cError == 0;
}

bool IMU::wakeMPU() {
    // Verify identity before changing registers.
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x75);
    stats.i2cError = Wire.endTransmission(false);
    if (stats.i2cError != 0) return false;
    if (Wire.requestFrom(MPU_ADDR, (uint8_t)1) != 1) {
        stats.i2cError = 0xFE; // short read / bus failure
        return false;
    }
    if (Wire.read() != 0x68) {
        stats.i2cError = 0xFD; // unexpected WHO_AM_I
        return false;
    }
    // Wake sensor
    if (!writeMPU(0x6B, 0x00)) return false;

    return true;
}

bool IMU::readMPU(
    float &ax, float &ay, float &az,
    float &gx, float &gy, float &gz
) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);

    stats.i2cError = Wire.endTransmission(false);
    if (stats.i2cError != 0) {
        return false;
    }

    if (Wire.requestFrom(MPU_ADDR, (uint8_t)14) != 14) {
        stats.i2cError = 0xFE;
        while (Wire.available()) Wire.read();
        return false;
    }

    auto read16 = []() -> int16_t {
        uint16_t high = Wire.read();
        uint16_t low = Wire.read();
        return (int16_t)((high << 8) | low);
    };

    int16_t rawAX = read16();
    int16_t rawAY = read16();
    int16_t rawAZ = read16();

    read16(); // Temperature (unused)

    int16_t rawGX = read16();
    int16_t rawGY = read16();
    int16_t rawGZ = read16();

    // +/- 2g accelerometer
    ax = rawAX / 16384.0f;
    ay = rawAY / 16384.0f;
    az = rawAZ / 16384.0f;

    // +/- 250 degrees/second gyro
    gx = rawGX / 131.0f;
    gy = rawGY / 131.0f;
    gz = rawGZ / 131.0f;

    return true;
}

void IMU::updateOrientation() {
    float ax, ay, az;
    float gx, gy, gz;

    if (!readMPU(ax, ay, az, gx, gy, gz)) {
        ++stats.errors;
        if (++consecutiveErrors >= 5) state = State::Offline;
        return;
    }

    uint32_t now = micros();
    if (initialized) recordMax(stats.sampleGapMaxUs, now - lastUpdate);
    float dt = (now - lastUpdate) / 1000000.0f;
    lastUpdate = now;
    lastSampleMs = millis();
    consecutiveErrors = 0;
    ++stats.samples;

    // Accelerometer-derived orientation
    float accelPitch =
        atan2f(-ax, sqrtf(ay * ay + az * az)) * DEG;

    float accelRoll =
        atan2f(ay, az) * DEG;

    if (!initialized) {
        pitch = accelPitch;
        roll = accelRoll;

        pitchZero = accelPitch;
        rollZero = accelRoll;

        initialized = true;
        return;
    }

    // Prevent huge integration jumps after delays
    if (dt <= 0.0f || dt > 0.2f) {
        // Reacquire after an outage, preserving the user's original zero.
        pitch = accelPitch;
        roll = accelRoll;
        ++stats.filterResyncs;
        return;
    }

    // Complementary filter:
    // Gyro handles fast movement.
    // Accelerometer corrects long-term drift.
    // Same 0.98 weight at 100 Hz; consistent correction at irregular intervals.
    const float alpha = expf(-dt / 0.495f);

    float predictedPitch = pitch + gy * dt;
    float predictedRoll = roll + gx * dt;

    pitch = predictedPitch +
        (1.0f - alpha) *
        angleDifference(accelPitch, predictedPitch);

    roll = predictedRoll +
        (1.0f - alpha) *
        angleDifference(accelRoll, predictedRoll);

    // Normalize roll angle
    roll = remainderf(roll, 360.0f);
}


void IMU::service() {
    uint32_t now=millis(), started=micros();
    if(state==State::Offline) {
        if(now-lastRecoveryMs<1000) return;
        lastRecoveryMs=now; ++stats.recoveries; consecutiveErrors=0;
        bool awake=wakeMPU();
        if(awake) { state=State::Waking; wakeAt=now; }
        else logLine("MPU detected=0 retry=%lu error=%u\n",(unsigned long)stats.recoveries,stats.i2cError);
    } else if(state==State::Waking) {
        if(now-wakeAt<100) return;
        bool ready=writeMPU(0x1B,0x00) && writeMPU(0x1C,0x00) && writeMPU(0x1A,0x03);
        state=ready ? State::Ready : State::Offline;
        logLine("MPU detected=%d address=0x68 retry=%lu error=%u\n",ready,(unsigned long)stats.recoveries,stats.i2cError);
    } else {
        if(started-lastAttemptUs<10000) return;
        lastAttemptUs=started;
        updateOrientation();
    }
    recordMax(stats.sensorMaxUs,micros()-started);
}
