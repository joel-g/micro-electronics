#include "compass_math.h"
#include <cassert>
#include <cstdio>

bool close(float a, float b, float tolerance = 0.01f) {
    return std::fabs(compass::difference(a, b)) < tolerance;
}
int main() {
    using namespace compass;
    assert(wrap(-1) == 359 && wrap(360) == 0);
    assert(headingDegrees(359.6f) == 0 && headingDegrees(358.6f) == 359);
    assert(difference(1, 359) == 2 && difference(359, 1) == -2);
    float forward = smooth(359, 1, 0.05f);
    float backward = smooth(1, 359, 0.05f);
    assert(forward > 359 && backward < 1 && backward > 0);
    for (int i = 0; i < 40; ++i) forward = smooth(forward, 1, 0.05f);
    assert(close(forward, 1));
    float h;
    assert(heading({1000, 0, 0}, {}, h) && close(h, 0));
    assert(heading({0, -1000, 0}, {}, h) && close(h, 90));
    assert(heading({-1000, 0, 0}, {}, h) && close(h, 180));
    assert(heading({0, 1000, 0}, {}, h) && close(h, 270));
    assert(!heading({0, 0, 1000}, {}, h));
    HardIronCalibration calibration;
    Vector offset{1, 2, 3};
    calibration.start();
    for (int i = 0; i < 250; ++i) calibration.add({100, 100, 100});
    assert(!calibration.finish(60000, offset));
    assert(offset.x == 1 && offset.y == 2 && offset.z == 3);
    calibration.start();
    Vector bias{350, -700, 900};
    for (int i = 0; i < 360; ++i) {
        float a = i * pi / 180.0f;
        calibration.add({bias.x + 1500 * std::cos(a), bias.y + 1500 * std::sin(a),
                         bias.z + 1500 * std::sin(2 * a)});
    }
    assert(!calibration.finish(9000, offset));
    assert(calibration.finish(60000, offset));
    assert(std::fabs(offset.x - bias.x) < 0.1f && std::fabs(offset.y - bias.y) < 0.1f &&
           std::fabs(offset.z - bias.z) < 0.1f);
    for (int angle = 0; angle < 360; ++angle) {
        float a = angle * pi / 180.0f;
        assert(heading({bias.x + 1000 * std::cos(a), bias.y - 1000 * std::sin(a), bias.z}, offset, h));
        assert(close(h, angle));
    }
    puts("PASS: magnetic headings, wraparound, smoothing, hard-iron recovery, insufficient-calibration rejection");
}
