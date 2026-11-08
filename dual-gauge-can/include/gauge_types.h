#pragma once
#include <cstdint>
// Persisted IDs: never renumber or reuse. Fans is reserved pending a defined stream.
enum class Gauge : uint8_t { Horizon=0, Compass=1, Coolant=2, Oil=3, Rpm=4, Boost=5, Fuel=6, Fans=7, Lambda=8, Battery=9, Iat=10 };
