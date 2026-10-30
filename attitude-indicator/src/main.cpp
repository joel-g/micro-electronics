
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GC9A01A.h>
#include <math.h>
#include <stdarg.h>
#include <esp_heap_caps.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include "horizon_geometry.h"

#ifndef TFT_SPI_HZ
#define TFT_SPI_HZ 20000000
#endif

// Opt in with -D GAUGE_DEBUG_OVERLAY=1 (PlatformIO environment: debug).
#ifndef GAUGE_DEBUG_OVERLAY
#define GAUGE_DEBUG_OVERLAY 0
#endif

// ---------- Hardware configuration ----------

constexpr uint8_t MPU_ADDR = 0x68;

constexpr int I2C_SDA = 1;
constexpr int I2C_SCL = 2;

constexpr int TFT_SCLK = 8;
constexpr int TFT_MOSI = 9;
constexpr int TFT_CS   = 10;
constexpr int TFT_DC   = 11;
constexpr int TFT_RST  = 12;

constexpr int WIDTH  = 240;
constexpr int HEIGHT = 240;
constexpr int CX = 120;
constexpr int CY = 120;

// ---------- Colors (RGB565) ----------

constexpr uint16_t SKY    = 0x249F;
constexpr uint16_t GROUND = 0x8A22;
constexpr uint16_t WHITE  = 0xFFFF;
constexpr uint16_t BLACK  = 0x0000;
constexpr uint16_t YELLOW = 0xFFE0;
constexpr uint16_t CYAN   = 0x07FF;
constexpr uint16_t RED    = 0xF800;

// ---------- Display ----------

Adafruit_GC9A01A tft(TFT_CS, TFT_DC, TFT_RST);

// Fixed internal-RAM storage: 115,200 bytes, no heap fragmentation or PSRAM
// dependency. The library must not free this externally owned buffer.
class HorizonCanvas : public GFXcanvas16 {
public:
    HorizonCanvas() : GFXcanvas16(::WIDTH, ::HEIGHT, false) { buffer = pixels; }
private:
    alignas(4) uint16_t pixels[::WIDTH * ::HEIGHT] = {};
};
HorizonCanvas canvas;

// ---------- Orientation ----------

float pitch = 0.0f;
float roll = 0.0f;

float pitchZero = 0.0f;
float rollZero = 0.0f;

uint32_t lastUpdate = 0;
bool initialized = false;
bool sensorReady = false;
bool demoMode = false;
bool watchdogReady = false;
uint32_t lastSampleMs = 0;
uint32_t lastAttemptUs = 0;
uint32_t lastRecoveryMs = 0;
uint32_t consecutiveErrors = 0;

constexpr uint32_t SAMPLE_US = 10000; // 100 Hz, including during TFT transfers
constexpr uint32_t FRAME_MS = 67;     // 15 fps at conservative 20 MHz SPI
constexpr uint32_t STALE_MS = 250;

enum Stage : uint32_t { IDLE, SENSOR, DRAW, SPI_TRANSFER, LOG, RECOVER, TFT_INIT, WATCHDOG_TEST };
#ifdef GAUGE_WATCHDOG_TEST
constexpr bool watchdogTestBuild = true;
#else
constexpr bool watchdogTestBuild = false;
#endif
// Retained across software/watchdog resets; interpreted only with a valid magic.
RTC_NOINIT_ATTR uint32_t retainedMagic;
RTC_NOINIT_ATTR volatile uint32_t retainedStage;
constexpr uint32_t STAGE_MAGIC = 0x47415547;
uint32_t previousStage = IDLE;
void stage(Stage value) { retainedStage = value; }

struct Diagnostics {
    uint32_t frames = 0, samples = 0, errors = 0, recoveries = 0;
    uint32_t droppedLogs = 0, overruns = 0, filterResyncs = 0;
    uint32_t sensorMaxUs = 0, drawMaxUs = 0, spiMaxUs = 0;
    uint32_t frameMaxUs = 0, loopMaxUs = 0, sampleGapMaxUs = 0;
    uint8_t i2cError = 0;
} stats;

void recordMax(uint32_t &maximum, uint32_t value) {
    if (value > maximum) maximum = value;
}

