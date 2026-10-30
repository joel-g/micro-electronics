#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GC9A01A.h>
#include "hardware.h"
#include "diagnostics.h"
#include "qmc5883p.h"

Adafruit_GC9A01A display(hardware::cs, hardware::dc, hardware::rst);
Qmc5883p sensor;
MagneticSample sample;
uint8_t scanAddress = 1;
bool found[128] = {}, ready = false;
uint32_t samples = 0, errors = 0, overflows = 0;

void setup() {
    beginDiagnostics();
    Wire.begin(hardware::sda, hardware::scl, hardware::i2cHz);
    Wire.setTimeOut(hardware::i2cTimeoutMs);
    SPI.begin(hardware::sclk, -1, hardware::mosi, hardware::cs);
    display.begin(hardware::spiHz);
    display.setRotation(0);
    display.fillScreen(0);
    display.setTextColor(0xFFFF);
    display.setTextSize(2);
    display.setCursor(48, 65); display.print("I2C SCAN");
}

void loop() {
    if (scanAddress < 127) {
        Wire.beginTransmission(scanAddress);
        uint8_t error = Wire.endTransmission();
        if (!error) {
            found[scanAddress] = true;
            logLine("SCAN address=0x%02X ACK\n", scanAddress);
        }
        ++scanAddress;
        if (scanAddress == 127) {
            ready = found[Qmc5883p::address] && sensor.begin();
            display.setTextSize(1);
            display.setCursor(42, 100);
            display.print(ready ? "QMC5883P FOUND 0x2C" : "QMC5883P NOT FOUND");
        }
    } else {
        static uint32_t attempt = 0;
        if (millis() - attempt >= 20) {
            attempt = millis();
            if (ready) {
                auto result = sensor.read(sample);
                if (result == Qmc5883p::Result::Sample) ++samples;
                if (result == Qmc5883p::Result::BusError) ++errors;
                if (result == Qmc5883p::Result::Overflow) ++overflows;
            }
        }
    }
    static uint32_t lastLog = 0;
    if (millis() - lastLog >= 1000) {
        lastLog = millis();
        logLine("PROBE up=%lu found2c=%d found68=%d found0d=%d found1e=%d "
                "ready=%d id=0x%02X ctrl1=0x%02X ctrl2=0x%02X "
                "sample=%lu err=%lu overflow=%lu bus=%u xyz=%d,%d,%d\n",
                (unsigned long)millis(), found[0x2c], found[0x68], found[0x0d],
                found[0x1e], ready, sensor.chipId, sensor.control1, sensor.control2,
                (unsigned long)samples, (unsigned long)errors,
                (unsigned long)overflows, sensor.busError, sample.x, sample.y, sample.z);
        if (scanAddress == 127) {
            display.fillRect(36, 125, 168, 45, 0);
            display.setCursor(42, 130); display.printf("X %d Y %d", sample.x, sample.y);
            display.setCursor(42, 145); display.printf("Z %d   samples %lu", sample.z, (unsigned long)samples);
        }
    }
    delay(1);
}
