#pragma once
#include <cstddef>
#include <cstdint>

#include "gauge_types.h"
#include <instrument_config.h>

namespace gaugeConfig {
inline const char *name(Gauge g) {
    switch(g) {
        case Gauge::Horizon:return "horizon"; case Gauge::Compass:return "compass";
        case Gauge::Coolant:return "coolant"; case Gauge::Oil:return "oil";
        case Gauge::Rpm:return "rpm"; case Gauge::Boost:return "boost";
        case Gauge::Fuel:return "fuel"; case Gauge::Fans:return "fans";
        case Gauge::Lambda:return "lambda"; case Gauge::Battery:return "battery";
        case Gauge::Iat:return "iat";
    }
    return "unknown";
}
template<size_t N> bool contains(const Gauge (&list)[N],Gauge value) {
    for(auto g:list) if(g==value) return true;
    return false;
}
template<size_t N> Gauge next(const Gauge (&list)[N],Gauge value) {
    for(size_t i=0;i<N;++i) if(list[i]==value) return list[(i+1)%N];
    return list[0];
}
}