// Never let a closed or stalled serial monitor hold up the instruments.
// Single writer, zero TX timeout, whole-record admission, no flush/retry.
void logMessage(const char *format, ...) __attribute__((format(printf, 1, 2)));
void logMessage(const char *format, ...) {
    char message[640];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    if (length <= 0) return;
    if (length >= static_cast<int>(sizeof(message)) || !Serial ||
        Serial.availableForWrite() < length) {
        ++stats.droppedLogs;
        return;
    }
    if (Serial.write(reinterpret_cast<const uint8_t *>(message), length) !=
        static_cast<size_t>(length)) ++stats.droppedLogs;
}

bool sensorFresh() {
    return initialized && sensorReady && millis() - lastSampleMs <= STALE_MS;
}

constexpr float RAD = PI / 180.0f;
constexpr float DEG = 180.0f / PI;

// ---------- MPU-6050 ----------

bool writeMPU(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    stats.i2cError = Wire.endTransmission();
    return stats.i2cError == 0;
}

bool initMPU() {
    // Verify identity before changing registers.
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x75);
    stats.i2cError = Wire.endTransmission(false);
    if (stats.i2cError != 0) return false;
    if (Wire.requestFrom(MPU_ADDR, (uint8_t)1) != 1) {
        stats.i2cError = 0xFE; // short read / bus failure
        return false;
    }
    if (Wire.read() != 0x68) {
        stats.i2cError = 0xFD; // unexpected WHO_AM_I
        return false;
    }
    // Wake sensor
    if (!writeMPU(0x6B, 0x00)) return false;

    delay(100);

    // Gyro: +/- 250 degrees/second
    if (!writeMPU(0x1B, 0x00)) return false;

    // Accelerometer: +/- 2g
    if (!writeMPU(0x1C, 0x00)) return false;

    // Digital low-pass filter
    if (!writeMPU(0x1A, 0x03)) return false;

    return true;
}

bool readMPU(
    float &ax, float &ay, float &az,
    float &gx, float &gy, float &gz
) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x3B);

    stats.i2cError = Wire.endTransmission(false);
    if (stats.i2cError != 0) {
        return false;
    }

    if (Wire.requestFrom(MPU_ADDR, (uint8_t)14) != 14) {
        stats.i2cError = 0xFE;
        while (Wire.available()) Wire.read();
        return false;
    }

    auto read16 = []() -> int16_t {
        uint16_t high = Wire.read();
        uint16_t low = Wire.read();
        return (int16_t)((high << 8) | low);
    };

    int16_t rawAX = read16();
    int16_t rawAY = read16();
    int16_t rawAZ = read16();

    read16(); // Temperature (unused)

    int16_t rawGX = read16();
    int16_t rawGY = read16();
    int16_t rawGZ = read16();

    // +/- 2g accelerometer
    ax = rawAX / 16384.0f;
    ay = rawAY / 16384.0f;
    az = rawAZ / 16384.0f;

    // +/- 250 degrees/second gyro
    gx = rawGX / 131.0f;
    gy = rawGY / 131.0f;
    gz = rawGZ / 131.0f;

    return true;
}

float angleDifference(float target, float current) {
    return remainderf(target - current, 360.0f);
}

void updateOrientation() {
    float ax, ay, az;
    float gx, gy, gz;

    if (!readMPU(ax, ay, az, gx, gy, gz)) {
        ++stats.errors;
        if (++consecutiveErrors >= 5) sensorReady = false;
        return;
    }

    uint32_t now = micros();
    if (initialized) recordMax(stats.sampleGapMaxUs, now - lastUpdate);
    float dt = (now - lastUpdate) / 1000000.0f;
    lastUpdate = now;
    lastSampleMs = millis();
    consecutiveErrors = 0;
    ++stats.samples;

    // Accelerometer-derived orientation
    float accelPitch =
        atan2f(-ax, sqrtf(ay * ay + az * az)) * DEG;

    float accelRoll =
        atan2f(ay, az) * DEG;

    if (!initialized) {
        pitch = accelPitch;
        roll = accelRoll;

        pitchZero = accelPitch;
        rollZero = accelRoll;

        initialized = true;
        return;
    }

    // Prevent huge integration jumps after delays
    if (dt <= 0.0f || dt > 0.2f) {
        // Reacquire after an outage, preserving the user's original zero.
        pitch = accelPitch;
        roll = accelRoll;
        ++stats.filterResyncs;
        return;
    }

    // Complementary filter:
    // Gyro handles fast movement.
    // Accelerometer corrects long-term drift.
    // Same 0.98 weight at 100 Hz; consistent correction at irregular intervals.
    const float alpha = expf(-dt / 0.495f);

    float predictedPitch = pitch + gy * dt;
    float predictedRoll = roll + gx * dt;

    pitch = predictedPitch +
        (1.0f - alpha) *
        angleDifference(accelPitch, predictedPitch);

    roll = predictedRoll +
        (1.0f - alpha) *
        angleDifference(accelRoll, predictedRoll);

    // Normalize roll angle
    roll = remainderf(roll, 360.0f);
}

