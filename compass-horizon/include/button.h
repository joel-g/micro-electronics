#pragma once
#include <cstdint>

// Pure state machine: false=pressed, true=released. No Arduino calls or sleeps.
// A button held at boot must be released before it can generate a press.
class DebouncedButton {
public:
    void begin(bool high, uint32_t now) {
        raw=stable=high; changed=now; armed=high;
    }
    bool update(bool high, uint32_t now);
    bool released() const { return stable; }
private:
    bool raw=true, stable=true, armed=true;
    uint32_t changed=0;
};
