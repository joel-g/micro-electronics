#include "button.h"
DebouncedButton::Event DebouncedButton::update(bool high, uint32_t now) {
    if (high != raw) { raw=high; changed=now; }
    if (raw != stable && uint32_t(now-changed) >= 40) {
        stable=raw;
        if (!stable && armed) {
            armed=false; active=true; longSent=false; pressedAt=changed;
            return Event::Pressed;
        }
        if (stable) {
            armed=true;
            if (active) {
                active=false;
                if (!longSent) {
                    return uint32_t(changed-pressedAt) >= 2000
                        ? Event::LongPress : Event::ShortPress;
                }
            }
        }
    }
    // A pending raw release freezes classification at its edge time. A press
    // released at 1999 ms must not become long during the 40 ms debounce wait.
    if (active && !longSent && !raw && uint32_t(now-pressedAt) >= 2000) {
        longSent=true;
        return Event::LongPress;
    }
    return Event::None;
}
