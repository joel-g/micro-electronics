#include "display.h"
#include <algorithm>
#include <cstdio>

int main() {
    Display horizon(10), compass(7);
    Display::beginBus(); horizon.begin(); compass.begin();
    assert(fake::resetPulses==1 && fake::busStarts==1);
    assert(fake::panels[0].inits==1 && fake::panels[1].inits==1);
    auto *h=horizon.canvas().getBuffer(), *c=compass.canvas().getBuffer();
    assert(h!=c);
    std::fill(h,h+57600,colors::Red);
    std::fill(c,c+57600,colors::Green);
    horizon.present(); compass.present();
    unsigned rounds=0;
    while(horizon.busy() || compass.busy()) {
        auto hw=fake::panels[0].writes, cw=fake::panels[1].writes;
        horizon.flushBatch(4); compass.flushBatch(4);
        assert(fake::panels[0].writes-hw<=4 && fake::panels[1].writes-cw<=4);
        assert(fake::selected==-1 && fake::pins[7]==HIGH && fake::pins[10]==HIGH);
        assert(++rounds<200);
    }
    assert(rounds==113 && horizon.frames==1 && compass.frames==1);
    assert(horizon.tiles==450 && compass.tiles==450);
    for(auto p:fake::panels[0].pixels) assert(p==0xF800);
    for(auto p:fake::panels[1].pixels) assert(p==0x07E0);

    // An unchanged frame transfers nothing on either panel.
    horizon.present(); compass.present();
    horizon.flushBatch(); compass.flushBatch();
    assert(!horizon.busy() && !compass.busy());
    assert(horizon.tiles==450 && compass.tiles==450);
    assert(horizon.frames==1 && compass.frames==1 && horizon.completed==2);

    // Changes at opposite ends exercise separate shadow buffers and addressing.
    h[57599]=colors::White; c[0]=colors::Black;
    horizon.present(); compass.present();
    horizon.flushBatch(); compass.flushBatch();
    assert(!horizon.busy() && !compass.busy());
    assert(horizon.tiles==451 && compass.tiles==451);
    assert(fake::panels[0].pixels[57599]==0xFFFF && fake::panels[0].pixels[0]==0xF800);
    assert(fake::panels[1].pixels[0]==0 && fake::panels[1].pixels[57599]==0x07E0);
    assert(fake::resetPulses==1 && fake::busStarts==1);
    // A mode change replaces a partly transmitted face without disturbing the
    // other display's queued image or leaving old pixels behind.
    std::fill(h,h+57600,colors::White); std::fill(c,c+57600,colors::Sky);
    horizon.present(); compass.present(); horizon.flushBatch(); compass.flushBatch();
    horizon.cancel(); std::fill(h,h+57600,colors::Black); horizon.present();
    rounds=0;
    while(horizon.busy() || compass.busy()) {
        horizon.flushBatch(); compass.flushBatch(); assert(++rounds<200);
    }
    for(auto p:fake::panels[0].pixels) assert(p==0);
    for(auto p:fake::panels[1].pixels) assert(p==0x249F);
    assert(fake::resetPulses==1 && fake::busStarts==1);

    // Reinitializing one panel clears its pixels, so redraw even when the
    // canvas is identical to the last completed frame.
    const auto compassWrites = fake::panels[1].writes;
    horizon.begin();
    assert(fake::panels[0].pixels[0]==0x0841);
    horizon.present();
    horizon.flushBatch(450);
    assert(!horizon.busy());
    for(auto p:fake::panels[0].pixels) assert(p==0);
    assert(fake::panels[1].writes==compassWrites && fake::panels[1].pixels[0]==0x249F);
    puts("PASS: shared startup reset, CS isolation, bounded transfers, independent shadows, unchanged suppression, partial-frame replacement, and redraw after reinitialization");
}
