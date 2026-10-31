#include "display.h"
#include <SPI.h>
#include <cstring>
namespace {
constexpr uint16_t palette[]={0x0841,0xFFFF,0x738E,0xFEA0,0xF985,0x5E7F,0xF800,0x07E0,
                              0x249F,0x8A22,0x0000,0xFFE0,0x07FF,0x8A60};
void recordMax(uint32_t &a,uint32_t b) { if(b>a) a=b; }
}
Display::Display() : tft(hardware::cs,hardware::dc,hardware::rst) {}
void Display::begin() {
    SPI.begin(hardware::sclk,-1,hardware::mosi,hardware::cs);
    tft.begin(hardware::spiHz);
    tft.setRotation(0);
    tft.fillScreen(palette[colors::Background]);
}
void Display::present() {
    nextTile=0; frameTiles=0; sending=true; started=micros();
}
void Display::flushBatch() {
    if (!sending) return;
    uint32_t started = micros();
    unsigned transmitted = 0;
    auto *pixels = buffer.getBuffer();
    while (nextTile < 450 && transmitted < 8) {
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
        recordMax(maxFrameUs, micros()-this->started);
        sending = false; force = false;
    }
    recordMax(maxTransferUs, micros() - started);
}
