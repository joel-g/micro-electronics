#pragma once
#include "Arduino.h"
class GFXcanvas8 {
public:
    GFXcanvas8(int,int,bool) {}
    uint8_t *getBuffer() { return buffer; }
protected:
    uint8_t *buffer=nullptr;
};
