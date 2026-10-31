#include "attitude_gauge.h"
#include "horizon_geometry.h"
#include "display.h"
#include <cmath>
namespace {
constexpr int WIDTH=240, HEIGHT=240, CX=120, CY=120;
constexpr float RAD=3.14159265358979323846f/180.0f;
constexpr uint8_t SKY=colors::Sky, GROUND=colors::Ground, WHITE=colors::White;
constexpr uint8_t BLACK=colors::Black, YELLOW=colors::Yellow, CYAN=colors::HorizonCyan, RED=colors::Red;
}
void AttitudeGauge::drawRotatedLine(
    float centerX,
    float centerY,
    float halfLength,
    float angle,
    uint16_t color
) {
    float c = cosf(angle);
    float s = sinf(angle);

    int x1 = roundf(centerX - halfLength * c);
    int y1 = roundf(centerY - halfLength * s);
    int x2 = roundf(centerX + halfLength * c);
    int y2 = roundf(centerY + halfLength * s);

    canvas.drawLine(x1, y1, x2, y2, color);
}


void AttitudeGauge::drawHorizon(float p, float r) {
    float angle = r * RAD;
    float c = cosf(angle);
    float s = sinf(angle);

    // Pixels per degree of pitch
    float pitchOffset = p * 3.0f;

    // Sky background
    canvas.fillScreen(SKY);

    // Ground boundary, rotated around the screen center
    for (int y = 0; y < HEIGHT; y++) {
        GroundSpan span = groundSpan(y, WIDTH, CX, CY, c, s, pitchOffset);
        if (span.count) canvas.drawFastHLine(span.start, y, span.count, GROUND);
    }

    // Main horizon boundary
    float horizonX = CX - s * pitchOffset;
    float horizonY = CY + c * pitchOffset;

    drawRotatedLine(
        horizonX,
        horizonY,
        220,
        angle,
        WHITE
    );

    // Pitch ladder
    for (int degrees = -30; degrees <= 30; degrees += 10) {
        if (degrees == 0) continue;

        float offset = (p - degrees) * 3.0f;

        float x = CX - s * offset;
        float y = CY + c * offset;

        float length =
            (degrees % 20 == 0) ? 27.0f : 16.0f;

        drawRotatedLine(
            x, y, length, angle, WHITE
        );
    }
}

void AttitudeGauge::drawAircraftSymbol() {
    // Fixed wings
    canvas.drawFastHLine(65, CY, 39, BLACK);
    canvas.drawFastHLine(136, CY, 39, BLACK);

    canvas.drawFastHLine(65, CY - 1, 39, YELLOW);
    canvas.drawFastHLine(136, CY - 1, 39, YELLOW);

    // Wing tips
    canvas.drawFastVLine(65, CY - 1, 9, YELLOW);
    canvas.drawFastVLine(174, CY - 1, 9, YELLOW);

    // Center aircraft reference
    canvas.fillCircle(CX, CY, 5, BLACK);
    canvas.fillCircle(CX, CY, 3, YELLOW);
}

void AttitudeGauge::drawBankScale(float r) {
    // Fixed bank-angle reference marks
    for (int deg = -60; deg <= 60; deg += 15) {
        float a = (270.0f + deg) * RAD;

        int x1 = CX + cosf(a) * 103;
        int y1 = CY + sinf(a) * 103;

        int x2 = CX + cosf(a) * 113;
        int y2 = CY + sinf(a) * 113;

        canvas.drawLine(x1, y1, x2, y2, WHITE);
    }

    // Moving bank indicator
    float a = (270.0f + r) * RAD;

    int x = CX + cosf(a) * 96;
    int y = CY + sinf(a) * 96;

    canvas.fillCircle(x, y, 4, YELLOW);
}

#if GAUGE_DEBUG_OVERLAY
void AttitudeGauge::drawReadouts(float p, float r) {
    // Black readout backgrounds
    canvas.fillRoundRect(
        62, 27, 116, 23, 5, BLACK
    );

    canvas.fillRoundRect(
        62, 190, 116, 23, 5, BLACK
    );

    canvas.setTextSize(1);
    canvas.setTextColor(CYAN);

    canvas.setCursor(73, 35);
    canvas.printf("PITCH %+5.1f", p);

    canvas.setCursor(76, 198);
    canvas.printf("ROLL %+5.1f", r);
}
#endif

void AttitudeGauge::draw(float p, float r, bool fresh, uint32_t frame, bool levelSet) {
    if (fresh) {
        drawHorizon(p, r);
        drawBankScale(r);
        drawAircraftSymbol();
#if GAUGE_DEBUG_OVERLAY
        drawReadouts(p, r);
#endif
    } else {
        // Do not present frozen numbers as a valid live measurement.
        canvas.fillScreen(BLACK);
        canvas.setTextColor(RED);
        canvas.setTextSize(2);
        canvas.setCursor(54, 105);
        canvas.print("MPU FAULT");
        canvas.setTextSize(1);
        canvas.setCursor(54, 130);
        canvas.print("Retrying I2C...");
    }
#if GAUGE_DEBUG_OVERLAY
    // Frame counter and heartbeat change even when the sensor is stationary.
    canvas.fillRect(66, 164, 114, 13, BLACK);
    canvas.setTextSize(1);
    canvas.setTextColor(WHITE);
    canvas.setCursor(69, 167);
    canvas.printf("FRAME %lu", (unsigned long)frame);
    canvas.fillCircle(120, 223, 3, (millis() / 500) % 2 ? YELLOW : BLACK);
#endif
    if (levelSet && fresh) {
        canvas.fillRoundRect(54,176,132,26,5,BLACK);
        canvas.setTextSize(2);
        canvas.setTextColor(YELLOW);
        canvas.setCursor(66,182);
        canvas.print("LEVEL SET");
    }
}
