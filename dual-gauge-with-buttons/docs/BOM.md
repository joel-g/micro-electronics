# Dual Gauge with Buttons — bill of materials

Module-level BOM for one dual-display prototype, recorded 2026-11-05. Quantities
are totals for the assembly, not additional purchases. Exact module manufacturers,
ordering suffixes, supplier links, board revisions, and cable specifications have
not been recorded; no specific vendor is inferred from generic module names.

| Item | Qty | Module / part identifier | Chip identifier | Purpose |
| --- | --- | --- | --- | --- |
| DGB-01 | 1 | ESP32-S3 SuperMini, 4 MB flash / 2 MB PSRAM | ESP32-S3, exact ordering suffix unrecorded | Shared controller; PSRAM disabled |
| DGB-02 | 2 | 1.28-inch round SPI TFT module, 240×240, 3.3 V | GC9A01 family; exact suffix unrecorded | Horizon CS GPIO10; Compass CS GPIO7 |
| DGB-03 | 1 | MPU-6050 breakout, carrier designation unrecorded | MPU-6050 | Pitch/roll; I²C 0x68 |
| DGB-04 | 1 | GY-271 module, QMC5883P variant | QMC5883P | Magnetic heading; I²C 0x2C |
| DGB-05 | 1 set | Signal and power interconnects | Not applicable | Shared SPI, shared I²C, separate CS, 3.3 V and ground |
| DGB-06 | 1 | USB data/power cable compatible with the fitted board | Not applicable | Flashing, diagnostics and bench power |
| DGB-07 | 2 | Normally-open momentary push buttons; exact MPN unrecorded | Not applicable | GPIO13 attitude and GPIO4 compass, each to GND; internal pull-ups |
| DGB-08 | 1 fitted, unused | CAN transceiver module; model/MPN unrecorded | Not recorded | Physically connected to GPIO5/6; ignored by this firmware |

Exact manufacturer MPNs and supplier SKUs remain to be recorded separately for
each module. Item IDs are documentation identifiers, not PCB reference designators.

Both TFTs share DC and reset as well as clock/MOSI, but must retain separate CS
connections. Match interface and voltage to the tested modules. The library name
`Adafruit GC9A01A` does not establish the manufacturer of the complete display.
GY-271 variants containing QMC5883L or HMC5883L are not substitutes for QMC5883P.
The generic PlatformIO DevKitC target is not the SuperMini's manufacturer part number.

See the [wiring diagram and table](../README.md#wiring) and
[verification record](verification.md). The two-display assembly's regulator
load and supply margin have not been measured; do not infer them from software
heap or frame-rate tests.
