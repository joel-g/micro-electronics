#include "button.h"
#include <cassert>
#include <cstdio>

int main() {
    DebouncedButton b;
    b.begin(true,0);
    assert(!b.update(false,10));
    assert(!b.update(true,17));
    assert(!b.update(false,24));
    assert(!b.update(false,63));
    assert(b.update(false,64));
    // Holding never repeats, including well beyond any proposed long press.
    for(unsigned now=65;now<10000;now+=7) assert(!b.update(false,now));
    assert(!b.update(true,10000));
    assert(!b.update(false,10015));
    assert(!b.update(true,10020));
    assert(!b.update(true,10060));
    assert(!b.update(false,10100));
    assert(b.update(false,10140));
    // Already held at power-on: release, then a NEW stable press is required.
    b.begin(false,0);
    assert(!b.update(false,5000));
    assert(!b.update(true,5100));
    assert(!b.update(true,5140));
    assert(!b.update(false,5200));
    assert(b.update(false,5240));
    // Unsigned elapsed-time debounce works across millis() rollover.
    b.begin(true,0xffffffe0u);
    assert(!b.update(false,0xfffffff0u));
    assert(!b.update(false,0x17u));
    assert(b.update(false,0x18u));
    puts("PASS: bounce, one event per held press, rearming, boot-held, timer rollover");
}
