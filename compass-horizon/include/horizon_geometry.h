#pragma once
#include <cmath>

struct GroundSpan {
    int start;
    int count;
};

// Solve (y-cy)*cos(angle) - (x-cx)*sin(angle) - offset >= 0.
// Endpoint classification avoids division near zero and keeps casts in range.
inline GroundSpan groundSpan(int y, int width, int cx, int cy,
                             float cosine, float sine, float offset) {
    const float left = (y - cy) * cosine + cx * sine - offset;
    const float right = (y - cy) * cosine - (width - 1 - cx) * sine - offset;
    if (left >= 0 && right >= 0) return {0, width};
    if (left < 0 && right < 0) return {0, 0};
    const float crossing = left / sine;
    if (sine > 0) {
        int count = static_cast<int>(std::floor(crossing)) + 1;
        if (count < 0) count = 0;
        if (count > width) count = width;
        return {0, count};
    }
    int start = static_cast<int>(std::ceil(crossing));
    if (start < 0) start = 0;
    if (start > width) start = width;
    return {start, width - start};
}
