#pragma once
#include <cstdint>
namespace colors {
enum : uint8_t { Background, White, Muted, Gold, North, Cyan, Red, Green,
                 Sky, Ground, Black, Yellow, HorizonCyan, DarkGold,
                 EngineBlue, EngineGreen, EngineOrange, EngineRed, Panel, Track };
// Original indices are unchanged for compass/horizon and tile-transfer compatibility.
inline constexpr uint16_t rgb565[]={0x0841,0xFFFF,0x738E,0xFEA0,0xF985,0x5E7F,0xF800,0x07E0,
    0x249F,0x8A22,0x0000,0xFFE0,0x07FF,0x8A60,
    0x349F,0x3EAE,0xFC60,0xF986,0x10A3,0x2946};
}
