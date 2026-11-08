#pragma once
#include "Arduino.h"
namespace fake {
struct Panel { std::array<uint16_t,240*240> pixels{}; unsigned writes=0, inits=0; };
inline std::array<Panel,2> panels;
}
class Adafruit_GC9A01A {
public:
    Adafruit_GC9A01A(int cs,int dc,int rst) : cs(cs), index(cs==10?0:1) {
        assert((cs==10 || cs==7) && dc==11 && rst==-1);
    }
    void begin(uint32_t hz) {
        assert(fake::busStarts==1 && hz==20000000);
        assert(fake::pins[10]==HIGH && fake::pins[7]==HIGH);
        ++fake::panels[index].inits;
    }
    void setRotation(int rotation) { assert(rotation==0); }
    void fillScreen(uint16_t color) { fake::panels[index].pixels.fill(color); }
    void startWrite() {
        assert(fake::selected==-1 && fake::pins[7]==HIGH && fake::pins[10]==HIGH);
        fake::selected=cs; digitalWrite(cs,LOW);
    }
    void setAddrWindow(int px,int py,int w,int h) {
        assert(fake::selected==cs && w==16 && h==8);
        x=px; y=py;
    }
    void writePixels(uint16_t *pixels,int count,bool blocking) {
        assert(fake::selected==cs && count==128 && blocking);
        for(int row=0;row<8;++row) for(int col=0;col<16;++col)
            fake::panels[index].pixels.at((y+row)*240+x+col)=pixels[row*16+col];
        ++fake::panels[index].writes;
    }
    void endWrite() {
        assert(fake::selected==cs); digitalWrite(cs,HIGH); fake::selected=-1;
    }
private:
    int cs,index,x=0,y=0;
};
