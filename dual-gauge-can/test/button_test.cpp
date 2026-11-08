#include "button.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

using Event=DebouncedButton::Event;
static void press(DebouncedButton &b,uint32_t at) {
    assert(b.update(false,at)==Event::None);
    assert(b.update(false,at+39)==Event::None);
    assert(b.update(false,at+40)==Event::Pressed);
}
static void release(DebouncedButton &b,uint32_t at,Event expected) {
    assert(b.update(true,at)==Event::None);
    assert(b.update(true,at+39)==Event::None);
    assert(b.update(true,at+40)==expected);
    assert(b.update(true,at+41)==Event::None);
}
int main() {
    DebouncedButton b;
    b.begin(true,0);
    // Press bounce produces one Pressed event, but no mode-switch action yet.
    assert(b.update(false,10)==Event::None);
    assert(b.update(true,17)==Event::None);
    assert(b.update(false,24)==Event::None);
    assert(b.update(false,63)==Event::None);
    assert(b.update(false,64)==Event::Pressed);
    assert(b.update(false,90)==Event::None);
    // Release bounce also produces one short action, only after stable release.
    assert(b.update(true,100)==Event::None);
    assert(b.update(false,115)==Event::None);
    release(b,120,Event::ShortPress);

    // A very short glitch is not a press at all.
    b.begin(true,0);
    assert(b.update(false,10)==Event::None);
    assert(b.update(true,20)==Event::None);
    assert(b.update(true,60)==Event::None);

    // 1999 ms remains short even though release debounce ends after 2000 ms.
    b.begin(true,0); press(b,100);
    release(b,2099,Event::ShortPress);

    // At exactly 2000 ms, fire once while still held; release cannot switch.
    b.begin(true,0); press(b,100);
    assert(b.update(false,2099)==Event::None);
    assert(b.update(false,2100)==Event::LongPress);
    for(uint32_t at=2101;at<9000;at+=7) assert(b.update(false,at)==Event::None);
    release(b,9000,Event::None);
    press(b,10000); release(b,10100,Event::ShortPress);

    // Exact-boundary/sparse-poll release must still classify as long once.
    b.begin(true,0); press(b,100);
    release(b,2100,Event::LongPress);
    b.begin(true,0); press(b,100);
    release(b,3100,Event::LongPress);

    // A release bounce around the threshold does not manufacture a short press.
    b.begin(true,0); press(b,100);
    assert(b.update(true,2090)==Event::None);
    assert(b.update(false,2110)==Event::LongPress);
    release(b,2200,Event::None);

    // Held at startup: no action, even after two seconds; release to arm.
    b.begin(false,0);
    assert(b.update(false,5000)==Event::None);
    release(b,5100,Event::None);
    press(b,5200); release(b,5300,Event::ShortPress);

    // Debounce and hold timing both survive millis() rollover.
    b.begin(true,0xffffffe0u);
    press(b,0xfffffff0u);
    assert(b.update(false,uint32_t(0xfffffff0u+1999u))==Event::None);
    assert(b.update(false,uint32_t(0xfffffff0u+2000u))==Event::LongPress);
    release(b,uint32_t(0xfffffff0u+2100u),Event::None);

    // A long hold on one instance cannot suppress a short press on the other.
    DebouncedButton h,c;
    h.begin(true,0); c.begin(true,0);
    press(h,100); press(c,200);
    release(c,350,Event::ShortPress);
    assert(h.update(false,2099)==Event::None);
    assert(h.update(false,2100)==Event::LongPress);
    press(c,2200); release(c,2300,Event::ShortPress);
    assert(h.update(false,2500)==Event::None);
    release(h,2600,Event::None);
    puts("PASS: debounce, short-on-release, 1999/2000ms boundary, one-shot hold, release suppression, boot-held, rollover, independent simultaneous buttons");
}
