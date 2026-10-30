#include "compass_display.h"
#include "compass_math.h"
#include <SPI.h>
#include <cstring>

namespace {
enum Color : uint8_t { Background, White, Muted, Gold, North, Cyan, Red, Green };
constexpr uint16_t palette[] = {0x0841, 0xFFFF, 0x738E, 0xFEA0, 0xF985, 0x5E7F, 0xF800, 0x07E0};
void recordMax(uint32_t &a, uint32_t b) { if (b > a) a = b; }
}

CompassDisplay::CompassDisplay() : tft(hardware::cs, hardware::dc, hardware::rst) {}
void CompassDisplay::begin() {
    SPI.begin(hardware::sclk, -1, hardware::mosi, hardware::cs);
    tft.begin(hardware::spiHz);
    tft.setRotation(0);
    tft.fillScreen(palette[Background]);
    force = true;
}
void CompassDisplay::centered(const char *text, int y, uint8_t size, uint8_t color) {
    canvas.setTextSize(size);
    canvas.setTextColor(color);
    canvas.setCursor(120 - int(strlen(text)) * 3 * size, y);
    canvas.print(text);
}
void CompassDisplay::prepare(DisplayState state) {
    if (sending) return;
    // Quantize only the graphics position, not the filter state.
    state.heading = compass::wrap(std::round(state.heading * 4.0f) / 4.0f);
    if (hasState && state.heading == lastState.heading && state.status == lastState.status &&
        state.seconds == lastState.seconds && state.saved == lastState.saved) return;
    uint32_t started = micros();
    lastState = state; hasState = true;
    canvas.fillScreen(Background);
    canvas.drawCircle(120, 120, 114, Muted);
    bool valid = state.status == CompassStatus::Live || state.status == CompassStatus::Uncalibrated;
    if (valid) {
        for (int bearing = 0; bearing < 360; bearing += 5) {
            float angle = (bearing - state.heading) * compass::pi / 180.0f;
            float s = std::sin(angle), c = std::cos(angle);
            int inner = bearing % 30 == 0 ? 96 : (bearing % 10 == 0 ? 101 : 105);
            canvas.drawLine(120 + std::lround(inner * s), 120 - std::lround(inner * c),
                            120 + std::lround(110 * s), 120 - std::lround(110 * c),
                            bearing == 0 ? North : White);
        }
        const char *labels[] = {"N", "E", "S", "W"};
        for (int i = 0; i < 4; ++i) {
            float angle = (i * 90.0f - state.heading) * compass::pi / 180.0f;
            canvas.setTextColor(i == 0 ? North : White);
            canvas.setTextSize(2);
            canvas.setCursor(114 + std::lround(82 * std::sin(angle)),
                             112 - std::lround(82 * std::cos(angle)));
            canvas.print(labels[i]);
        }
        canvas.fillCircle(120, 120, 59, Background);
        centered("MAGNETIC", 83, 1, Muted);
        char heading[4]; snprintf(heading, sizeof(heading), "%03d", compass::headingDegrees(state.heading));
        centered(heading, 103, 5, White);
        canvas.drawCircle(171, 104, 3, White);
        const char *directions[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        centered(directions[static_cast<int>((state.heading + 22.5f) / 45.0f) % 8], 146, 2, Gold);
        centered(state.status == CompassStatus::Uncalibrated ? "CAL REQUIRED" :
                 (state.saved ? "CAL SAVED" : "CAL RAM"), 178, 1,
                 state.status == CompassStatus::Uncalibrated ? Gold : Muted);
    } else if (state.status == CompassStatus::Calibrating) {
        centered("CALIBRATING", 73, 1, Gold);
        char seconds[8]; snprintf(seconds, sizeof(seconds), "%02ds", state.seconds);
        centered(seconds, 99, 4, White);
        centered("ROTATE ALL AXES", 147, 1, White);
        centered("AWAY FROM METAL", 163, 1, Muted);
    } else {
        centered("---", 102, 4, Muted);
        const char *message = state.status == CompassStatus::Fault ? "MAG FAULT" :
                              state.status == CompassStatus::CalibrationFailed ? "CAL INCOMPLETE" :
                              state.status == CompassStatus::WeakField ? "WEAK FIELD" : "DETECTING MAG";
        centered(message, 150, 1, state.status == CompassStatus::Fault ? Red : Gold);
    }
    // Fixed lubber mark: the bearing passing under this point is the heading.
    canvas.fillTriangle(113, 4, 127, 4, 120, 18, Gold);
    nextTile = 0; frameTiles = 0; sending = true;
    recordMax(maxDrawUs, micros() - started);
}
void CompassDisplay::flushBatch() {
    if (!sending) return;
    uint32_t started = micros();
    unsigned transmitted = 0;
    auto *pixels = canvas.getBuffer();
    while (nextTile < 450 && transmitted < 4) {
        int x = (nextTile % 15) * 16, y = (nextTile / 15) * 8;
        ++nextTile;
        bool changed = force;
        for (int row = 0; row < 8 && !changed; ++row) {
            int index = (y + row) * 240 + x;
            changed = memcmp(pixels + index, previous + index, 16) != 0;
        }
        if (!changed) continue;
        for (int row = 0; row < 8; ++row) {
            int index = (y + row) * 240 + x;
            for (int column = 0; column < 16; ++column)
                tilePixels[row * 16 + column] = palette[pixels[index + column]];
        }
        tft.startWrite();
        tft.setAddrWindow(x, y, 16, 8);
        tft.writePixels(tilePixels, 16 * 8, true);
        tft.endWrite();
        for (int row = 0; row < 8; ++row) {
            int index = (y + row) * 240 + x;
            memcpy(previous + index, pixels + index, 16);
        }
        ++tiles; ++frameTiles; ++transmitted;
    }
    if (nextTile == 450) {
        if (frameTiles) ++frames;
        sending = false; force = false;
    }
    recordMax(maxTransferUs, micros() - started);
}
