#include "qmc5883p.h"

// QST QMC5883P datasheet, sections 7 and 9. Deliberately not a QMC5883L driver.
bool Qmc5883p::readRegisters(uint8_t reg, uint8_t *data, uint8_t size) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    busError = Wire.endTransmission(false);
    if (busError) return false;
    if (Wire.requestFrom(address, size) != size) {
        busError = 0xFE;
        while (Wire.available()) Wire.read();
        return false;
    }
    for (uint8_t i = 0; i < size; ++i) data[i] = Wire.read();
    return true;
}

bool Qmc5883p::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    busError = Wire.endTransmission();
    return busError == 0;
}

bool Qmc5883p::begin() {
    chipId = 0;
    if (!readRegisters(0x00, &chipId, 1)) return false;
    if (chipId != expectedId) { busError = 0xFD; return false; }
    // Suspend before changing modes; establish datasheet axis signs, +/-8 G,
    // automatic set/reset, OSR1=8, OSR2=8, normal measurement at 50 Hz.
    if (!writeRegister(0x0A, 0x00) || !writeRegister(0x29, 0x06) ||
        !writeRegister(0x0B, 0x08) || !writeRegister(0x0A, 0xC5)) return false;
    if (!readRegisters(0x0A, &control1, 1) ||
        !readRegisters(0x0B, &control2, 1)) return false;
    if (control1 != 0xC5 || control2 != 0x08) { busError = 0xFC; return false; }
    return true;
}

Qmc5883p::Result Qmc5883p::read(MagneticSample &sample) {
    // Read status once: overflow is clear-on-read. Poll DRDY; no wired DRDY pin.
    if (!readRegisters(0x09, &status, 1)) return Result::BusError;
    if (!(status & 0x03)) return Result::NotReady;
    uint8_t bytes[6];
    if (!readRegisters(0x01, bytes, sizeof(bytes))) return Result::BusError;
    if (status & 0x02) return Result::Overflow;
    auto decode = [&](int index) -> int16_t {
        return static_cast<int16_t>(uint16_t(bytes[index]) |
                                    (uint16_t(bytes[index + 1]) << 8));
    };
    sample = {decode(0), decode(2), decode(4)};
    return Result::Sample;
}
