#include "button.h"
bool DebouncedButton::update(bool high, uint32_t now) {
    if (high != raw) { raw=high; changed=now; }
    if (raw == stable || uint32_t(now-changed) < 40) return false;
    stable=raw;
    if (stable) { armed=true; return false; }
    if (!armed) return false;
    armed=false;
    return true;
}
