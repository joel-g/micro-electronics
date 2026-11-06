#include "horizon_geometry.h"
#include <cassert>
#include <cstdio>

int main() {
    unsigned long checks = 0;
    // Include level, inverted, vertical boundaries, and off-screen horizons.
    for (int roll = -180; roll <= 180; roll += 3) {
        float angle = roll * 3.14159265358979323846f / 180.0f;
        float c = std::cos(angle), s = std::sin(angle);
        for (int pitch = -180; pitch <= 180; pitch += 9) {
            for (int y = 0; y < 240; ++y) {
                auto span = groundSpan(y, 240, 120, 120, c, s, pitch * 3.0f);
                assert(span.start >= 0 && span.count >= 0);
                assert(span.start + span.count <= 240);
                for (int x = 0; x < 240; ++x) {
                    // Original per-pixel classification is the oracle. Permit
                    // roundoff only immediately on the mathematical boundary.
                    float d = (y - 120) * c - (x - 120) * s - pitch * 3.0f;
                    bool ground = x >= span.start && x < span.start + span.count;
                    assert(std::fabs(d) < 0.0001f || ground == (d >= 0));
                    ++checks;
                }
            }
        }
    }
    std::printf("PASS: %lu pixel classifications across pitch/roll extremes\n", checks);
}
