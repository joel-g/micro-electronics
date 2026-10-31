#pragma once
#include <cmath>
#include <cstdint>
#include <limits>

namespace compass {
constexpr float pi = 3.14159265358979323846f;
struct Vector { float x = 0, y = 0, z = 0; };
inline float wrap(float angle) {
    float value = std::fmod(angle, 360.0f);
    return value < 0 ? value + 360.0f : value;
}
inline float difference(float target, float current) {
    return std::remainder(target - current, 360.0f);
}
inline float smooth(float current, float target, float seconds, float tau = 0.18f) {
    if (seconds <= 0) return current;
    return wrap(current + (1.0f - std::exp(-seconds / tau)) * difference(target, current));
}
inline int headingDegrees(float angle) { return static_cast<int>(std::floor(wrap(angle) + 0.5f)) % 360; }

// Sensor +X is forward, +Y is right. Both directions must lie in the level
// plane. Heading is clockwise from magnetic north; no startup zero/declination.
inline bool heading(Vector measured, Vector offset, float &result) {
    float forward = measured.x - offset.x;
    float right = measured.y - offset.y;
    // +/-8 G conversion: 37.5 counts/uT. Reject <2 uT horizontal magnitude.
    if (!std::isfinite(forward) || !std::isfinite(right) ||
        forward * forward + right * right < 75.0f * 75.0f) return false;
    result = wrap(std::atan2(-right, forward) * 180.0f / pi);
    return true;
}

class HardIronCalibration {
public:
    void start() {
        count = 0;
        low = {32767, 32767, 32767};
        high = {-32768, -32768, -32768};
    }
    void add(Vector value) {
        if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z)) return;
        low.x = std::fmin(low.x, value.x); high.x = std::fmax(high.x, value.x);
        low.y = std::fmin(low.y, value.y); high.y = std::fmax(high.y, value.y);
        low.z = std::fmin(low.z, value.z); high.z = std::fmax(high.z, value.z);
        ++count;
    }
    Vector span() const {
        return count ? Vector{high.x - low.x, high.y - low.y, high.z - low.z} : Vector{};
    }
    bool finish(uint32_t elapsedMs, Vector &offset) const {
        Vector extent = span();
        if (elapsedMs < 10000 || count < 200 ||
            extent.x < 400 || extent.y < 400 || extent.z < 400) return false;
        offset = {(high.x + low.x) * 0.5f, (high.y + low.y) * 0.5f,
                  (high.z + low.z) * 0.5f};
        return true;
    }
    uint32_t count = 0;
private:
    Vector low{}, high{};
};
}
