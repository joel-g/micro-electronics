#include "compass_gauge.h"
#include "compass_math.h"
#include "display.h"
#include <cstring>
using namespace colors;

void CompassGauge::centered(const char *text, int y, uint8_t size, uint8_t color) {
    canvas.setTextSize(size);
    canvas.setTextColor(color);
    canvas.setCursor(120 - int(strlen(text)) * 3 * size, y);
    canvas.print(text);
}
bool CompassGauge::draw(CompassState state, bool force) {
    // Quantize only the graphics position, not the filter state.
    state.heading = compass::wrap(std::round(state.heading * 4.0f) / 4.0f);
    if (!force && hasState && state.heading == lastState.heading && state.status == lastState.status &&
        state.seconds == lastState.seconds && state.saved == lastState.saved &&
        state.light == lastState.light && state.sensorFresh == lastState.sensorFresh) return false;
    lastState = state; hasState = true;
    const auto background=state.light ? White : Background;
    const auto foreground=state.light ? Black : White;
    const auto accent=state.light ? DarkGold : Gold;
    const auto north=state.light ? Red : North;
    canvas.fillScreen(background);
    canvas.drawCircle(120, 120, 114, Muted);
    bool valid = state.status == CompassStatus::Live || state.status == CompassStatus::Uncalibrated;
    if (valid) {
        for (int bearing = 0; bearing < 360; bearing += 5) {
            float angle = (bearing - state.heading) * compass::pi / 180.0f;
            float s = std::sin(angle), c = std::cos(angle);
            int inner = bearing % 30 == 0 ? 96 : (bearing % 10 == 0 ? 101 : 105);
            canvas.drawLine(120 + std::lround(inner * s), 120 - std::lround(inner * c),
                            120 + std::lround(110 * s), 120 - std::lround(110 * c),
                            bearing == 0 ? north : foreground);
        }
        const char *labels[] = {"N", "E", "S", "W"};
        for (int i = 0; i < 4; ++i) {
            float angle = (i * 90.0f - state.heading) * compass::pi / 180.0f;
            canvas.setTextColor(i == 0 ? north : foreground);
            canvas.setTextSize(2);
            canvas.setCursor(114 + std::lround(82 * std::sin(angle)),
                             112 - std::lround(82 * std::cos(angle)));
            canvas.print(labels[i]);
        }
        canvas.fillCircle(120, 120, 59, background);
        centered("MAGNETIC", 83, 1, Muted);
        char heading[4]; snprintf(heading, sizeof(heading), "%03d", compass::headingDegrees(state.heading));
        centered(heading, 103, 5, foreground);
        canvas.drawCircle(171, 104, 3, foreground);
        const char *directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        centered(directions[static_cast<int>((state.heading + 22.5f) / 45.0f) % 8], 146, 2, accent);
        centered(state.status == CompassStatus::Uncalibrated ? "CAL REQUIRED" :
                 (state.saved ? "CAL SAVED" : "CAL RAM"), 178, 1,
                 state.status == CompassStatus::Uncalibrated ? accent : Muted);
    } else if (state.status == CompassStatus::Calibrating) {
        centered("CALIBRATING", 73, 1, accent);
        char seconds[8]; snprintf(seconds, sizeof(seconds), "%02ds", state.seconds);
        centered(seconds, 99, 4, foreground);
        centered(state.sensorFresh ? "ROTATE ALL AXES" : "NO SENSOR DATA", 147, 1,
                 state.sensorFresh ? foreground : Red);
        centered("TURN / TIP / INVERT", 179, 1, foreground);
        centered("AWAY FROM METAL", 163, 1, Muted);
    } else if (state.status == CompassStatus::CalibrationSaved ||
               state.status == CompassStatus::CalibrationFailed ||
               state.status == CompassStatus::SaveFailed) {
        centered("CALIBRATION", 89, 2, foreground);
        centered(state.status == CompassStatus::CalibrationSaved ? "SAVED" :
                 state.status == CompassStatus::SaveFailed ? "SAVE FAILED" : "FAILED", 118, 2,
                 state.status == CompassStatus::CalibrationSaved ? accent : Red);
        centered(state.status == CompassStatus::CalibrationSaved ? "RETURN SENSOR TO LEVEL" :
                 "PRIOR OFFSETS KEPT", 156, 1, Muted);
    } else {
        centered("---", 102, 4, Muted);
        const char *message = state.status == CompassStatus::Fault ? "MAG FAULT" :
                              state.status == CompassStatus::CalibrationFailed ? "CAL INCOMPLETE" :
                              state.status == CompassStatus::WeakField ? "WEAK FIELD" : "DETECTING MAG";
        centered(message, 150, 1, state.status == CompassStatus::Fault ? Red : accent);
    }
    // Fixed lubber mark: the bearing passing under this point is the heading.
    canvas.fillTriangle(113, 4, 127, 4, 120, 18, accent);
    return true;
}
