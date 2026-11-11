#include "display.h"
#include <SPI.h>
namespace {
constexpr uint16_t palette[]={0x0841,0xFFFF,0x738E,0xFEA0,0xF985,0x5E7F,0xF800,0x07E0,
                              0x249F,0x8A22,0x0000,0xFFE0,0x07FF,0x8A60};
}
Display::Display() : IndexedFramebuffer(palette), tft(hardware::cs,hardware::dc,hardware::rst) {}
void Display::begin() {
    SPI.begin(hardware::sclk,-1,hardware::mosi,hardware::cs);
    tft.begin(hardware::spiHz);
    tft.setRotation(0);
    tft.fillScreen(palette[colors::Background]);
    invalidate();
}
void Display::flushBatch() {
    IndexedFramebuffer::flushBatch(8, tft);
}
