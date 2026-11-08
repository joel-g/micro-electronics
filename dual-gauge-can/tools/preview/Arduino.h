#pragma once
// Host-only adapter used with the actual pinned Adafruit GFX rasterizer.
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#define PROGMEM
class __FlashStringHelper;
class String:public std::string {public:using std::string::string;};
inline float radians(float degrees) {return degrees*3.14159265358979323846f/180.0f;}
