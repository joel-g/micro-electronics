#pragma once
#include <Arduino.h>
#include <stdarg.h>

// One application writer, zero TX timeout, no flush or wait for a host.
inline bool logLine(const char *format, ...) __attribute__((format(printf, 1, 2)));
inline bool logLine(const char *format, ...) {
    char text[1024];
    va_list args;
    va_start(args, format);
    int length = vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if (length <= 0 || length >= static_cast<int>(sizeof(text)) ||
        Serial.availableForWrite() < length) return false;
    return Serial.write(reinterpret_cast<const uint8_t *>(text), length) ==
           static_cast<size_t>(length);
}

inline void beginDiagnostics() {
    Serial.setTxBufferSize(2048);
    Serial.begin(115200);
    Serial.setTxTimeoutMs(0);
}
