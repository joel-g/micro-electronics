#pragma once
#include "can_frame.h"
#include "engine_data.h"
#include <instrument_config.h>

class EcuDecoder {
public:
    virtual ~EcuDecoder()=default;
    virtual void onFrame(const CanFrame &,uint32_t now,engine::Data &)=0;
};

// Link Generic Dash (not Generic Dash 2). See docs/link-generic-dash.md.
// CAN ID/format and optional source availability come from the JSON manifest.
// Optional sources require confirmation; raw placeholders do not prove sensors exist.
class LinkGenericDashDecoder final : public EcuDecoder {
public:
    static constexpr uint32_t canId=instrumentConfig::canId;
    uint32_t matched=0, malformed=0, unknownMux=0, invalidValues=0;
    void onFrame(const CanFrame &f,uint32_t now,engine::Data &data) override {
        if(f.id!=canId || f.extended!=instrumentConfig::canExtended || f.rtr) return;
        if(f.dlc!=8 || f.data[1]!=0) { ++malformed; return; }
        ++matched;
        using S=engine::Signal;
        switch(f.data[0]) {
            case 0:
                put(data,S::Rpm,word(f,2),0,15000,1,0,now);
                put(data,S::MapKpa,word(f,4),0,650,1,0,now);break;
            case 1:put(data,S::BaroKpa,word(f,2),1,2000,0.1f,0,now);break;
            case 2:put(data,S::CoolantC,word(f,6),0,255,1,-50,now);break;
            case 3:
                put(data,S::IatC,word(f,2),30,255,1,-50,now);
                put(data,S::BatteryV,word(f,4),1,6500,0.01f,0,now);break;
            case 6:
                if(instrumentConfig::enableLambda) put(data,S::Lambda,word(f,4),1,3000,0.001f,0,now);
                break;
            case 7:
                if(instrumentConfig::enableFuel) put(data,S::FuelKpa,word(f,6),0,6550,1,0,now);
                break;
            case 8:
                if(instrumentConfig::enableOil) put(data,S::OilKpa,word(f,4),0,6550,1,0,now);
                break;
            // Fan status has no definition in this profile; the validator rejects it.
            case 4:case 5:case 9:case 10:case 11:case 12:case 13:break;
            default:++unknownMux;break;
        }
    }
private:
    static uint16_t word(const CanFrame &f,unsigned offset) {
        return uint16_t(f.data[offset]) | (uint16_t(f.data[offset+1])<<8);
    }
    void put(engine::Data &data,engine::Signal signal,uint16_t raw,uint16_t low,
             uint16_t high,float scale,float offset,uint32_t now) {
        auto &channel=data.get(signal);
        if(raw<low || raw>high) {channel.invalidate(now);++invalidValues;}
        else channel.set(float(raw)*scale+offset,now);
    }
};
