#include "engine_gauge.h"
#include <cmath>
#include <cstring>
using namespace colors;
namespace {
constexpr float radians=3.14159265358979323846f/180.0f;
struct Point {int16_t x,y;};
Point polar(float degrees,float radius) {
    float a=degrees*radians;
    return {int16_t(std::lround(120+radius*std::cos(a))),int16_t(std::lround(120+radius*std::sin(a)))};
}
}
void EngineGauge::centered(const char *s,int y,uint8_t size,uint8_t color) {
    canvas.setTextSize(size);canvas.setTextColor(color);
    canvas.setCursor(120-int(strlen(s))*3*size,y);canvas.print(s);
}
void EngineGauge::scale(Gauge gauge,const gaugeStyle::Style &style,float position,bool valid) {
    // 54 fixed segments, no animation loop or allocation. Only changed tiles reach SPI.
    for(int i=0;i<54;++i) {
        float a=135+i*5.0f;
        auto p0=polar(a,103),p1=polar(a+3.8f,103),p2=polar(a+3.8f,112),p3=polar(a,112);
        uint8_t color=Track;
        if(valid && (i+0.5f)/54<=position) color=style.accent;
        if(valid && gauge==Gauge::Coolant) color=gaugeStyle::coolantColor(style.minimum+(i+0.5f)/54*(style.maximum-style.minimum));
        canvas.fillTriangle(p0.x,p0.y,p1.x,p1.y,p2.x,p2.y,color);
        canvas.fillTriangle(p0.x,p0.y,p2.x,p2.y,p3.x,p3.y,color);
    }
    for(int i=0;i<=12;++i) {
        float a=135+i*22.5f;
        auto inner=polar(a,i%3==0?89:93),outer=polar(a,98);
        canvas.drawLine(inner.x,inner.y,outer.x,outer.y,valid?Muted:Track);
    }
    if(valid && gauge!=Gauge::Fans) {
        // Discrete pointer follows the same quantization as redraw suppression.
        float a=135+std::round(position*54)*5;
        auto tip=polar(a,101),left=polar(a-3,87),right=polar(a+3,87);
        canvas.fillTriangle(tip.x,tip.y,left.x,left.y,right.x,right.y,White);
    }
}
void EngineGauge::icon(Gauge gauge,uint8_t color) {
    if(gauge==Gauge::Coolant || gauge==Gauge::Iat) {
        canvas.drawRoundRect(116,33,8,20,4,color);
        canvas.fillCircle(120,54,5,color);
        canvas.drawFastVLine(120,39,15,color);
        canvas.drawFastHLine(127,38,5,color);canvas.drawFastHLine(127,45,3,color);
    } else if(gauge==Gauge::Battery) {
        canvas.drawRoundRect(106,37,28,17,2,color);canvas.fillRect(111,34,6,3,color);canvas.fillRect(124,34,6,3,color);
        canvas.drawFastHLine(110,45,7,color);canvas.drawFastVLine(113,42,7,color);canvas.drawFastHLine(123,45,7,color);
    } else {
        canvas.drawCircle(120,46,11,color);
        canvas.drawLine(120,46,126,39,color);canvas.fillCircle(120,46,2,color);
    }
}
bool EngineGauge::draw(Gauge gauge,const engine::Data &data,uint32_t now,bool force) {
    auto reading=engineReading(gauge,data,now);
    auto style=gaugeStyle::forReading(gauge,reading);
    bool valid=reading.state==engine::State::Valid;
    float position=valid?gaugeStyle::position(style,reading.value):0;
    int segment=int(std::round(position*54));
    if(!force && initialized && previous==gauge && last.state==reading.state &&
       !strcmp(last.text,reading.text) && lastAccent==style.accent && lastSegment==segment) return false;
    initialized=true;previous=gauge;last=reading;lastAccent=style.accent;lastSegment=segment;
    canvas.fillScreen(Black);
    canvas.setTextWrap(false);
    canvas.drawCircle(120,120,117,Track);
    scale(gauge,style,position,valid);
    icon(gauge,valid?style.accent:uint8_t(Muted));
    centered(reading.label,68,reading.label && strlen(reading.label)>15?1:2,White);
    canvas.drawFastHLine(76,89,88,Track);
    if(valid) {
        uint8_t size=strlen(reading.text)>8?2:strlen(reading.text)>5?3:5;
        centered(reading.text,104,size,White);
        centered(!strcmp(reading.units,"F")?"DEG F":!strcmp(reading.units,"C")?"DEG C":reading.units,149,1,Muted);
        canvas.fillRoundRect(64,172,112,23,7,Panel);
        canvas.drawRoundRect(64,172,112,23,7,style.accent);
        centered(style.status,180,1,style.accent);
        if(gauge!=Gauge::Fans) {
            char low[12],high[12];
            snprintf(low,sizeof(low),"%.0f",style.minimum);snprintf(high,sizeof(high),"%.0f",style.maximum);
            canvas.setTextSize(1);canvas.setTextColor(Muted);
            canvas.setCursor(66-int(strlen(low))*3,201);canvas.print(low);
            canvas.setCursor(174-int(strlen(high))*3,201);canvas.print(high);
        }
        centered("LINK / LIVE",222,1,Muted);
    } else {
        centered("---",104,5,Muted);
        centered(reading.state==engine::State::Never?"NO CAN DATA":reading.state==engine::State::Stale?"STALE DATA":"INVALID DATA",153,1,
                 reading.state==engine::State::Never?Muted:EngineOrange);
        centered(reading.state==engine::State::Never?"SOURCE UNAVAILABLE":"WAITING FOR VALID DATA",180,1,Muted);
        centered("LINK / CAN",222,1,Muted);
    }
    return true;
}
