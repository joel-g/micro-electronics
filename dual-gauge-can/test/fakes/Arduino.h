#pragma once
#include <cstdint>
#include <cassert>
#include <array>
constexpr int HIGH=1, LOW=0, OUTPUT=1;
namespace fake {
inline std::array<int,40> pins{};
inline uint32_t timeUs=0;
inline int resetPulses=0, selected=-1, busStarts=0;
}
inline void pinMode(int,int) {}
inline void digitalWrite(int pin,int value) {
    if(pin==12 && value==LOW) {
        assert(fake::pins[7]==HIGH && fake::pins[10]==HIGH);
        ++fake::resetPulses;
    }
    fake::pins.at(pin)=value;
}
inline void delay(uint32_t ms) { fake::timeUs+=ms*1000; }
inline uint32_t micros() { return ++fake::timeUs; }
inline uint32_t millis() { return fake::timeUs/1000; }
