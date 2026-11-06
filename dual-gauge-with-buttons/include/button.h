#pragma once
#include <cstdint>

// Pure state machine: false=pressed, true=released. No Arduino calls or sleeps.
// A button held at boot must be released before it can generate a press.
// Duration uses the start of each debounced edge, excluding release debounce.
class DebouncedButton {
public:
    enum class Event { None, Pressed, ShortPress, LongPress };
    void begin(bool high, uint32_t now) {
        raw=stable=high; changed=now; armed=high;
        active=false; longSent=false; pressedAt=now;
    }
    Event update(bool high, uint32_t now);
    bool released() const { return stable; }
private:
    bool raw=true, stable=true, armed=true;
    bool active=false, longSent=false;
    uint32_t changed=0, pressedAt=0;
};
