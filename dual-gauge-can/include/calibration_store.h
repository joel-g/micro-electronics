#pragma once
#include "compass_math.h"

class CalibrationStore {
public:
    bool begin();
    bool load(compass::Vector &offset);
    bool save(compass::Vector offset);
    bool clear();
private:
    bool available = false;
};
