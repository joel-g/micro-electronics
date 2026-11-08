#pragma once
#include <cstdint>
#include <cmath>

namespace engine {
enum class Signal : uint8_t { Rpm, CoolantC, OilKpa, FuelKpa, MapKpa, BaroKpa, Lambda, BatteryV, Fan1, Fan2, IatC, Count };
enum class State { Never, Invalid, Stale, Valid };
struct Channel {
    float value=0;
    uint32_t updated=0;
    bool received=false, valid=false;
    void set(float v,uint32_t now) { value=v; updated=now; received=true; valid=std::isfinite(v); }
    void invalidate(uint32_t now) { updated=now; received=true; valid=false; }
    State state(uint32_t now,uint32_t timeout=1500) const {
        if(!received) return State::Never;
        if(!valid) return State::Invalid;
        return uint32_t(now-updated)>timeout ? State::Stale : State::Valid;
    }
};
struct Data {
    Channel channels[static_cast<unsigned>(Signal::Count)]{};
    Channel &get(Signal s) { return channels[static_cast<unsigned>(s)]; }
    const Channel &get(Signal s) const { return channels[static_cast<unsigned>(s)]; }
};
}
