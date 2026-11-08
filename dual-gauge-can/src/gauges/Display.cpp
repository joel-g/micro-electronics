#include "display.h"
#include <SPI.h>
#include <cstring>
namespace {
constexpr auto &palette=colors::rgb565;
void recordMax(uint32_t &a,uint32_t b) { if(b>a) a=b; }
}
// Never give either driver the shared reset pin: starting TFT 2 must not
// hardware-reset TFT 1. Each begin() issues a CS-scoped software reset instead.
Display::Display(int cs) : tft(cs,hardware::dc,-1) {}
void Display::beginBus() {
    digitalWrite(hardware::horizonCs,HIGH);
    digitalWrite(hardware::compassCs,HIGH);
    pinMode(hardware::horizonCs,OUTPUT);
    pinMode(hardware::compassCs,OUTPUT);
    digitalWrite(hardware::horizonCs,HIGH);
    digitalWrite(hardware::compassCs,HIGH);
    SPI.begin(hardware::sclk,-1,hardware::mosi,-1);
    // Startup only; no reset or initialization delays in the service loop.
    pinMode(hardware::rst,OUTPUT);
    digitalWrite(hardware::rst,HIGH); delay(100);
    digitalWrite(hardware::rst,LOW); delay(100);
    digitalWrite(hardware::rst,HIGH); delay(200);
}
void Display::begin() {
    tft.begin(hardware::spiHz);
    tft.setRotation(0);
    tft.fillScreen(palette[colors::Background]);
}
void Display::present() {
    nextTile=0; frameTiles=0; sending=true; started=micros();
}
void Display::flushBatch(unsigned budget) {
    if (!sending) return;
    uint32_t started = micros();
    unsigned transmitted = 0;
    auto *pixels = buffer.getBuffer();
    while (nextTile < 450 && transmitted < budget) {
        int x = (nextTile % 15) * 16, y = (nextTile / 15) * 8;
        ++nextTile;
        bool changed = force;
        for (int row = 0; row < 8 && !changed; ++row) {
            int index = (y + row) * 240 + x;
            changed = memcmp(pixels + index, previous + index, 16) != 0;
        }
        if (!changed) continue;
        for (int row = 0; row < 8; ++row) {
            int index = (y + row) * 240 + x;
            for (int column = 0; column < 16; ++column)
                tilePixels[row * 16 + column] = palette[pixels[index + column]];
        }
        tft.startWrite();
        tft.setAddrWindow(x, y, 16, 8);
        tft.writePixels(tilePixels, 16 * 8, true);
        tft.endWrite();
        for (int row = 0; row < 8; ++row) {
            int index = (y + row) * 240 + x;
            memcpy(previous + index, pixels + index, 16);
        }
        ++tiles; ++frameTiles; ++transmitted;
    }
    if (nextTile == 450) {
        if (frameTiles) ++frames;
        ++completed;
        recordMax(maxFrameUs, micros()-this->started);
        sending = false; force = false;
    }
    recordMax(maxTransferUs, micros() - started);
}
