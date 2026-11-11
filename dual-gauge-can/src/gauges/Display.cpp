#include "display.h"
#include <SPI.h>
namespace {
constexpr auto &palette=colors::rgb565;
}
// Never give either driver the shared reset pin: starting TFT 2 must not
// hardware-reset TFT 1. Each begin() issues a CS-scoped software reset instead.
Display::Display(int cs) : IndexedFramebuffer(palette), tft(cs,hardware::dc,-1) {}
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
    invalidate();
}
void Display::flushBatch(unsigned budget) {
    IndexedFramebuffer::flushBatch(budget, tft);
}
