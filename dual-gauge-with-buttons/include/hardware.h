#pragma once
#include <Arduino.h>

namespace hardware {
constexpr int sda = 1, scl = 2;
constexpr int sclk = 8, mosi = 9, horizonCs = 10, compassCs = 7, dc = 11, rst = 12;
constexpr int attitudeButton=13, compassButton=4;
// GPIO5/6 belong to the fitted CAN transceiver and are deliberately unused.
constexpr int width = 240, height = 240;
constexpr uint32_t i2cHz = 400000;
constexpr uint16_t i2cTimeoutMs = 20;
#ifndef TFT_SPI_HZ
#define TFT_SPI_HZ 20000000
#endif
constexpr uint32_t spiHz = TFT_SPI_HZ;
}
