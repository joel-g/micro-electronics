#pragma once
#include <Wire.h>

struct MagneticSample { int16_t x = 0, y = 0, z = 0; };

class Qmc5883p {
public:
    static constexpr uint8_t address = 0x2C;
    static constexpr uint8_t expectedId = 0x80;
    // +/-8 G: 3750 counts/G, 100 uT/G.
    static constexpr float microteslaPerCount = 100.0f / 3750.0f;
    enum class Result { Sample, NotReady, Overflow, BusError };
    bool begin();
    Result read(MagneticSample &sample);
    uint8_t chipId = 0, busError = 0, status = 0;
    uint8_t control1 = 0, control2 = 0;
private:
    bool readRegisters(uint8_t reg, uint8_t *data, uint8_t size);
    bool writeRegister(uint8_t reg, uint8_t value);
};