void serviceSensor() {
    uint32_t now = micros();
    if (!sensorReady || now - lastAttemptUs < SAMPLE_US) return;
    lastAttemptUs = now; // No catch-up burst after a delay.
    stage(SENSOR);
    updateOrientation();
    recordMax(stats.sensorMaxUs, micros() - now);
    stage(IDLE);
}

void recoverSensor() {
    if (sensorReady || millis() - lastRecoveryMs < 1000) return;
    stage(RECOVER);
    lastRecoveryMs = millis();
    ++stats.recoveries;
    Wire.end();
    bool busReady = Wire.begin(I2C_SDA, I2C_SCL, 400000);
    Wire.setTimeOut(20);
    sensorReady = busReady && initMPU();
    consecutiveErrors = 0;
    logMessage("MPU recovery=%lu ready=%d error=%u\n",
               (unsigned long)stats.recoveries, sensorReady, stats.i2cError);
    stage(IDLE);
}

// ---------- Graphics ----------

void drawRotatedLine(
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


void drawHorizon(float p, float r) {
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

void drawAircraftSymbol() {
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

void drawBankScale(float r) {
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
void drawReadouts(float p, float r) {
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

void renderDisplay() {
    uint32_t frameStart = micros();
    stage(DRAW);
    float p = pitch - pitchZero;
    float r = angleDifference(roll, rollZero);
    if (demoMode) {
        float seconds = millis() * 0.001f;
        p = 20.0f * sinf(seconds);
        r = 60.0f * sinf(seconds * 0.7f);
    }

    if (sensorFresh() || demoMode) {
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
    canvas.setTextColor(demoMode ? YELLOW : WHITE);
    canvas.setCursor(69, 167);
    canvas.printf("%s %lu", demoMode ? "DEMO" : "FRAME",
                  (unsigned long)(stats.frames + 1));
    canvas.fillCircle(120, 223, 3, (millis() / 500) % 2 ? YELLOW : BLACK);
#else
    // Synthetic motion must remain visibly labeled even with diagnostics hidden.
    if (demoMode) {
        canvas.fillRect(102, 164, 36, 13, BLACK);
        canvas.setTextSize(1);
        canvas.setTextColor(YELLOW);
        canvas.setCursor(108, 167);
        canvas.print("DEMO");
    }
#endif
    recordMax(stats.drawMaxUs, micros() - frameStart);

    // Each stripe is a complete synchronous SPI transaction. Service I2C only
    // after releasing CS/SPI. No DMA, buffer reuse race, or per-frame allocation.
    for (int y = 0; y < HEIGHT; y += 8) {
        serviceSensor();
        stage(SPI_TRANSFER);
        uint32_t start = micros();
        tft.startWrite();
        tft.setAddrWindow(0, y, WIDTH, 8);
        tft.writePixels(canvas.getBuffer() + y * WIDTH, WIDTH * 8, true);
        tft.endWrite();
        recordMax(stats.spiMaxUs, micros() - start);
    }
    ++stats.frames;
    uint32_t elapsed = micros() - frameStart;
    recordMax(stats.frameMaxUs, elapsed);
    if (elapsed > FRAME_MS * 1000) ++stats.overruns;
    stage(IDLE);
}

// ---------- Main ----------

void initializeDisplay() {
    stage(TFT_INIT);
    tft.begin(TFT_SPI_HZ);
    tft.setRotation(0);
    tft.fillScreen(BLACK);
    stage(IDLE);
}

void reportDiagnostics() {
    stage(LOG);
    logMessage("up=%lu frame=%lu sample=%lu fresh=%d age=%lu err=%lu i2c=%u "
               "retry=%lu drop=%lu over=%lu sync=%lu demo=%d "
               "max_us[i2c=%lu draw=%lu spi=%lu frame=%lu loop=%lu gap=%lu] "
               "heap=%u min=%u largest=%u stack=%u reset=%d prev=%lu wdt=%d test=%d "
               "pitch=%.1f roll=%.1f\n",
               (unsigned long)millis(), (unsigned long)stats.frames,
               (unsigned long)stats.samples, sensorFresh(),
               (unsigned long)(initialized ? millis() - lastSampleMs : UINT32_MAX),
               (unsigned long)stats.errors, stats.i2cError,
               (unsigned long)stats.recoveries, (unsigned long)stats.droppedLogs,
               (unsigned long)stats.overruns, (unsigned long)stats.filterResyncs, demoMode,
               (unsigned long)stats.sensorMaxUs, (unsigned long)stats.drawMaxUs,
               (unsigned long)stats.spiMaxUs, (unsigned long)stats.frameMaxUs,
               (unsigned long)stats.loopMaxUs, (unsigned long)stats.sampleGapMaxUs,
               (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
               (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
               (unsigned)uxTaskGetStackHighWaterMark(nullptr), (int)esp_reset_reason(),
               (unsigned long)previousStage, watchdogReady, watchdogTestBuild,
               pitch - pitchZero, angleDifference(roll, rollZero));
    stage(IDLE);
}

void setup() {
    previousStage = retainedMagic == STAGE_MAGIC ? retainedStage : UINT32_MAX;
    retainedMagic = STAGE_MAGIC;
    stage(IDLE);
    Serial.setTxBufferSize(1024);
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);

    // Detect and recover a genuinely stuck loop instead of leaving stale data
    // forever. Feed only at loop completion, never inside a blocked operation.
    esp_task_wdt_config_t config = {};
    config.timeout_ms = 5000;
    config.idle_core_mask = 0;
    config.trigger_panic = true;
    esp_err_t result = esp_task_wdt_init(&config);
    if (result == ESP_ERR_INVALID_STATE) result = esp_task_wdt_reconfigure(&config);
    if (result == ESP_OK) watchdogReady = esp_task_wdt_add(nullptr) == ESP_OK;

    // Initialize SPI and round TFT
    SPI.begin(
        TFT_SCLK,
        -1,
        TFT_MOSI,
        TFT_CS
    );

    initializeDisplay();
    lastRecoveryMs = millis() - 1000;
    recoverSensor();
    logMessage("Attitude Indicator v1.0.0 framebuffer=%u internal RAM spi=%u psram=%u "
               "wdt=%d; commands: d=demo, r=TFT reset, z=zero. "
               "Keep sensor level at startup.\n",
               (unsigned)(WIDTH * HEIGHT * 2), (unsigned)TFT_SPI_HZ,
               (unsigned)ESP.getPsramSize(), watchdogReady);
    if (watchdogReady) esp_task_wdt_reset();
}

void loop() {
    uint32_t loopStart = micros();
    serviceSensor();
    recoverSensor();
    // Limit command handling so a flooding host cannot starve the instruments.
    if (Serial.available()) {
        switch (Serial.read()) {
#ifdef GAUGE_WATCHDOG_TEST
            case 'w':
                if (watchdogReady) {
                    logMessage("WATCHDOG TEST: stalling loop without feeding watchdog\n");
                    stage(WATCHDOG_TEST);
                    // Leave interrupts and other tasks running. Only the task
                    // watchdog can recover this deliberately stuck loop.
                    for (;;) delay(10);
                }
                break;
#endif
            case 'd': demoMode = !demoMode; break;
            case 'r': initializeDisplay(); break;
            case 'z':
                if (sensorFresh()) { pitchZero = pitch; rollZero = roll; }
                break;
        }
    }
    static uint32_t lastFrame = 0;
    if (millis() - lastFrame >= FRAME_MS) {
        lastFrame = millis();
        renderDisplay();
    }
    static uint32_t lastLog = 0;
    if (millis() - lastLog >= 1000) {
        lastLog = millis();
        reportDiagnostics();
    }
    recordMax(stats.loopMaxUs, micros() - loopStart);
    stage(IDLE);
    if (watchdogReady) esp_task_wdt_reset();
    delay(1);
}
