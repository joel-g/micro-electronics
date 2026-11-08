#pragma once
#include "Arduino.h"
struct SPIClass {
    void begin(int clock,int miso,int mosi,int cs) {
        assert(clock==8 && miso==-1 && mosi==9 && cs==-1);
        assert(fake::pins[7]==HIGH && fake::pins[10]==HIGH);
        ++fake::busStarts;
    }
};
inline SPIClass SPI;
