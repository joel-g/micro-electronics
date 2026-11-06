#include "magnetometer.h"
#include "diagnostics.h"
namespace {
void maxTime(uint32_t &a,uint32_t b) { if(b>a) a=b; }
}
void Magnetometer::begin() {
    bool available=storage.begin();
    saved=calibrated=storage.load(offset);
    lastBegin=millis()-1000;
    lastAnimation=micros();
    logLine("CAL storage=%d loaded=%d offset=%.1f,%.1f,%.1f\n",available,saved,offset.x,offset.y,offset.z);
}
void Magnetometer::service() {
    serviceSensor();
    if(collecting && !fresh()) calibrationInvalid=true;
    if(collecting && millis()-progressAt>=5000) {
        progressAt=millis();
        auto extent=calibration.span();
        uint32_t elapsed=millis()-calibrationStarted;
        logLine("CAL progress remaining=%lu samples=%lu span=%.0f,%.0f,%.0f invalid=%d\n",
                (unsigned long)(elapsed>=calibrationDuration ? 0 : (calibrationDuration-elapsed+999)/1000),
                (unsigned long)calibration.count,extent.x,extent.y,extent.z,calibrationInvalid);
    }
    if(collecting && millis()-calibrationStarted>=calibrationDuration) finishCalibration();
    uint32_t now=micros();
    float dt=(now-lastAnimation)*0.000001f;
    lastAnimation=now;
    if(fresh() && fieldValid && !collecting) heading=compass::smooth(heading,targetHeading,dt);
}
void Magnetometer::serviceSensor() {
    uint32_t now = millis();
    if (!sensorReady) {
        if (now - lastBegin < 1000) return;
        lastBegin = now;

        ++stats.retries;
        sensorReady = magnetometer.begin();
        receivedSinceBegin = false;
        sensorStarted = now;
        stats.streak = 0;
        headingReady = false;
        logLine("MAG detected=%d address=0x2C id=0x%02X ctrl1=0x%02X ctrl2=0x%02X error=%u\n",
                sensorReady, magnetometer.chipId, magnetometer.control1,
                magnetometer.control2, magnetometer.busError);

        return;
    }
    if (now - lastRead < 10) return;
    lastRead = now;

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
        if(collecting) calibrationInvalid=true;
        ++stats.errors;
        if (++stats.streak >= 5) sensorReady = false;
    } else if (result == Qmc5883p::Result::Overflow) {
        if(collecting) calibrationInvalid=true;
        ++stats.overflows;
        fieldValid = false;
    }
    // An ACK without new DRDY samples is not a healthy sensor.
    uint32_t newest = receivedSinceBegin ? lastSample : sensorStarted;
    if (now - newest > 1000) sensorReady = false;

}

bool Magnetometer::startCalibration(bool saveAutomatically) {
    if(collecting) { logLine("CAL start ignored: already collecting\n"); return false; }
    if(!fresh()) {
        feedback=CompassStatus::CalibrationFailed; feedbackAt=millis();
        logLine("CAL FAILED reason=sensor-unavailable; prior offsets kept\n");
        return false;
    }
    calibration.start(); collecting=true; autoSave=saveAutomatically;
    calibrationInvalid=false; feedback=CompassStatus::Waiting;
    progressAt=calibrationStarted=millis();
    logLine("CAL START seconds=60 autosave=%d; turn, tip, invert sensor through all axes\n",autoSave);
    return true;
}

void Magnetometer::finishCalibration() {
    if (!collecting) return;
    compass::Vector candidate;
    bool valid = !calibrationInvalid && fresh() && calibration.finish(millis() - calibrationStarted, candidate);
    collecting = false;
    if (valid) {
        // Save and read back before applying the candidate in button workflow.
        // Invalid collections never touch NVS or replace the current offsets.
        if(autoSave && !storage.save(candidate)) {
            feedback=CompassStatus::SaveFailed; feedbackAt=millis();
            logLine("CAL SAVE FAILED; prior active offsets kept\n");
            return;
        }
        offset = candidate;
        calibrated = true; saved = autoSave; headingReady = false;
        fieldValid=compass::heading({float(raw.x),float(raw.y),float(raw.z)},offset,targetHeading);
        if(fieldValid) { heading=targetHeading; headingReady=true; }
        feedback=autoSave ? CompassStatus::CalibrationSaved : CompassStatus::Waiting;
        feedbackAt=millis();
        logLine("CAL applied saved=%d offset=%.1f,%.1f,%.1f samples=%lu%s\n",
                saved,offset.x, offset.y, offset.z, (unsigned long)calibration.count,
                saved ? "; CALIBRATION SAVED" : "; RAM only, send s to save");
    } else {
        feedback=CompassStatus::CalibrationFailed; feedbackAt=millis();
        auto extent = calibration.span();
        logLine("CAL FAILED samples=%lu span=%.0f,%.0f,%.0f invalid=%d; prior offsets kept\n",
                (unsigned long)calibration.count, extent.x, extent.y, extent.z,calibrationInvalid);
    }
}

void Magnetometer::command(char value) {
    switch (value) {
        case 'c': startCalibration(); break;
        case 'f':
            if(collecting && autoSave) logLine("CAL finish ignored: button calibration requires full 60 seconds\n");
            else finishCalibration();
            break;
        case 'x': collecting = false; feedback=CompassStatus::Waiting; logLine("CAL cancelled; prior offsets kept\n"); break;
        case 's':
            if (!calibrated || collecting) { logLine("SAVE refused: finish a valid calibration first\n"); break; }
            saved = storage.save(offset);
            logLine("SAVE ok=%d offset=%.1f,%.1f,%.1f\n", saved, offset.x, offset.y, offset.z);
            break;
        case 'l': {
            if (collecting) break;
            compass::Vector loaded;
            bool ok = storage.load(loaded);
            if (ok) { offset = loaded; calibrated = saved = true; headingReady = false; }
            logLine("LOAD ok=%d offset=%.1f,%.1f,%.1f\n", ok, offset.x, offset.y, offset.z);
            break;
        }
        case 'e':

            if (storage.clear()) {
                offset = {}; calibrated = saved = collecting = false; headingReady = false;
                logLine("CAL erased from RAM and NVS\n");
            } else logLine("CAL erase failed; existing calibration kept\n");

            break;
    }
}

CompassState Magnetometer::displayState() const {
    CompassState state;
    state.heading = heading; state.saved = saved; state.sensorFresh=fresh();
    if (collecting) {
        state.status = CompassStatus::Calibrating;
        state.seconds = (calibrationDuration - (millis() - calibrationStarted) + 999) / 1000;
    } else if(feedback!=CompassStatus::Waiting && millis()-feedbackAt<3000) state.status=feedback;
    else if (!fresh()) state.status = CompassStatus::Fault;
    else if (!fieldValid) state.status = CompassStatus::WeakField;
    else state.status = calibrated ? CompassStatus::Live : CompassStatus::Uncalibrated;
    return state;
}
